#include "exec.h" // For the module definitions
#include "kernel.h" // For printk
#include "fs.h" // For chrdev
#include "error.h" // For error values E*
#include <fcntl.h>

// Forward Declarations
int screen_load(module_t* module);
int screen_remove(module_t* module);

int screen_open(struct file* file, struct dentry* dentry, int flags);
int screen_close(struct file* file, struct dentry* dentry);
ssize_t screen_write(struct file* file, const char* buffer, size_t size);

struct file_operations screen_ops = {
	.open = screen_open,
	.close = screen_close,
	.write = screen_write
};

// This tells the operating system the module name, and the
// load and remove functions to be called at respective times.
MODULE_INFO("screen", screen_load, screen_remove);

int screen_open(struct file* file __attribute__((unused)), struct dentry* dentry __attribute__((unused)), int flags)
{
	
	if( (flags+1) & _FREAD ) return -EACCES;
	
	return 0;
}

int screen_close(struct file* file __attribute__((unused)), struct dentry* dentry __attribute__((unused)))
{
	return 0;
}

ssize_t screen_write(struct file* file __attribute__((unused)), const char* buffer, size_t size __attribute__((unused)))
{
	printk("%s", buffer);
	return strlen(buffer);
}

// This function will be called during module insertion
// The return value should be zero for success. All other
// values are passed back from insmod.
int screen_load(module_t* module)
{
	UNUSED(module);
	
	register_chrdev(0x00, "screen", &screen_ops);
	
	printk("screen: module loaded successfully.\n");
	return 0;
}

// This function will be called during module removal.
// It is only called when module->m_refs == 0, therefore
// you don't need to worry about reference counting. Just
// free any data you allocated, and return. Any non-zero
// return value will be passed back from rmmod
int screen_remove(module_t* module)
{
	UNUSED(module);
	unregister_chrdev(0x00);
	printk("screen: module removed successfully.\n");
	return 0;
}