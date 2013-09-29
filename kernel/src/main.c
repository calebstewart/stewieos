#include "kernel.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"

tick_t my_timer_callback(tick_t, struct regs*);
tick_t my_timer_callback(tick_t time, struct regs* regs)
{
	printk("timer_callback: time: %d+%d/1000\n", (time / 1000), time%1000);
	return time+timer_get_freq();
}

int main( void )
{
	initialize_descriptor_tables();
	asm volatile("sti");
	init_timer(1000); // init the timer at 1000hz
	printk("initializing paging... ");
	init_paging(); // initialize the page tables and enable paging
	printk(" done.\n");
	
	char cpu_vendor[16] = {0};
	u32 max_code = cpuid_string(CPUID_GETVENDORSTRING, cpu_vendor);
	
	printk("CPU Vendor String: %s (maximum supported cpuid code: %d)\n", &cpu_vendor[0], max_code);
	
	
	printk("Registering timer callback for the next second... ");
	int result = timer_callback(timer_get_ticks()+timer_get_freq(), my_timer_callback);
	printk("done (result=%d)\n", result);
	
	*((u32*)0xA0000000) = 0xDEADBEAF;
	
	return ((int)0xDEADBEAF);
} 