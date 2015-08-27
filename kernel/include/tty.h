#ifndef _TTY_H_
#define _TTY_H_

#include "kernel.h"
#include "task.h"
#include "spinlock.h"
#include <termios.h>

#define TTY_QUEUELEN 1024
#define TTY_NQUEUE 2

struct _tty_device;
typedef struct _tty_device tty_device_t;
struct _tty_driver;
typedef struct _tty_driver tty_driver_t;

struct tty_operations
{
	int(*putchar)(tty_device_t* tty, char c);
	int(*write)(tty_device_t* tty, const char* buffer, size_t len);
	int(*ioctl)(tty_device_t* tty, int cmd, char* parm);
	int(*open)(tty_device_t* tty, int omode);
	int(*close)(tty_device_t* tty);
};

typedef struct _tty_queue
{
	char* read;
	char* write;
	char queue[TTY_QUEUELEN];
} tty_queue_t;

struct _tty_device
{
	dev_t devid; // the device id for this tty
	tty_queue_t queue;
	struct tty_operations* ops; // the tty operations
	struct task* task; // the process which currently has control over this TTY
	spinlock_t lock; // lock for reading and writing
	struct termios termios; // terminal input/output flags
	unsigned int refs;
};

struct _tty_driver
{
	char name[32];
	const char* devname;
	unsigned int major;
	unsigned int nminors;
	void* private;
	
	int(*open)(tty_driver_t* driver, tty_device_t* device);
	int(*close)(tty_driver_t* driver, tty_device_t* device);
	
	tty_device_t device[];
};

/* External Interfaces (Forward Facing) */
tty_device_t* tty_find_device(dev_t device);
tty_driver_t* tty_find_driver(dev_t device);
void tty_device_insert(tty_device_t* device, char c);
char tty_device_remove(tty_device_t* device);
ssize_t tty_device_read(tty_device_t* device, char* buffer, size_t count);


int tty_queue_empty(tty_device_t* device); // check if the queue is empty 
void tty_queue_insert(tty_device_t* device, char c); // insert a character to be read
char tty_queue_remove(tty_device_t* device); // remove the last inserted character
char tty_queue_getchar(tty_device_t* device); // get the next character to be read
ssize_t tty_queue_read(tty_device_t* device, char* buffer, size_t count); // read up to count characters, and return how many were read
char tty_queue_peek(tty_device_t* device);

void tty_input_char(tty_device_t* device, char input);
void tty_input_pop(tty_device_t* device);

void tty_read(tty_device_t* device, const char* buffer, size_t len);
void tty_write(tty_device_t* device, const char* buffer, size_t len);
void tty_putchar(tty_device_t* device, char c);

int tty_end_of_line(tty_device_t* device);

void tty_ioctl(tty_device_t* device, int cmd, char* parm);

tty_driver_t* tty_alloc_driver(const char* name, unsigned int major, unsigned int nminors, struct tty_operations* ops);
int tty_free_driver(unsigned int major);
tty_driver_t* tty_find_driver(unsigned int major);

#endif
