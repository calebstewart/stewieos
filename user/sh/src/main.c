#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "builtin.h"

int shell_exit = 0;

void shell(FILE* filp, char** envp);

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
		if( access(name, F_OK | X_OK) != 0 ){
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
		if( access(buffer, F_OK | X_OK) == 0 ){
			return buffer;
		}
		*colon = old_value;
		path = colon+1;
	} while( old_value != 0 );
	
	return NULL;
}

int main(int argc, char** argv, char** envp)
{	
	shell(stdin, envp);
	
	return 0;
}

void shell(FILE* filp, char** envp)
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
	
	if( getcwd(cwd, 256) == NULL ){
		strcpy(cwd, "sh");
	}
	
	while( 1 )
	{
		if( !isatty(fileno(filp)) ){
			if( feof(filp) ){
				break;
			}
		} else {
			getcwd(cwd, 256);
			printf("root@%s $ ",cwd);
			fflush(stdout);
		}
		// if this returns null, it's the EOF and that means this
		// isn't a TTY. We can safely break no matter what.
		if( fgets(line, 512, filp) == NULL ){
			break;
		}
		// Remove the newline character
		strip_newline(line);
		
		if( strlen(line) == 0 ){
			continue;
		}
		
		// Count the parameters
		argc = 1;
		for(unsigned int i = 0; line[i]; ++i){
			if( line[i] == ' ' ){
				argc++;
			}
		}
		
		// Allocate space for the arguments
		argv = malloc(sizeof(char*)*(argc+1));
		if( argv == 0 ){
			printf("error: unable to allocate argument space.\n");
		}
		// Fill in the first and last, as we know explicitly what they will be
		argv[0] = line;
		argv[argc] = NULL;
		
		// Iterate over all parameters setting spaces to null and assigning 
		// seperate string pointers within argv
		for(unsigned int i = 0, p = 1; p < argc; ++i){
			if( line[i] == ' ' ){
				line[i] = 0;
				argv[p++] = &line[i+1];
			}
		}
		
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
			printf("error: unable to fork: %d\n", errno);
		} else if( pid == 0 ){ // child
			result = execve(argv[0], argv, envp);
			printf("error: unable to execute %s: %d\n", argv[0], result);
		} else { // parent
			if( wait_on_child ){
				pid = waitpid(pid, &status, 0);
			}
		}
		
		free(argv);
		
	}
	
}