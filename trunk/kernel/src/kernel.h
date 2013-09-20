#ifndef _KERNEL_H_
#define _KERNEL_H_

#define UNUSED(var) do{ (void)var; } while(0)

typedef unsigned char	u8;
typedef signed char	s8;
typedef unsigned short	u16;
typedef signed short	s16;
typedef unsigned int	u32;
typedef signed int	s32;
typedef unsigned int	uint;
typedef unsigned int	size_t;

extern int (*printk)(const char* format, ...);

int initial_printk(const char* format, ...);

void outb(unsigned short port, unsigned char v);
unsigned char inb(unsigned short port);
unsigned short inw(unsigned short port);

size_t memset(void* ptr, size_t count, int v);
size_t memcpy(void* dst, void* src, size_t count);

#endif
