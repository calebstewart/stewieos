#include <stewieos/exec.h> // For the module definitions
#include <stewieos/kernel.h> // For printk
#include <stewieos/fs.h> // For chrdev
#include <stewieos/error.h> // For error values E*
#include <fcntl.h>
// These need to be created. They should each handle the monitor and keyboard respectfully!
// monitor.h should provide:
//	int monitor_init(module_t* module);
//	int monitor_quit(module_t* module);
// 	int monitor_putchar(char c)
// 	int monitor_setattr(char attrib);
//	int monitor_clear();
//	int monitor_moveto(int row, int col);
// keyboard.h should provide:
// 	int keyboard_init(module_t* module, tty_driver_t* driver);
// NOTE: The keyboard init module should setup the keyboard interrupt and fill characters into the device
// 		upon interrupts immediately after setup. VTTY does not call keyboard functions directly.
// 		The TTY layer will simply wait for the keyboard layer to fill in bytes into the buffer through
// 		interrupts.
#include "monitor.h"
#include "keyboard.h"
#include "vtty.h"

// Module Functions
int vtty_load(module_t* module);
int vtty_remove(module_t* module);
// TTY Functions
int vtty_putchar(tty_device_t* tty, char c);
int vtty_write(tty_device_t* tty, const char* buffer, size_t len);
int vtty_ioctl(tty_device_t* tty, int cmd, char* parm);
int vtty_open(tty_device_t* tty, int omode);
int vtty_close(tty_device_t* tty);

// TTY Operations structure
struct tty_operations vtty_ops = {
	.putchar = vtty_putchar,
	.write = vtty_write,
	.ioctl = vtty_ioctl,
	.open = vtty_open,
	.close = vtty_close,
};
// Module Information Structure
MODULE_INFO("vtty", vtty_load, vtty_remove);

int vtty_load(module_t* module __attribute__((unused)))
{
	// Allocate the tty driver structure
	tty_driver_t* driver = tty_alloc_driver("vtty", VTTY_MAJOR, VTTY_NMINORS, &vtty_ops);
	if( IS_ERR(driver) ){
		return PTR_ERR(driver);
	}
	
	// Setup the termios
	driver->device[0].termios.c_lflag |= ECHO;
	
	// Initialize the monitor
	int error = monitor_init(module);
	if( error != 0 ){
		tty_free_driver(VTTY_MAJOR);
		return error;
	}
	
	// Initialize the keyboard
	error = keyboard_init(module, &driver->device[0]);
	if( error != 0 ){
		monitor_quit(module);
		tty_free_driver(VTTY_MAJOR);
		return error;
	}
	
	// Create the device node
	error = sys_mknod("/dev/tty0", S_IFCHR, makedev(VTTY_MAJOR, 0));
	if( error != 0 && error != (-EEXIST) ){
		syslog(KERN_WARN, "unable to create vtty device node. error code %d", error);
	}
	
	return 0;
}

int vtty_remove(module_t* module __attribute__((unused)))
{
	keyboard_quit(module);
	monitor_quit(module);
	return tty_free_driver(VTTY_MAJOR);
}

int vtty_putchar(tty_device_t* tty __attribute__((unused)), char c)
{
	return monitor_putchar(c);
}

int vtty_write(tty_device_t* tty __attribute__((unused)), const char* s, size_t l)
{
	size_t n = 0;
	while(n < l){
		int res = monitor_putchar(s[n++]);
		if( res != 0 ){
			return n-1;
		}
	}
	return l;
}

int vtty_ioctl(tty_device_t* tty __attribute__((unused)), int cmd __attribute__((unused)), char* parm __attribute__((unused)))
{
	return 0;
}

int vtty_open(tty_device_t* device __attribute__((unused)), int omode __attribute__((unused)))
{
	return 0;
}

int vtty_close(tty_device_t* device __attribute__((unused)))
{
	return 0;
}