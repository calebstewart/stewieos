#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/task.h>
#include <errno.h>

static ssize_t readline(int fd, char* buffer, size_t max)
{
	ssize_t n = 0;
	int result = 0;
	do
	{
		result = read(fd, buffer, 1);
		if( result == -1 ){
			if( n == 0 ) return -1;
			else return n;
		}
		n++;
	} while( *(buffer++) != '\n' && (size_t)n < max );
	return n;
}

int main(int argc, char** argv)
{
	// open and possibly create the syslog
	int log = open("/var/log/syslog", O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if( log < 0 ){
		printf("syslogd: error: unable to open syslog! error code %d.\n", errno);
		return -1;
	}
	
	// open the system log pipe for writing to the syslog from the kernel
	int pipefd = open("/dev/syslog_pipe", O_RDONLY);
	if( pipefd < 0 ){
		printf("syslogd: error: unable to open syslog pipe! error code %d.\n", errno);
		return -1;
	}
	
	// close the standard file descriptors
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	// detach from our parent and controlling terminal (creating a daemon)
	detach(getpid());
	
	char buffer[64] = {0};
	ssize_t count = 0;
	
	while( (count = readline(pipefd, buffer, 64)) >= 0 )
	{
		if( count == 0 ) continue;
		write(log, buffer, count);
	}
	
	return 0;
}
