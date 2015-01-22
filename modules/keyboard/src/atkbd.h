#ifndef _ATKBD_H_
#define _ATKBD_H_

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

#endif