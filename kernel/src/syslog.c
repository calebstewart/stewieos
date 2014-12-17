#include "kernel.h"
#include "task.h"
#include "syslog.h"
#include "fs.h"
#include "error.h"
#include "spinlock.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include "pipe.h"
#include "exec.h"

static const char* syslog_level[SYSLOG_LEVEL_COUNT] = {
	[KERN_NOTIFY] = "KERN_NOTIFY",
	[KERN_WARN] = "KERN_WARN",
	[KERN_ERR] = "KERN_ERR",
	[KERN_PANIC] = "KERN_PANIC"
};
struct file* syslog_pipe = NULL;

int begin_syslog_daemon(const char* syslog_device ATTR((unused)))
{
	u32 flags = disablei();

	// create the named pipe
	int result = sys_mknod("/dev/syslog_pipe", S_IFIFO | 0666, 0);
	if( result != 0 && result != -EEXIST ){
		printk("error: unable to create system log pipe. error code %d.\n", -result);
		return -1;
	}
	
	// open the pipe
	int pipefd = sys_open("/dev/syslog_pipe", O_WRONLY | O_TRUNC, 0);
	// grab the file handle
	syslog_pipe = current->t_vfs.v_openvect[pipefd].file;
	
	restore(flags);
	
	return 0;
}

void syslog(int level, const char* format, ...)
{
	char buffer[512];
	char buffer2[512];
	
	// make sure level is within the range
	if( level < 0 || level >= SYSLOG_LEVEL_COUNT ){
		level = 0;
	}
	
	va_list args;
	va_start(args, format);
	ee_vsprintf(buffer, format, args);
	va_end(args);
	
	
	sprintf(buffer2, "[%ld] %s: %s\n", timer_get_time(), syslog_level[level], buffer);
	
	// we normally can't write to a file from the kernel, but this is a named pipe and
	// should ever actually modify the file system. This is fine, since everything is
	// handled "in house"
	if( syslog_pipe != NULL ){
		file_write(syslog_pipe, buffer2, strlen(buffer2));
	} else {
		printk("%s\n", buffer);
	}
	
}