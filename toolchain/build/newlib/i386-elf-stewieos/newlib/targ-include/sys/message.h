#ifndef _STEWIEOS_MESSAGE_H_
#define _STEWIEOS_MESSAGE_H_

#include <sys/types.h>

#define MESG_MAX_LENGTH 512

enum message_type
{
	MESG_LORESV = 0xFFFFF000,
	MESG_QUIT,
	MESG_ANY = 0xFFFFFFFF,
	MESG_HIRESV = 0xFFFFFFFF,
};

enum _pop_flags
{
	MESG_POP_DEFAULT = 0x00000000, 
	MESG_POP_LEAVE = 0x00000001, // leave the message on the queue
	MESG_POP_WAIT = 0x00000002, // wait until a message is recieved to return
};

enum _syslog_level
{
	SYSLOG_INFO,
	SYSLOG_WARN,
	SYSLOG_ERR,
	SYSLOG_PANIC
};

typedef struct message
{
	pid_t from;
	unsigned int id;
	size_t length;
	char data[0];
} message_t;

// Allocate a message buffer to use with message_peek
message_t* message_alloc( void );
// Free a message buffer
int message_free(message_t* message);
// Pop a message from this tasks queue
int message_pop(message_t* buffer, unsigned int type_mask, unsigned int flags);
// Send a message to another process
int message_send(pid_t to, unsigned int type, const char* data, size_t length);
// Post a message to the system log
void syslog(int level, const char* message, ...);

#endif /* ifdef _STEWIEOS_MESSAGE_H_ */ 
