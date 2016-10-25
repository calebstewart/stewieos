#ifndef _KERNEL_PS2_H_
#define _KERNEL_PS2_H_

#include <stewieos/kernel.h>
#include <stewieos/linkedlist.h>

#define PS2_DATA 0x60
#define PS2_STAT 0x64
#define PS2_CMD  0x64

// PS2 Acknowledge response
#define PS2_ACK			0xFA
#define PS2_RESEND		0xFE

// Status Register Bits
#define PS2_STAT_OUTPUT		(1<<0) /* Is PS2_DATA full and ready to be read from? */
#define PS2_STAT_INPUT		(1<<1) /* Are PS2_DATA and PS2_CMD clear and ready to be written to? */
#define PS2_STAT_SYSTEM		(1<<2) /* Basically, set if the system is running... */
#define PS2_STAT_MODE_CMD	(1<<3) /* Reset for data write, Set for controller command write */
#define PS2_STAT_MODE_DAT	(0)    /* Don't set the 3rd bit for data mode */
#define PS2_STAT_SYS0		(1<<4) /* System Specific (Unknown, probably unused) */
#define PS2_STAT_SYS1		(1<<5) /* System Specific (Unknown/Unreliable) */
#define PS2_STAT_TIMEOUT	(1<<6) /* Reset for no error, Set for timeout error */
#define PS2_STAT_PARITY		(1<<7) /* Reset for no error, Set for parity error */

// PS/2 Controller Configuration Byte
#define PS2_CFG_INT1		(1<<0)	/* First PS/2 Port Interrupt */
#define PS2_CFG_INT2		(1<<1)	/* Second PS/2 Port Interrupt */
#define PS2_CFG_SYS		(1<<2)	/* Indicates if your system is running/Passed POST test */
#define PS2_CFG_ZER0		(1<<3)	/* Should be zero */
#define PS2_CFG_DIS1		(1<<4)	/* First PS/2 Port Clock */
#define PS2_CFG_DIS2		(1<<5)	/* Second PS/2 Port Clock */
#define PS2_CFG_TRN2		(1<<6)	/* First PS/2 Port Translation */
#define PS2_CFG_ZER1		(1<<7)	/* Must be zero */

#define PS2_CFG_INT(p)		( (p) == PS2_PORT1 ? PS2_CFG_INT1 : ( (p) == PS2_PORT2 ? PS2_CFG_INT2 : 0 ) )

// PS/2 Controller Output Port
#define PS2_OUT_RST			(1<<0)	/* Always '1', pulse using PS2_CMD_RST */
#define PS2_OUT_A20			(1<<1)	/* A20 Gate */
#define PS2_OUT_CLK2		(1<<2)	/* Second PS/2 Port Clock */
#define PS2_OUT_DAT2		(1<<2)	/* Second PS/2 Port Data */
#define PS2_OUT_FUL1		(1<<3)	/* Output Buffer Full From PS/2 Port 1 */
#define PS2_OUT_FUL2		(1<<4)	/* Output Buffer Full From PS/2 Port 2 */
#define PS2_OUT_CLK1		(1<<5)	/* First PS/2 Port Clock */
#define PS2_OUT_DAT1		(1<<6)	/* First PS/2 Port Data */

// PS/2 Identify Results
#define PS2_DEV_MOUSE		0x0000	/* Standard PS/2 Mouse */
#define PS2_DEV_SCRLMOUSE	0x0300	/* Mouse with scroll wheel */
#define PS2_DEV_5BTNMOUSE	0x0400	/* 5-button Mouse */
#define PS2_DEV_TRKEYBOARD1	0xAB41	/* MF2 Keyboard With Controller Translation */
#define PS2_DEV_TRKEYBOARD2	0xABC1	/* Same as above */
#define PS2_DEV_KEYBOARD	0xAB83	/* MF2 Keyboard */
#define PS2_DEV_NONE		0xFFFF	/* Either no device is attached or there was an input error */

#define PS2_IS_KBD(id)		(id == PS2_DEV_TRKEYBOARD1 || id == PS2_DEV_TRKEYBOARD2 \
					|| id == PS2_DEV_KEYBOARD )
#define PS2_IS_MOUSE(id)	(id == PS2_DEV_MOUSE || id == PS2_DEV_SCRLMOUSE \
					|| id == PS2_DEV_5BTNMOUSE )


