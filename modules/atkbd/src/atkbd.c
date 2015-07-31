#include "kernel.h"
#include "atkbd.h"
#include <error.h>
#include <ps2.h>
#include <event.h>

static u8 atkbd_state = 0;
static u32 atkbd_multibyte = 0;

vkey_t single_scancode_map[0x84] = {
	[0x01] = KEY_F9, [0x03] = KEY_F5, [0x04] = KEY_F3, [0x05] = KEY_F1, [0x06] = KEY_F2,
	[0x07] = KEY_F12, [0x09] = KEY_F10, [0x0A] = KEY_F8, [0x0B] = KEY_F6, [0x0C] = KEY_F4,
	[0x0D] = KEY_TAB, [0x0E] = KEY_BACKTICK, [0x11] = KEY_LEFTALT, [0x12] = KEY_LEFTSHIFT,
	[0x14] = KEY_LEFTCTRL, [0x15] = KEY_Q, [0x16] = KEY_1, [0x1A] = KEY_Z, [0x1b] = KEY_S,
	[0x1C] = KEY_A, [0x1D] = KEY_W, [0x1E] = KEY_2, [0x21] = KEY_C, [0x22] = KEY_X,
	[0x23] = KEY_D, [0x24] = KEY_E, [0x25] = KEY_4, [0x26] = KEY_3, [0x29] = KEY_SPACE,
	[0x2A] = KEY_V, [0x2B] = KEY_F, [0x2C] = KEY_T, [0x2D] = KEY_R, [0x2E] = KEY_5,
	[0x31] = KEY_N, [0x32] = KEY_B, [0x33] = KEY_H, [0x34] = KEY_G, [0x35] = KEY_Y,
	[0x36] = KEY_6, [0x3A] = KEY_M, [0x3B] = KEY_J, [0x3C] = KEY_U, [0x3D] = KEY_7,
	[0x3E] = KEY_8, [0x41] = KEY_COMMA, [0x42] = KEY_K, [0x43] = KEY_I, [0x44] = KEY_O,
	[0x45] = KEY_0, [0x46] = KEY_9, [0x49] = KEY_PERIOD, [0x4A] = KEY_SLASH, [0x4B] = KEY_L,
	[0x4C] = KEY_SEMICOLON, [0x4D] = KEY_P, [0x4E] = KEY_MINUS, [0x52] = KEY_SINGLEQUOTE,
	[0x54] = KEY_LEFTBRACKET, [0x55] = KEY_EQUAL, [0x58] = KEY_CAPSLOCK,
	[0x59] = KEY_RIGHTSHIFT, [0x5A] = KEY_ENTER, [0x5B] = KEY_LEFTBRACKET, [0x5D] = KEY_BACKSLASH,
	[0x66] = KEY_BACKSPACE, [0x69] = KEY_K1, [0x6B] = KEY_K4, [0x6C] = KEY_K7,
	[0x70] = KEY_K0, [0x71] = KEY_KPERIOD, [0x72] = KEY_K2, [0x73] = KEY_K5, [0x74] = KEY_K6,
	[0x75] = KEY_K8, [0x76] = KEY_ESC, [0x77] = KEY_NUMLOCK, [0x78] = KEY_F11,
	[0x79] = KEY_KPLUS, [0x7A] = KEY_K3, [0x7B] = KEY_KMINUS, [0x7C] = KEY_KASTERISK,
	[0x7D] = KEY_K9, [0x7E] = KEY_SCRLOCK, [0x83] = KEY_F7
};

vkey_t multi_scancode_map[0x7E] = {
	[0x10] = KEY_WWWSEARCH, [0x11] = KEY_RIGHTALT, [0x14] = KEY_RIGHTCTRL, [0x15] = KEY_MMPREV,
	[0x18] = KEY_WWWFAV, [0x1F] = KEY_LEFTGUI, [0x20] = KEY_WWWREFRESH, [0x21] = KEY_MMVOLDOWN,
	[0x23] = KEY_MMMUTE, [0x27] = KEY_RIGHTGUI, [0x28] = KEY_WWWSTOP, [0x2B] = KEY_MMCALC,
	[0x2F] = KEY_APPS, [0x30] = KEY_WWWFORWARD, [0x32] = KEY_MMVOLUP, [0x34] = KEY_MMPAUSE,
	[0x37] = KEY_ACPIPOWER, [0x38] = KEY_WWWBACK, [0x3A] = KEY_WWWHOME, [0x3B] = KEY_MMSTOP,
	[0x40] = KEY_MMMYCOMP, [0x48] = KEY_MMEMAIL, [0x4A] = KEY_KSLASH, [0x4D] = KEY_MMNEXT,
	[0x50] = KEY_MMMEDIA, [0x5A] = KEY_KENTER, [0x5E] = KEY_ACPIWAKE, [0x69] = KEY_END,
	[0x6B] = KEY_CURSORLEFT, [0x6C] = KEY_HOME, [0x70] = KEY_INSERT, [0x71] = KEY_DELETE,
	[0x72] = KEY_CURSORDOWN, [0x74] = KEY_CURSORRIGHT, [0x75] = KEY_CURSORUP,
	[0x7A] = KEY_PAGEDOWN, [0x7C] = KEY_PRINTSCREEN, [0x7D] = KEY_PAGEUP
};

vkey_t* scancode_map[2] = { single_scancode_map, multi_scancode_map };

void kbd_event(u8 port ATTR((unused)), u8 sc)
{
	u32 scancode = sc;

	if( sc == 0xE0 ){
		atkbd_multibyte = 1;
		return;
	} else if( sc == 0xF0 ){
		atkbd_state = KEY_RELEASED;
		return;
	}
	
	// this is the first 2 scancodes of the print screen scancode (the other two are unique, so we just ignore these
	if( atkbd_multibyte && sc == 0x12 ){
		atkbd_multibyte = 0;
		atkbd_state = KEY_PRESSED;
		return;
	}
	
	// get the virtual key mapping
	vkey_t key = scancode_map[atkbd_multibyte][sc];
	
	// Print Screen always acts like it is being released
	// and has a 4-byte scancode
	if( key == KEY_PRINTSCREEN ){
		atkbd_state = KEY_RELEASED;
		scancode = 0xE012E07C;
	} else {
		// build the scancode
		scancode = sc;
		if( atkbd_multibyte && atkbd_state == KEY_PRESSED ) scancode |= 0xE000;
		if( atkbd_multibyte && atkbd_state == KEY_RELEASED ) scancode |= 0xE0F000;
		if( !atkbd_multibyte && atkbd_state == KEY_RELEASED ) scancode |= 0xF000;
	}
	
// 	// Build the key event structure
// 	key_event_t event = {
// 		.scancode = scancode,
// 		.key = key,
// 		.state = atkbd_state,
// 	};
	
	// BROADCAST!
	event_raise(ET_KEY, key, atkbd_state);
	
	// Reset the flags
	atkbd_state = KEY_PRESSED;
	atkbd_multibyte = 0;
}
