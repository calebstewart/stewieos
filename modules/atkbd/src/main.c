#include "exec.h" // For the module definitions
#include "kernel.h" // For printk
#include "atkbd.h" // AT Keyboard Definitions
#include "ps2.h"
#include <error.h>

// Forward Declarations
int keyboard_load(module_t* module);
int keyboard_remove(module_t* module);

// This tells the operating system the module name, and the
// load and remove functions to be called at respective times.
MODULE_INFO("keyboard", keyboard_load, keyboard_remove);

// Static Global Declarations
static ps2_listener_t* kbd_listener = NULL;

// This function will be called during module insertion
// The return value should be zero for success. All other
// values are passed back from insmod.
int keyboard_load(module_t* module ATTR((unused)))
{
	u8 kbd_port = 0;
	u16 port_id = ps2_identify(PS2_PORT1);
	if( PS2_IS_KBD(port_id) ){
		kbd_port = PS2_PORT1;
	} else {
		syslog(KERN_ERR, "atkbd: first port id: %x\n", port_id);
		port_id = ps2_identify(PS2_PORT2);
		if( !PS2_IS_KBD(port_id) ){
			syslog(KERN_ERR, "atkbd: no ps/2 keyboard devices detected! %x", port_id);
			return -ENODEV;
		}
		kbd_port = PS2_PORT2;
	}
	//ps2_disable(kbd_port); // disable the device
	ps2_config(0, PS2_CFG_INT(kbd_port)); // disable interrupts for that device
	// Turn off all LEDs
// 	ps2_send_data(kbd_port, 0xED);
// 	ps2_send_data(kbd_port, 0x00);
// 	ps2_read_data( );
	// Set the scancode set to Scancode Set 2
	ps2_send_data(kbd_port, 0xF0);
	ps2_read_data( );
	ps2_send_data(kbd_port, 0x01);
	// Re-enable interrupts
	ps2_config(PS2_CFG_INT(kbd_port), 0);
	ps2_enable(kbd_port);
	// Register a listener for the ps/2 port
	kbd_listener = ps2_listen(kbd_port, kbd_event);
	
	if( kbd_listener == NULL ){
		syslog(KERN_ERR, "atkbd: unable to listen on ps/2 port %d.", kbd_port);
		return -EIO;
	}
		
	syslog(KERN_ERR, "atkbd: found ps/2 keyboard. listening on port %d.", kbd_port);
	
	return 0;
}

// This function will be called during module removal.
// It is only called when module->m_refs == 0, therefore
// you don't need to worry about reference counting. Just
// free any data you allocated, and return. Any non-zero
// return value will be passed back from rmmod
int keyboard_remove(module_t* module ATTR((unused)))
{
	ps2_unlisten(kbd_listener);
	printk("atkbd: module removed successfully.\n");
	return 0;
}