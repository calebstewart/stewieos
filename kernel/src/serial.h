#ifndef _KERNEL_SERIAL_H_
#define _KERNEL_SERIAL_H_

#include "kernel.h"
#include "spinlock.h"

// Module/TTY parameters
#define SERIAL_MAJOR 0x01
#define SERIAL_NMINORS 0x04
#define SERIAL_NAME "serial"
#define SERIAL_BUFLEN 0x1000

// serial device abstraction
typedef struct _serial_device
{
	spinlock_t rlock, wlock;
	unsigned short port;
} serial_device_t;

// Initialization of the serial structures and also the tty user-land interface
void serial_init( void );
// get a serial device by its port number (0-3)
serial_device_t* serial_get_device(u32 devid);

/* Write to a serial port buffer. Buffer are flushed on three occasions:
	1. When a newline is encountered.
	2. When the internal buffer is full.
	3. When serial_flush is called.
*/
ssize_t serial_write(serial_device_t* device, const char* buffer, size_t buflen);
/* Read from a serial port. The serial port may already be in use for reading,
	in which case you will block (spinlock) until the other process is done
	reading.
*/
ssize_t serial_read(serial_device_t* device, char* buffer, size_t maxlen);
/* Flush the data from the internal buffer out to the device */
int serial_flush(serial_device_t* device);


#endif