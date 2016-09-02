#include <stdio.h>
#include <sys/module.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/message.h>

int main(int argc, char** argv)
{
	FILE* modfile = NULL;
	char module_line[1024];
	int result = 0, lineno = 0;
	char* arguments[] = {"/bin/sh",NULL};
	char* environ[] = {"PATH=/bin",NULL};
	pid_t pid = 0;

	// Load default modules
	result = modules_load();
	if( result != 0 ){
		syslog(SYSLOG_INFO,"%s: error while loading modules: %d", argv[0], result);
	}

	// Open stdin/stdout/stderr
	int fd = open("/dev/tty0", O_RDWR);
	if( fd < 0 ){
		syslog(SYSLOG_ERR, "%s: unable to open tty0: %d", argv[0], errno);
		return 0;
	}
	if( dup(fd) < 0 ){
		syslog(SYSLOG_ERR, "%s: unable to duplicate tty0: %d", argv[0], errno);
		return 0;
	}
	if( dup(fd) < 0 ){
		syslog(SYSLOG_ERR, "%s: unable to duplicate tty0: %d", argv[0], errno);
		return 0;
	}
	
	result = services_start();
	if( result < 0 ){
		syslog(SYSLOG_INFO,"serviced: error: unable to start services.");
		return -1;
	}

	// we should monitor the services... :(
	while( 1 ){
		//sleep(10);
	}
	
	pid = fork();
	if( pid < 0 ){
		syslog(SYSLOG_INFO,"INIT: unable to fork process. error code %d.", errno);
		return 0;
	} else if( pid == 0 ){
		char* syslogd_args[] = {"/bin/syslogd", NULL}, *syslogd_env[] = {"PATH=/bin", NULL};
		result = execve(syslogd_args[0], syslogd_args, syslogd_env);
		syslog(SYSLOG_INFO,"INIT: error: unable to execute syslogd. error code %d.", errno);
		exit(-1);
	}

	// Execute the shell
	syslog(SYSLOG_INFO,"INIT: executing default shell (%s)...", arguments[0]);
	
	pid = fork();
	
	if( pid < 0 )
	{
		syslog(SYSLOG_INFO,"INIT: unable to fork process: error code %d.", errno);
		return 0;
	} else if( pid != 0 ){
		syslog(SYSLOG_INFO,"INIT: forked process.");
		monitor_shell(pid, arguments, environ);
	} else {
		result = execve(arguments[0], arguments, environ);
		syslog(SYSLOG_INFO,"INIT: error: unable to execute shell: error code %d.", errno);
	}
	
	return 0;
}