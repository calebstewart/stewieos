#include "exec.h" // For the module definitions
#include "kernel.h" // For printk

// Forward Declarations
int module_template_load(module_t* module);
int module_template_remove(module_t* module);

// This tells the operating system the module name, and the
// load and remove functions to be called at respective times.
MODULE_INFO("module_template", module_template_load, module_template_remove);

// This function will be called during module insertion
// The return value should be zero for success. All other
// values are passed back from insmod.
int module_template_load(module_t* module)
{
	UNUSED(module);
	printk("module_template: module loaded successfully.\n");
	return 0;
}

// This function will be called during module removal.
// It is only called when module->m_refs == 0, therefore
// you don't need to worry about reference counting. Just
// free any data you allocated, and return. Any non-zero
// return value will be passed back from rmmod
int module_template_remove(module_t* module)
{
	UNUSED(module);
	printk("module_template: module removed successfully.\n");
	return 0;
}