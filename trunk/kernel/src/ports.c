#include "kernel.h"

// Write a byte out to the specified port.
void outb(unsigned short port, unsigned char value)
{
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}
unsigned char inb(unsigned short port)
{
   unsigned char ret;
   asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}

unsigned short inw(unsigned short port)
{
   unsigned short ret;
   asm volatile ("inw %1, %0" : "=a" (ret) : "d" (port));
   return ret;
}
void outw(unsigned short port, unsigned short value)
{
    asm volatile ("outw %1, %0" : : "d" (port), "a" (value));
}

void outl(unsigned short port, unsigned long value)
{
    asm volatile ("outl %1, %0" : : "d" (port), "a" (value));
}
unsigned long inl(unsigned short port)
{
   unsigned long ret;
   asm volatile("inl %1, %0" : "=a" (ret) : "d" (port));
   return ret;
}