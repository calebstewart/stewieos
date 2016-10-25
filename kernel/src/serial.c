#include "stewieos/serial.h"

static serial_device_t serial_device[SERIAL_NMINORS] = {
	{
		.port = 0x3F8,
		.rlock = 0, .wlock = 0,
	},
	{
		.port = 0x2F8,
		.rlock = 0, .wlock = 0,
	},
	{
		.port = 0x3E8,
		.rlock = 0, .wlock = 0,
	},
	{
		.port = 0x2E8,
		.rlock = 0, .wlock = 0,
	}
};

void serial_init( void )
{
	for(int i = 0; i < SERIAL_NMINORS; ++i)
	{
		// disable interrupts
		outb((u16)(serial_device[i].port+1), 0);
		outb((u16)(serial_device[i].port+3), 0x80); // enable dlab
		outb((u16)(serial_device[i].port+0), 0x03); // divisor=3
		outb((u16)(serial_device[i].port+1), 0x00); // divisor high byte
		outb((u16)(serial_device[i].port+3), 0x03); // disable dlab, and enable 8N1
		outb((u16)(serial_device[i].port+2), 0xC7); // enable FIF with 14-byte threshold
	}

	return;
}

serial_device_t* serial_get_device(u32 devid)
{
	if( devid >= SERIAL_NMINORS ){
		return NULL;
	}
	
	return &serial_device[devid];
}

ssize_t serial_write(serial_device_t* device, const char* buffer, size_t buflen)
{
	spin_lock(&device->wlock);
	
	for(size_t i = 0; i < buflen; ++i){
		// wait for empty buffer (this is spin-locked so should be quick
		while(!(inb((u16)(device->port+5))&0x20));
		outb(device->port, buffer[i]);
	}
	
	spin_unlock(&device->wlock);
	return (ssize_t)buflen;
}

ssize_t serial_read(serial_device_t* device, char* buffer, size_t buflen)
{
	spin_lock(&device->rlock);
	
	for(size_t i = 0; i < buflen; ++i)
	{
		while( !(inb((u16)(device->port+5))&1) );
		buffer[i] = inb(device->port);
		if( buffer[i] == '\n' ){
			spin_unlock(&device->rlock);
			return (ssize_t)i;
		}
	}
	
	spin_unlock(&device->rlock);
	
	return (ssize_t)buflen;
}