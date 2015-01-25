#include "kernel.h"
#include "atkbd.h"
#include <error.h>
#include <ps2.h>
#include <event.h>

static u32 atkbd_scancode = 0;
static u32 atkbd_multibyte = 0;

void kbd_event(u8 port ATTR((unused)), u8 sc)
{
	if( sc == 0xE0 || sc == 0xE1 ){
		atkbd_scancode = sc;
		atkbd_multibyte = 1;
		return;
	} else if( atkbd_multibyte && (sc == 0x2A || sc == 0xB7 || sc == 0x1D || sc == 0x45 || sc == 0x9D)  ){ // these bytes mean it is a 3-byte scancode
		atkbd_scancode = (atkbd_scancode << 8) | sc;
		return;
	}
	
	// add this scancode byte to the total scancode (if not multibyte, atkbd_scancode will be zero)
	atkbd_scancode = (atkbd_scancode << 8) | (sc & 0x7f);
	// Build the key event structure
	key_event_t event = {
		.scancode = atkbd_scancode,
		.state = !(sc & 0x80) ? KEY_PRESSED : KEY_RELEASED
	};
	// Reset the multibyte scancode
	atkbd_scancode = 0;
	
	// BROADCAST!
	event_raise(ET_KEY, EVENT_SCANCODE, &event, sizeof(event));
}
