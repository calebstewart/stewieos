#include "keyboard.h"
#include "descriptor_tables.h"
#include <error.h>
#include <timer.h>
#include "vtty.h"
#include <ps2.h>
#include <event.h>

int kbd_listen(event_t* event, void* data);

static char keymap[KEY_COUNT] = { [0 ... KEY_COUNT-1] = KEY_RELEASED };
static char vkey_std[128] = {
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y',
	'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's', 'd', 'f',
	'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z',
	'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0,
	' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9',
	'-', '4', '5', '6', '+', '1', '2', '3', '0', '.', 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static char vkey_caps[128] = {
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'-', '=', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y',
	'U', 'I', 'O', 'P', '[', ']', '\n', 0, 'A', 'S', 'D', 'F',
	'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 0, '\\', 'Z',
	'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, '*', 0,
	' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9',
	'-', '4', '5', '6', '+', '1', '2', '3', '0', '.', 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static char vkey_shift[128] = {
	0, 0, '!', '@', '#', '$', '%', '^', '&', '&', '(', ')',
	'_', '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y',
	'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S', 'D', 'F',
	'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z',
	'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0,
	' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9',
	'-', '4', '5', '6', '+', '1', '2', '3', '0', '.', 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
}; 
static char vkey_ctrl[128] = {
	0
};
static char vkey_alt[128] = {
	0
};

char vkey_convert(vkey_t key, const char* map)
{
	if( map[KEY_LEFTSHIFT] || map[KEY_RIGHTSHIFT] ){
		return vkey_shift[key];
	} else if( map[KEY_CAPSLOCK] ){
		return vkey_caps[key];
	} else if( map[KEY_LEFTALT] || map[KEY_RIGHTALT] ){
		return vkey_alt[key];
	} else if( map[KEY_LEFTCTRL] || map[KEY_RIGHTCTRL] ){
		return vkey_ctrl[key];
	} else {
		return vkey_std[key];
	}
}

int keyboard_init(module_t* module, tty_device_t* device __attribute__((unused)))
{
	// Register an event listener for the system-wide key events
	event_handler_t* handler = event_listen(EVENT_KEY, kbd_listen, NULL);
	if( handler == NULL ){
		syslog(KERN_ERR, "unable to register key event listener for vtty!");
		return -ENODEV;
	}

	module->m_private = (void*)handler;

	return 0;
}

int keyboard_quit(module_t* module)
{
	event_unlisten((event_handler_t*)module->m_private);
	return 0;
}

int kbd_listen(event_t* event, void* data ATTR((unused)))
{
	
	if( event->ev_event == KEY_UNKNOWN ){
		return EVENT_CONTINUE;
	}
	
	if( event->ev_value == KEY_PRESSED )
	{
		char c = vkey_convert((vkey_t)event->ev_event, keymap);
		if( c != 0 )
		{
			tty_driver_t* driver = tty_find_driver(VTTY_MAJOR);
			tty_device_t* device = &driver->device[0];
			
			tty_queue_insert(device, c);
		}
	}
	
	if( event->ev_event == KEY_CAPSLOCK || event->ev_event == KEY_NUMLOCK || event->ev_event == KEY_SCRLOCK ){
		if( event->ev_value == KEY_PRESSED ){
			keymap[event->ev_event] = !keymap[event->ev_event];
		}
		u32 led = LED_CAPSLOCK;
		if( event->ev_event == KEY_NUMLOCK ) led = LED_NUMLOCK;
		event_raise(EVENT_LED, led, keymap[event->ev_event]); // raise a led event
	} else {
		keymap[event->ev_event] = event->ev_value;
	}

	return EVENT_CONTINUE;
}