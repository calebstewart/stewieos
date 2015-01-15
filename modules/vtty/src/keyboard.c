#include "keyboard.h"
#include "descriptor_tables.h"
#include <error.h>
#include <timer.h>
#include "vtty.h"
#include <ps2.h>
#include <event.h>

void kbd_int(struct regs* regs);
void kbd_listen(event_t* event, void* data);
u8 kbd_recv( void );
void kbd_send(u8 data);
void ps2_cmd(u8 command);
void ps2_cmd_parm(u8 command, u8 parm);

static char keymap[128] = { 0 };
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

static vkey_t sc_to_vkey[128] = {
	0, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
	KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y,
	KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFTBRACKET, KEY_RIGHTBRACKET, KEY_ENTER, KEY_LEFTCTRL,
	KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_SINGLEQUOTE,
	KEY_BACKTICK, KEY_LEFTSHIFT, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M,
	KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_RIGHTSHIFT, KEY_ASTERISK, KEY_LEFTALT, KEY_SPACE, KEY_CAPSLOCK,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCRLOCK,
	KEY_K7, KEY_K8, KEY_K9, KEY_KMINUS, KEY_K4, KEY_K5, KEY_K6, KEY_KPLUS, KEY_K1, KEY_K2, KEY_K3, KEY_K0,
	KEY_KPERIOD, 0, 0, 0, KEY_F11, KEY_F12
};

static vkey_t multi_sc_to_vkey[256] = {
	[0x1C] = KEY_KENTER, [0x1D] = KEY_RIGHTCTRL, [0x35] = KEY_SLASH, [0x38] = KEY_RIGHTALT, [0x47] = KEY_HOME,
	[0x48] = KEY_CURSORUP, [0x49] = KEY_PAGEUP, [0x4B] = KEY_CURSORLEFT, [0x4D] = KEY_CURSORRIGHT,
	[0x4F] = KEY_END, [0x50] = KEY_CURSORDOWN, [0x51] = KEY_PAGEDOWN, [0x52] = KEY_INSERT,
	[0x53] = KEY_DELETE, [0x5B] = KEY_LEFTGUI, [0x5C] = KEY_RIGHTGUI, [0x5D] = KEY_APPS,
	[0x37] = KEY_PRINTSCREEN, [0xC5] = KEY_PAUSE
};

static char multibyte_scancode = 0;
static ps2_listener_t* ps2_listener = NULL;

vkey_t scancode_convert( u8 sc )
{
	if( sc >= 128 ){
		return KEY_UNKNOWN;
	}
	
	if( multibyte_scancode ){
		multibyte_scancode = 0;
		return multi_sc_to_vkey[sc];
	}
		
	return sc_to_vkey[sc];
}

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

int keyboard_init(module_t* module __attribute__((unused)), tty_device_t* device __attribute__((unused)))
{
// 	outb(PS2_COMMAND, 0xAD); 		// disable ps2 device 1
// 	outb(PS2_COMMAND, 0xA7); 		// disable ps2 device 2
// 	inb(PS2_DATA); 				// ignore any floating input data
// 	outb(PS2_COMMAND, 0x20); 		// Ask for the configuration byte
// 	while( !(inb(PS2_STATUS) & 1) );	// wait for the data
// 	u8 cfg = inb(PS2_DATA); 		// read the configuration byte
// 	cfg &= (u8)(~(1 | 2 | 32));			// Clear bits 0, 1, and 6 (decimal 1, 2, and 32)
// 	outb(PS2_COMMAND, 0x60);		// Ask to write the configuration byte
// 	while( (inb(PS2_STATUS) & 2) != 0 );	// Wait for the controller to be ready
// 	outb(PS2_DATA, cfg);			// Write the configuration byte
// 	outb(PS2_COMMAND, 0xAA);		// Perform controller self-test
// 	while( !(inb(PS2_STATUS) & 1) );	// Wait for the data
// 	u8 result = inb(PS2_DATA);		// Get the response
// 	if( result != 0x55 ){			// Bad self-test
// 		return -ENXIO;
// 	}
// 	outb(PS2_COMMAND, 0xAB);		// Perform First Port Test
// 	while( !(inb(PS2_STATUS) & 1) );	// Wait for response
// 	result = inb(PS2_DATA);
// 	if( result != 0 ){			// Bad self-test
// 		return -ENXIO;
// 	}
// 	outb(PS2_COMMAND, 0xAE);		// Enable first port
// 	outb(PS2_COMMAND, 0x20);		// Ask for configuration byte
// 	while( !(inb(PS2_STATUS) & 1) );	// wait for data
// 	cfg = inb(PS2_DATA);			// get the configuration data
// 	cfg |= 1;				// Enable interrupts for first port (bit 0)
// 	outb(PS2_COMMAND, 0x60);		// Write configuration byte
// 	while( (inb(PS2_STATUS) & 2) != 0 );	// Wait for controller to be ready
// 	outb(PS2_DATA, cfg);			// Write the byte
// 	
// 	register_interrupt(IRQ1, kbd_int);
	
	event_listen(ET_KEY, kbd_listen, NULL);
	//ps2_listener = ps2_listen(PS2_PORT1, kbd_listen);
	
	return 0;
}

