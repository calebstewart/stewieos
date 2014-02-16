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
	//int status = 0;
	//pid_t pid = 0;
	const char* filename = "/somefile.txt";
	
	printk("done.\n");
	//while(1);
	
	printk("INIT: mounting testfs to \"/\"...\n");
	int result = sys_mount("", "/", "testfs", MS_RDONLY, NULL);
	printk("INIT: mounting result: %i\n", result);
	
	/*
	pid = sys_fork();
	//while(1);
	if( pid != 0 )
	{
		printk("INIT: waiting for child to exit...\n");
		sys_waitpid(-1, &status, 0);
		printk("INIT: child exited with result %i\n", status);
	} else {
		printk("child: waiting for five seconds...\n");
		tick_t child_end = timer_get_ticks()+5*timer_get_freq();
		while(child_end > timer_get_ticks());
		printk("child: exiting with result %d.\n", 42);
		sys_exit(42);
	}*/
	
	printk("INIT: attempting to open file %s\n", filename);

	int fd = sys_open(filename, O_RDONLY, 0);
	
	
	if( fd >= 0 ){
		printk("INIT: successfully opened %s: file descriptor %i\n", filename, fd);
	} else {
		printk("INIT: unable to open %s: error code %i\n", filename, fd);
	}
	
	printk("INIT: attempting to unmount while file is open...\n");
	result = sys_umount("/", 0);
	if( result == 0 ){
		printk("INIT: Uh-Oh, I was able to unmount with an open file...\n");
	} else {
		printk("INIT: Good, that didn't go well (error %d).\n", result);
	}
	
	printk("INIT: closing open file descriptor...\n");
	sys_close(fd);
	
	printk("INIT: attempting to unmount the filesystem again...\n");
	result = sys_umount("/", 0);
	if( result == 0 ){
		printk("INIT: the filesystem was successfully unmounted!\n");
	} else {
		printk("INIT: filesystem could not be unmounted (error %d)\n", result);
	}
	
	printk("INIT: attempting to open a file from old mount...\n");
	fd = sys_open(filename, O_RDONLY, 0);
	if( fd >= 0 ){
		printk("INIT: successfully opened %s: file descriptor: %i\n", filename, fd);
	} else {
		printk("INIT: unable to open %s (error %d).\n", filename, fd);
	}
	
	printk("INIT: creating a new spinlock.\n");
	spinlock_t* lock = (spinlock_t*)kmalloc(sizeof(spinlock_t));
	printk("INIT: initializing the lock to `unlocked' state.\n");
	spin_init(lock);
	printk("INIT: attempting a fork.\n");
	pid_t pid = sys_fork();
	printk("INIT(%d): forked init process.\n", sys_getpid());
	
	spin_lock(lock);
	printk("INIT(%d): Process %d locked the spinlock.\n", sys_getpid(), sys_getpid());
	tick_t wait_end = timer_get_ticks()+5*timer_get_freq();
	while( wait_end > timer_get_ticks() );
	printk("INIT(%d): Process %d unlocking...\n", sys_getpid(), sys_getpid());
	spin_unlock(lock);
	
}