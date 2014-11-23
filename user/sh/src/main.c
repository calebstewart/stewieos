#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

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

int main(int argc, char** argv, char** envp)
{	
	shell(stdin, envp);
	
	return 0;
}

void shell(FILE* filp, char** envp)
{
	char line[512];
	int argc = 0;
	char** argv;
	pid_t pid = 0;
	int result = 0, status = 0;
	char cwd[256];
	
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
		
		if( strcmp(argv[0], "cd") == 0 || strcmp(argv[0], "chdir") == 0 )
		{
			if( argc != 2 ){
				printf("%s: usage:\n\t%s new_path\n", argv[0], argv[0]);
			}
			result = chdir(argv[1]);
			if( result < 0 ){
				printf("error: unable to change directories: %d\n", errno);
			}
			if( getcwd(cwd, 256) == NULL ){
				strcpy(cwd, "sh");
			}
			free(argv);
			continue;
		}
			
		
		// Look for the requested file
		int fd = open(argv[0], O_RDONLY);
		if( fd < 0 ){
			printf("%s: command not found\n", argv[0]);
			free(argv);
			continue;
		}
		
		close(fd);
		
		pid = fork();
		if( pid < 0 ){ // error
			printf("error: unable to fork: %d\n", errno);
		} else if( pid == 0 ){ // child
			result = execve(argv[0], argv, envp);
			printf("error: unable to execute %s: %d\n", argv[0], result);
		} else { // parent
			pid = waitpid(pid, &status, 0);
		}
		
		free(argv);
		
	}
	
}