#include "kernel.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"
#include "kmem.h"
#include "fs.h"
#include "task.h"
#include "multiboot.h"

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

void multitasking_entry( void );

int kmain( multiboot_info_t* mb );
int kmain( multiboot_info_t* mb )
{
	//clear_screen();
	initialize_descriptor_tables();
	asm volatile("sti");
	init_timer(1000); // init the timer at 1000hz
	printk("initializing paging... ");
	init_paging(mb); // initialize the page tables and enable paging
	printk(" done.\n");
	
	char cpu_vendor[16] = {0};
	u32 max_code = cpuid_string(CPUID_GETVENDORSTRING, cpu_vendor);
	
	printk("CPU Vendor String: %s (maximum supported cpuid code: %d)\n", &cpu_vendor[0], max_code);
	
	
	/*printk("Registering timer callback for the next second... ");
	int result = timer_callback(timer_get_ticks()+timer_get_freq(), my_timer_callback);
	printk("done (result=%d)\n", result);*/
	
	printk("Initializing virtual filesystem... ");
	initialize_filesystem();
	printk("done.\n");
	
	printk("Initializing multitasking subsystem...\n");
	task_init();
	
	
	printk("init: forking init task... ");
	
	pid_t pid = sys_fork();
	
	if( pid == 0 ){
		multitasking_entry();
		// this should never happen
		sys_exit(-1);
	}
	
	while(1);
	
	return (int)0xdeadbeaf;
}

void multitasking_entry( void )
{
	int status = 0;
	pid_t pid = 0;
	
	printk("done.\n");
	//while(1);
	
	pid = sys_fork();
	//while(1);
	if( pid != 0 )
	{
		printk("parent: waiting for child to exit...\n");
		sys_waitpid(-1, &status, 0);
		printk("parent: child exited with result %i\n", status);
		printk("parent: exiting.\n");
		sys_exit(0);
	} else {
		printk("child: waiting for five seconds...\n");
		tick_t child_end = timer_get_ticks()+5*timer_get_freq();
		while(child_end > timer_get_ticks());
		printk("child: exiting.\n");
		sys_exit(42);
	}
	
}