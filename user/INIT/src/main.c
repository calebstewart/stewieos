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
