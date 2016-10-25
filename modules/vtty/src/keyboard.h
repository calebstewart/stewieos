#ifndef _VTTY_KEYBOARD_H_
#define _VTTY_KEYBOARD_H_

#include <stewieos/kernel.h>
#include <stewieos/exec.h>
#include <stewieos/tty.h>
#include <stewieos/keys.h>

#define PS2_STATUS 0x64
#define PS2_DATA 0x60
#define PS2_COMMAND 0x64

#define PS2_READ	0x20
#define PS2_WRITE	0x60
#define PS2_DISABLE2	0xA7
#define PS2_ENABLE2	0xA8
#define PS2_TEST2	0xA9
#define PS2_TEST	0xAA
#define PS2_TEST1	0xAB
#define PS2_DIAG	0xAC
#define PS2_DISABLE1	0xAD
#define PS2_ENABLE1	0xAE
#define PS2_READINPUT	0xC0
#define PS2_READOUT	0xD0
#define PS2_WRITEOUT	0xD1

#define PS2_CONFIG_INT1		(1<<1)
#define PS2_CONFIG_INT2		(1<<1)
#define PS2_CONFIG_SYSFLAG	(1<<2)
#define PS2_CONFIG_CLOCK1	(1<<4)
#define PS2_CONFIG_CLOCK2	(1<<5)
#define PS2_CONFIG_TRANS	(1<<6)

#define PS2_BUFFER1 (1<<4)
#define PS2_BUFFER2 (1<<5)

vkey_t scancode_convert(u8 sc);
char vkey_convert(vkey_t key, const char* map);
void set_vkey_map(char* std, char* caps, char* shift, char* ctrl, char* alt);

int keyboard_init(module_t* module, tty_device_t* device);
int keyboard_quit(module_t* module);

int keyboard_send_command(u8 command, u8 parameter, int retry_limit);

#endif