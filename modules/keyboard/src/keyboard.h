#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "kernel.h"

// Keyboard Responses
#define KBD_DETERR1 0x00 /* Key Detection Error */
#define KBD_PASS 0xAA /* Self Test Passed */
#define KBD_ECHO 0xEE /* Response from echo command */
#define KBD_ACK 0xFA /* Acknowledge */
#define KBD_FAIL1 0xFC /* Self Test Failed */
#define KBD_FAIL2 0xFD /* Self Test Failed */
#define KBD_RESEND /* Resend previous data */
#define KDB_DETERR2 0xFF /* Key Detection Error */

// Multibyte checks
#define KBD_DETERR(r) ((r) == KBD_DETERR1 || (r) == KBD_DETERR2)
#define KBD_FAIL(r) ((r) == KBD_FAIL1 || (r) == KBD_FAIL2)



#endif