int keyboard_quit(module_t* module __attribute__((unused)))
{
	ps2_unlisten(ps2_listener);
	//unregister_interrupt(IRQ1);
	return 0;
}

void ps2_cmd(u8 cmd)
{
	outb(PS2_COMMAND, cmd);
}

void ps2_cmd_parm(u8 cmd, u8 parm)
{
	outb(PS2_COMMAND, cmd);
	kbd_send(parm);
}

void kbd_send(u8 data)
{
	tick_t ticks = timer_get_ticks();
	while( inb(PS2_STATUS) & (1<<1) ){
		if( (timer_get_ticks() - ticks) > timer_get_freq() ){
			return;
		}
	}
	
	outb(PS2_DATA, data);
}

u8 kbd_recv( void )
{
	tick_t ticks = timer_get_ticks();
	while( inb(PS2_STATUS) & (1<<0) ){
		if( (timer_get_ticks() - ticks) > timer_get_freq() ){
			return 0;
		}
	}
	
	return inb(PS2_DATA);
}

void kbd_listen(event_t* event, void* data ATTR((unused)))
{
	// Get the key event data
	key_event_t* key = (key_event_t*)event->ev_data;
	
// 	// This is a multibyte scancode
// 	if( (key->scancode & 0xE000) || (key->scancode & 0xE00000) || (key->scancode & 0xE10000) ) {
// 		return;
// 	}
// 	
// 	// convert the scancode to a virtual key code
// 	vkey_t vk = scancode_convert((u8)(key->scancode & 0xFF));
	
	if( key->key == KEY_UNKNOWN ){
		return;
	}
	
	if( key->state == KEY_PRESSED )
	{
		char c = vkey_convert(key->key, keymap);
		if( c != 0 )
		{
			tty_driver_t* driver = tty_find_driver(VTTY_MAJOR);
			tty_device_t* device = &driver->device[0];
			
			tty_queue_insert(device, c);
		}
	}
	
	if( key->key == KEY_CAPSLOCK || key->key == KEY_NUMLOCK || key == KEY_SCRLOCK ){
		if( key->state == KEY_PRESSED ){
			keymap[key->key] = !keymap[key->key];
		}
	} else {
		keymap[key->key] = key->state;
	}
}

void kbd_int(struct regs* regs __attribute__((unused)))
{
	u8 sc = inb(PS2_DATA);
	u8 state = ((sc & 0x80) == 0); // 1 is pressed, 0 is released
	// we need to wait on the next byte
	if( sc == 0xE0 && multibyte_scancode == 0 )
	{
		multibyte_scancode = 1;
		return;
	}
	sc = sc & 0x7f;
	vkey_t vk = scancode_convert(sc);
	
	if( vk == KEY_UNKNOWN ){
		return;
	}
	
	if( state == 1 )
	{
		char c = vkey_convert(vk, keymap);
		if( c != 0 )
		{
			tty_driver_t* driver = tty_find_driver(VTTY_MAJOR);
			tty_device_t* device = &driver->device[0];
			
			tty_queue_insert(device, c);
		}
	}
	
	if( vk == KEY_CAPSLOCK || vk == KEY_NUMLOCK || vk == KEY_SCRLOCK ){
		if( state == 1 ){
			keymap[vk] = !keymap[vk];
		}
	} else {
		keymap[vk] = state;
	}
}