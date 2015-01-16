#include <stdio.h>
#include <sys/module.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static void read_line(FILE* filp, char* buffer, size_t max)
{
	size_t n = 0;
	char c = 0;
	while( !feof(filp) && n < (max-1) && c != '\n' ){
		c = fgetc(filp);
		buffer[n] = c;
		n++;
	}
	if( c == '\n' || c == -1 ){
		buffer[n-1] = 0;
	}
	buffer[n] = 0;
}

int main(int argc, char** argv)
{
	FILE* modfile = NULL;
	char module_line[1024];
	int result = 0, lineno = 0;
	char* arguments[] = {"/bin/sh",NULL};
	char* environ[] = {"PATH=/bin",NULL};
	pid_t pid = 0;
	
	printf("INIT: started with pid %d.\n", getpid());
	
	FILE* modules = fopen("/etc/modules.conf", "r");
	
	while( !feof(modules) )
	{
		read_line(modules, module_line, 1024);
		if( strlen(module_line) == 0 ){
			continue;
		}
		
		if( module_line[0] == '#' ){
			continue;
		}
		
		result = insmod(module_line);
		if( result == -1 ){
			printf("error: unable to load module %s: %d\n", module_line, errno);
			errno = 0;
		} else {
			printf("INIT: loaded module %s.\n", module_line);
		}
		
	}
	
	pid = fork();
	if( pid < 0 ){
		printf("INIT: unable to fork process. error code %d.\n", errno);
		return 0;
	} else if( pid == 0 ){
		char* syslogd_args[] = {"/bin/syslogd", NULL}, *syslogd_env[] = {"PATH=/bin", NULL};
		result = execve(syslogd_args[0], syslogd_args, syslogd_env);
		printf("INIT: error: unable to execute syslogd. error code %d.\n", errno);
		exit(-1);
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
