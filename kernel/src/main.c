#include "kernel.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"
#include "kmem.h"
#include "fs.h"
#include "task.h"
#include "multiboot.h"
#include <sys/fcntl.h>
#include "sys/mount.h"
#include "elf/elf32.h"
#include "error.h"
#include "sys/stat.h"
#include "sys/types.h"
#include <unistd.h>
#include "exec.h"

// testing spinlock
#include "spinlock.h"

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

void multitasking_entry( multiboot_info_t* mb);
int initfs_install(multiboot_info_t* mb);

int kmain( multiboot_info_t* mb );
int kmain( multiboot_info_t* mb )
{
	//clear_screen();
	initialize_descriptor_tables();
	asm volatile("sti");
	init_timer(1000); // init the timer at 1000hz
	printk("initializing paging... ");
	unsigned int curpos = get_cursor_pos();
	printk("\n");
	init_paging(mb); // initialize the page tables and enable paging
	printk_at(curpos, " done.\n");

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
		multitasking_entry(mb);
		// this should never happen
		sys_exit(-1);
	}
	
	while(1);
	
	return (int)0xdeadbeaf;
}

void multitasking_entry( multiboot_info_t* mb )
{
	printk("done.\n");
	
	initfs_install(mb);
	
	printk("INIT: loading module 'test_mod.o'\n");
	int error = sys_insmod("/test_mod.o");
	if( error != 0 ){
		printk("INIT: unable to load module: %d\n", error);
	} else {
		printk("INIT: removing module 'test_mod'\n");
		error = sys_rmmod("test_mod");
		if( error != 0 ){
			printk("INIT: unable to remove module: %d\n", error);
		}
	}
	
	printk("INIT: Finished.\n");
	
	return;
	
}