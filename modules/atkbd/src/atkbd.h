#ifndef _ATKBD_H_
#define _ATKBD_H_

#define ATKBD_SET_SCANCODE 0xF0
#define ATKBD_SET_LED 0xED

#define ATKBD_SCANCODE_SET2 2

void kbd_event(u8 port, u8 data);

#endif