#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "builtin.h"
#include "slre/slre.h"
#include <string.h>
//#include "readline/readline.h"

int shell_exit = 0;

void shell(FILE* outfl, FILE* filp, char** envp);

void strip_newline(char* buffer)
{
	while( *buffer ){
		if( *buffer == '\n' ){
			*buffer = 0;
			return;
		}
		buffer++;
	}
}

/* Find an executable within the given path environment variable with the given name.
 * The full path to the executable is stored in buffer. If 'name' is an absolute path
 * then existence is simply checked, and then the path is copied to buffer.
 * 
 * returns NULL for a non-existent executable and returns buffer for a good executable.
 */
static char* find_exec(const char* name, char* buffer, char* path)
{
	char old_value = 0;
	char* colon = NULL;
	
	if( *name == '/' ){
		if( access(name, F_OK) != 0 ){
			return NULL;
		} else {
			strcpy(buffer, name);
			return buffer;
		}
	}
	
	do
	{
		colon = strchrnul(path, ':');
		old_value = *colon;
		sprintf(buffer, "%s/%s", path, name);
		if( access(buffer, F_OK) == 0 ){
			return buffer;
		}
		*colon = old_value;
		path = colon+1;
	} while( old_value != 0 );
	
	return NULL;
}

static int shell_read_line(FILE* outfl, FILE* filp, size_t* lineno, char* buffer, size_t length)
{
	char c;
	char* iter = buffer;
	size_t count = 0;
	int inside_quote = 0, waiting_for_close = 0, inside_single = 0;
	int escape = 0;
	
	while( (size_t)(iter-buffer) < length && (c = fgetc(filp)) != EOF )
	{
		if( c == '\n' && isatty(fileno(filp)) && (waiting_for_close || escape) ){
			fprintf(outfl, "> ");
			fflush(outfl);
			(*lineno)++;
			escape = 0;
			continue;
		} else if( c == '\n' && !waiting_for_close ){
			(*lineno)++;
			break;
		} else if( c == '\n' ){
			if( escape == 1 ){
				escape = 0;
				continue;
			} else {
				fprintf(stderr, "error: missing closing brace or quote at line %d.\n", *lineno);
			}
			return -1;
		}
		
		if( c == '\'' && !inside_quote ){
			inside_single = !inside_single;
			waiting_for_close = !waiting_for_close;
			//continue;
		}
		
		if( c == '"' && !inside_single ){
			inside_quote = !inside_quote;
			waiting_for_close = !waiting_for_close;
			//continue;
		}
		
		if( c == '\\' ){
			escape = 1;
			continue;
		}
		
		*iter = c;
		iter++;
	}
	
// 	if( c == EOF ){
// 		return -1;
// 	}
	
	if( ((size_t)(iter-buffer)) == length ){
		fprintf(stderr, "error: buffer overflow. line too long.\n");
		return 1;
	}
	
	*iter = 0;
	
	return 0;
}

int main(int argc, char** argv, char** envp)
{
	
	if( argc != 1 )
	{
		FILE* file = fopen(argv[1], "r");
		if( file == NULL ){
			perror("unable to run script");
			return -1;
		}
		shell(stdout, file, envp);
		return 0;
	}
	
	shell(stdout, stdin, envp);
	
	return 0;
}

static void remove_character(char* where)
{
	for(; *where; where++){
		*where = *(where+1);
	}
}

