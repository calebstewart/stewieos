#ifndef _SYSLOG_H_
#define _SYSLOG_H_

#define SYSLOG_BUFFER_LENGTH 1024

#define SYSLOG_IDLE		0x00		/* Syslog daemon is idle */
#define SYSLOG_FLUSH		0x01		/* Syslog daemon is writing the syslog to disk */
#define SYSLOG_SHUTDOWN		0x02		/* Syslog daemon is writing the syslog to disk then exiting */

#define SYSLOG_LEVEL_COUNT	4		/* number of KERN_* identifiers */
#define KERN_NOTIFY		0x00		/* Notify the system admin of some condition, not time sensitive */
#define KERN_WARN		0x01		/* Warn the system admin of some condition, important, but not time sensitive */
#define KERN_ERR		0x02		/* An error occurred in the system, finish current operation, but display ASAP */
#define KERN_PANIC		0x03		/* An unrecoverable error occurred. Display the message, and do not continue executing. */

/* function: syslog
 * parameters:
 * 	level - One of the KERN_* constants proportionate to the problem/message
 * 	format - printf style format message
 * 	... - arguments for the printf style formatting
 * purpose:
 * 	This function will log a message to the system log. The system log is a buffer in memory which is SYSLOG_LENGTH bytes
 * 	long. The message will be logged immediately, and flushed to the syslog output device when the buffer becomes full.
 * 	However, the buffer may be flushed earlier depending on the warning level.
 * 	
 * 	For KERN_NOTIFY/KERN_WARN, the syslog daemon will flush normally. When the buffer becomes full, your message will be output.
 * 
 * 	For KERN_ERR/KERN_PANIC, the syslog daemon will flush when the syslog daemon is switched to again (should be next clock tick
 * 	as this function releases the current tasks timeslice).
 * 
 * 	The warning levels also indicate the format of the message to be printed as follows:
 * 
 * 	KERN_NOTIFY: a simply unix timestamp will be output like so: [TIME_STAMP] formatted message
 * 	KERN_WARN: the unix timestamp, plus the word "WARNING" like so: [TIME_STAMP] WARNING: formatted message
 * 	KERN_ERR: same as warn, except ERROR will be printed instead of warning like so: [TIME_STAMP] ERROR: formatted message
 * 	KERN_PANIC: same as err, except KERNEL PANIC will be printed instead of ERROR like so: [TIME_STAMP] KERNEL PANIC: formatted message
 * 
 */
void syslog(int level, const char* format, ...);

// This is identical to syslog, but does not provide the newline character
void syslog_printf(const char* fmt, ...);

/* Start the syslog daemon.
 * 	This function forks and starts a syslog daemon which monitors the kernel system log and will flush
 * 	the log to the given device or regular file whenever the buffer is filled or special messages are
 * 	output to the log. The log will also be flushed when the process is killed. 
 */
int begin_syslog_daemon(const char* syslog_device);

extern pid_t syslog_pid;

#endif