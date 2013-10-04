#include "kernel.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"
#include "kmem.h"

tick_t my_timer_callback(tick_t, struct regs*);
tick_t my_timer_callback(tick_t time, struct regs* regs)
{
	UNUSED(regs);
	printk("timer_callback: time: %d+%d/1000\n", (time / 1000), time%1000);
	return time+timer_get_freq();
}

// defined in initial_printk.c
//void clear_screen(void);

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
	
	
	printk("Registering timer callback for the next second... ");
	int result = timer_callback(timer_get_ticks()+timer_get_freq(), my_timer_callback);
	printk("done (result=%d)\n", result);
	
	//init_kheap(0xE0000000, 0xE0010000, 0xF0000000);
	
	void* p1 = kmalloc(15);
	u32 p2_phys = 0;
	printk("p1=%p\n", p1);
	void* p2 = kmalloc_p(1, &p2_phys);
	printk("p2=%p\np2_phys=0x%X\n", p2, p2_phys);
	void* p3 = kmalloc_a(0x1000);
	
	printk("p3=%p\n", p3);
	
	kfree(p3);
	kfree(p2);
	kfree(p1);
	
	return ((int)0xDEADBEAF);
} 