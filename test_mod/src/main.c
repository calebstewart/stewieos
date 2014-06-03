#include "exec.h"
#include "kernel.h"

// Forward Declarations
int test_load(module_t* module);
int test_remove(module_t* module);

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
	printk("test_mod: module removed successfully.\n");
	return 0;
}