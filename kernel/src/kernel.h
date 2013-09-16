#ifndef _KERNEL_H_
#define _KERNEL_H_

extern int (*printk)(const char* format, ...);

int initial_printk(const char* format, ...);

void outb(unsigned short port, unsigned char v);
unsigned char inb(unsigned short port);
unsigned short inw(unsigned short port);

#endif