void shell(FILE* outfl, FILE* filp, char** envp)
{
	char exec_buffer[512];
	char* exec_path;
	char line[512];
	int argc = 0;
	char** argv;
	pid_t pid = 0;
	int result = 0, status = 0;
	char cwd[256];
	int wait_on_child = 1;
	int inside_quotes = 0;
	size_t lineno;
	size_t length = 0;
	size_t iter = 0;
	struct slre_cap match[2];
	const char* regex = "(\"[^\"]+\"|\'[^\']+\'|[^'\" ]+)";
	
	if( getcwd(cwd, 256) == NULL ){
		strcpy(cwd, "sh");
	}
	
	while( 1 )
	{
		if( !isatty(fileno(filp)) ){
			if( feof(filp) ){
				break;
			}
		} else if( outfl ) {
			getcwd(cwd, 256);
			printf("root@%s $ ",cwd);
			fflush(stdout);
		}
		// if this returns null, it's the EOF and that means this
		// isn't a TTY. We can safely break no matter what.
// 		if( fgets(line, 512, filp) == NULL ){
// 			break;
// 		}
		result = shell_read_line(outfl, filp, &lineno, line, 512);
		if( result > 0 ){
			continue;
		} else if( result < 0 ){
			break;
		}
		// Remove the newline character
		strip_newline(line);
		
		length = strlen(line);
		
		if( length == 0 || line[0] == '#' ){
			continue;
		}
		
		argc = 0;
		iter = 0;
		while( iter < length && (result = slre_match(regex, line+iter, length-iter, match, 2, 0)) > 0 ){
			argc++;
			iter += result;
		}
		
// 		// Count the parameters
// 		argc = 1;
// 		inside_quotes = 0;
// 		for(unsigned int i = 0; line[i]; ++i){
// 			if( line[i] == ' ' && inside_quotes == 0){
// 				argc++;
// 			} else if( line[i] == '"' ){
// 				inside_quotes = inside_quotes ? 0 : 1;
// 			}
// 		}
		
		// Allocate space for the arguments
		argv = malloc(sizeof(char*)*(argc+1));
		if( argv == 0 ){
			printf("error: unable to allocate argument space.\n");
		}
		// Fill in the first and last, as we know explicitly what they will be
		argv[0] = line;
		argv[argc] = NULL;
		
		unsigned int i = 0;
		iter = 0;
		while( iter < length && (result = slre_match(regex, line+iter, length-iter, match, 2, 0)) > 0 ){
			argv[i] = (char*)match[0].ptr;
			argv[i][match[0].len] = 0;
			iter += (match[0].len+1);
			
			if( argv[i][0] == '"' || argv[i][0] == '\'' ){
				remove_character(argv[i]);
				argv[i][strlen(argv[i])-1] = 0;
			}
			
			i++;
		}
		
		// Iterate over all parameters setting spaces to null and assigning 
		// seperate string pointers within argv
// 		inside_quotes = 0;
// 		for(unsigned int i = 0, p = 1; p < argc; ++i){
// 			if( line[i] == ' ' && inside_quotes == 0 ){
// 				line[i] = 0;
// 				argv[p++] = &line[i+1];
// 				printf("Argument %d: %s\n", p-1, argv[p-1]);
// 			}
// 			if( line[i] == '"' ){
// 				remove_character(&line[i]);
// 				inside_quotes = inside_quotes ? 0 : 1;
// 			}
// 		}
		
		if( strcmp(argv[argc-1], "&") == 0 ){
			wait_on_child = 0;
		} else {
			wait_on_child = 1;
		}
		
		// look for a builtin command
		builtin_command_func_t cmd = find_command(argv[0]);
		if( cmd != NULL )
		{
			cmd(argc, argv);
			free(argv);
			continue;
		}
		
		// look for an executable
		exec_path = find_exec(argv[0], exec_buffer, getenv("PATH"));
		if( exec_path == NULL )
		{
			printf("%s: command not found\n", argv[0]);
			free(argv);
			continue;
		} else {
			argv[0] = exec_path;
		}
		
		pid = fork();
		if( pid < 0 ){ // error
			perror("error: unable to fork");
		} else if( pid == 0 ){ // child
			result = execve(argv[0], argv, envp);
			perror("error: unable to execute file");
			exit(0);
		} else { // parent
			if( wait_on_child ){
				pid = waitpid(pid, &status, 0);
			}
		}
		
		free(argv);
		
	}
	
}