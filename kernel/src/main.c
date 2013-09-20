#include "kernel.h"
#include "descriptor_tables.h"

int main( void )
{
	printk("Installing new Global Descriptor Table... ");
	initialize_descriptor_tables();
	printk("done!\n");
	
	
	return ((int)0xDEADBEAF);
}