// PS2 Controller Commands
#define PS2_CMD_READ		0x20	/* Read byte N from internal RAM (configuration byte) */
#define PS2_CMD_RDCFG		0x20	/* Same as PS2_CMD_READ */
#define PS2_CMD_WRITE		0x60	/* Write to byte N of internal RAM */
#define PS2_CMD_WRCFG		0x60	/* Same as PS2_CMD_WRITE */
#define PS2_CMD_DIS2		0xA7	/* Disable Second PS/2 Port */
#define PS2_CMD_ENA2		0xA8	/* Enable Second PS/2 Port */
#define PS2_CMD_TES2		0xA9	/* Test Second PS/2 Port */
#define PS2_CMD_TEST		0xAA	/* Test PS/2 Controller */
#define PS2_CMD_TES1		0xAB	/* Test First PS/2 Port */
#define PS2_CMD_DIAG		0xAC	/* Dumb all bytes from internal RAM (unknown response) */
#define PS2_CMD_DIS1		0xAD	/* Disable First PS/2 Port */
#define PS2_CMD_ENA1		0xAE	/* Enable First PS/2 Port */
#define PS2_CMD_RDCT		0xC0	/* Read Controller Input (No Defined Usage) */
#define PS2_CMD_CPLW		0xC1	/* Copy bits 0-3 of Input to Status Bits 4-7 */
#define PS2_CMD_CPHI		0xC2	/* Copy bits 4-7 of Input to Status Bits 4-7 */
#define PS2_CMD_RDOUT		0xD0	/* Read Controller Output Port */
#define PS2_CMD_WROUT		0xD1	/* Write to Controller Output Port */
#define PS2_CMD_WROUT1		0xD2	/* Write to First PS/2 Output Buffer */
#define PS2_CMD_WROUT2		0xD3	/* Write to Second PS/2 Output Buffer */
#define PS2_CMD_WRINP2		0xD4	/* Write to Second PS/2 Input Buffer */
#define PS2_CMD_RST		0xFE	/* Reset the machine */

// PS2 Port Identifiers
#define PS2_CTRL		0x00
#define PS2_PORT1		0x01
#define PS2_PORT2		0x02

// PS/2 Self Tests
#define PS2_TEST_CTRL		0x00	/* Test the controller */
#define PS2_TEST_PORT1		0x01	/* Test Port 1 */
#define PS2_TEST_PORT2		0x02	/* Test Port 2 */
#define PS2_TEST_ALL		0x03	/* Perform all 3 tests */

// PS/2 Self Test Results
#define PS2_RESULT_PASS		0x00	/* All tests passed */
#define PS2_RESULT_TIMEOUT	0x21	/* Polling reached a timeout */
#define PS2_RESULT_FAIL		0x20	/* PS2 Reset Failed */
#define PS2_RESULT_CLK1		0x01	/* Clock line stuck on port 1 */
#define PS2_RESULT_DAT1		0x02	/* Data line stuck on port 1 */
#define PS2_RESULT_CLK2		0x11	/* Clock line stuck on port 2 */
#define PS2_RESULT_DAT2		0x12	/* Data line stuck on port 2 */
#define PS2_RESULT_CTRL		0xFC	/* Controller Test Failed */

#define PS2_BLOCKING		1
#define PS2_NONBLOCKING		0

#define PS2_INPUT_BUFFER	PS2_STAT_INPUT
#define PS2_OUTPUT_BUFFER	PS2_STAT_OUTPUT

// PS/2 Generic Device Commands (sent to the data port)
#define PS2_DEVCMD_RESET	0xFF	/* Reset the device */

typedef void(*ps2_device_listener_t)(u8 port, u8 data);
typedef struct _ps2_listener
{
	ps2_device_listener_t listener;
	list_t entry;
} ps2_listener_t;

#define ps2_status() (inb(PS2_STAT))

// Enables or disables the given configuration parameters (0 means unchanged)
// If the same bits in enable and disable are specified, enabled takes precidence
// Lastly, return the new configuration
u8 ps2_config(u8 enable, u8 disable);
// Send a command to the PS/2 Controller
void ps2_send_cmd(u8 command);
// Send data to a specific PS/2 Device
void ps2_send_data(u8 port, u8 data);
// Read from the output buffer
u8 ps2_read_data( void );
// Read from the output buffer (don't wait for data)
// Retrieve the number of PS/2 Devices Connected
u8 ps2_device_count( void );
// Disable a PS/2 Device
void ps2_disable(u8 ports);
// Enable a PS/2 Device
void ps2_enable(u8 ports);
// Read from output buffer
u8 ps2_recieve( void );
// Perform Self Tests
u8 ps2_self_test( u8 tests );
// Reset Device
u8 ps2_reset( u8 port );
// Initialize the PS/2 Controller
void ps2_init( void );
// Poll until the given buffer is empty
u8 ps2_poll(u8 buffer);
// Retrieve the identification from the PS/2 Device
u16 ps2_identify(u8 port);

ps2_listener_t* ps2_listen(u8 port, ps2_device_listener_t listener);
void ps2_unlisten(ps2_listener_t* listener);

#endif