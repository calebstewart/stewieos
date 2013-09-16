#include "kernel.h"

int main(void* multiboot)
{
	//((char*)0xB8000)[0] = ((char*)("A"))[0];
	//((char*)0xB8000)[1] = 0x0F;
	printk("Hello, World!\nHere's an integer: %#08X\n", 0xDEADBEAF);
	return 0xDEADBEAF;
}