#include "kernel.h"
#include "descriptor_tables.h"

int main( void )
{
	initialize_descriptor_tables();
	
	char cpu_vendor[16] = {0};
	u32 max_code = cpuid_string(CPUID_GETVENDORSTRING, cpu_vendor);
	
	printk("CPU Vendor String: %s (maximum supported cpuid code: %d)\n", &cpu_vendor[0], max_code);
	
	return ((int)0xDEADBEAF);
} 