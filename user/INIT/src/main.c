#include <stdio.h>
#include <sys/module.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	FILE* modfile = NULL;
	char module_line[1024];
	int result = 0, lineno = 0;
	char* arguments[] = {"/bin/sh",NULL};
	char* environ[] = {"PATH=/bin",NULL};
	pid_t pid = 0;
	
	printf("INIT: started with pid %d.\n", getpid());
	
	// Open the modules configuration file
	modfile = fopen("/etc/modules.conf", "r");
	if( !modfile ){
		printf("INIT: error: unable to open /etc/modules.conf: error code %d\n", errno);
		return -1;
	}
	
	// Attempt to load in all requested modules
	while( !feof(modfile) )
	{
		if( fgets(module_line, 1024, modfile) == NULL ){
			break;
		}
		
		lineno++;
		
		if( module_line[0] == '#' || module_line[0] == 0 || module_line[0] == '\n' ){
			continue;
		}
		
		*strchr(module_line, '\n') = 0;
		
		printf("INIT: loading module \"%s\"...\n", module_line);
		
		result = insmod(module_line);
		if( result < 0 ){
			printf("INIT: modules.conf(%d): error: unable to load module, code %d.\n", lineno, errno);
			errno = 0;
		}
	}
	
	// Execute the shell
	printf("INIT: executing default shell (%s)...\n", arguments[0]);
	
	pid = fork();
	
	if( pid < 0 )
	{
		printf("INIT: unable to fork process: error code %d.\n", errno);
		return 0;
	} else if( pid != 0 ){
		printf("INIT: forked process.\n");
		exit(0);
		return 0;
	} else {
		result = execve(arguments[0], arguments, environ);
		printf("INIT: error: unable to execute shell: error code %d.\n", errno);
	}
	
	return 0;
}
