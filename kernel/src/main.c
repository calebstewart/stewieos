#include "kernel.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"
#include "kmem.h"
#include "fs.h"
#include "task.h"

tick_t my_timer_callback(tick_t, struct regs*);
tick_t my_timer_callback(tick_t time, struct regs* regs)
{
	UNUSED(regs);
	printk("timer_callback: time: %d+%d/1000\n", (time / 1000), time%1000);
	return time+timer_get_freq();
}

// defined in initial_printk.c
//void clear_screen(void);

int global_var = 0;

int main( void )
{
	//clear_screen();
	initialize_descriptor_tables();
	asm volatile("sti");
	init_timer(1000); // init the timer at 1000hz
	printk("initializing paging... ");
	init_paging(); // initialize the page tables and enable paging
	printk(" done.\n");
	
	char cpu_vendor[16] = {0};
	u32 max_code = cpuid_string(CPUID_GETVENDORSTRING, cpu_vendor);
	
	printk("CPU Vendor String: %s (maximum supported cpuid code: %d)\n", &cpu_vendor[0], max_code);
	
	
	/*printk("Registering timer callback for the next second... ");
	int result = timer_callback(timer_get_ticks()+timer_get_freq(), my_timer_callback);
	printk("done (result=%d)\n", result);*/
	
	printk("Initializing virtual filesystem...\n");
	initialize_filesystem();
	printk("done.\n");
	
	printk("Initializing multitasking subsystem...\n");
	task_init();
	
	printk("Forking...\n");
	
	pid_t pid = sys_fork();
	
	if( pid != 0 )
	{
		printk("I'm the parent!\n");
		global_var = 42;
	} else {
		printk("I'm the child!\nglobal_var = %d\n", global_var);
		sys_exit(-1);
	}
	
	printk("Let's try and kill ourselves...\n");
	sys_exit(-1);
	
	while(1);
	
	return ((int)0xDEADBEAF);
} 