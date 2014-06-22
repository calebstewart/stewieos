#include "exec.h"
#include "kernel.h"
#include "timer.h"

// Forward Declarations
int test_load(module_t* module);
int test_remove(module_t* module);
tick_t test_timer(tick_t ticks, struct regs* regs);

int g_should_remove = 0;
int g_has_removed = 0;

// This tells the operating system the module name, and the
// load and remove functions to be called at respective times.
MODULE_INFO("test_mod", test_load, test_remove);

// This function will be called during module insertion
// The return value should be zero for success. All other
// values are passed back from insmod.
int test_load(module_t* module)
{
	UNUSED(module);
	printk("test_mod: module loaded successfully.\n");
	timer_callback(TIMER_IN(1000), test_timer);
	
	return 0;
}

// This function will be called during module removal.
// It is only called when module->m_refs == 0, therefore
// you don't need to worry about reference counting. Just
// free any data you allocated, and return. Any non-zero
// return value will be passed back from rmmod
int test_remove(module_t* module)
{
	UNUSED(module);
	printk("test_mod: waiting for timer to cancel...\n");
	g_should_remove = 1;
	while( !g_has_removed );
	printk("test_mod: module removed successfully.\n");
	return 0;
}

/* function: test_timer
 * purpose:
 * 	Print a message every 1 second
 * parameters:
 * 	tick_t ticks - The number of elapsed ticks
 * 	struct regs* regs - The register structure (this is an interrupt routine)
 * return value:
 * 	When the time should fire again, or TIMER_CANCEL to remove the timer.
 */
tick_t test_timer(tick_t ticks, struct regs* regs)
{
	UNUSED(regs);
	if( g_should_remove ){
		printk("test_mod: timer interrupt: recieved cancel message.\n");
		g_has_removed = 1;
		return TIMER_CANCEL;
	}
	printk("test_mod: timer interrupt: ticks=%d\n", ticks);
	return TIMER_IN(1000);
}