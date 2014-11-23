#ifndef _VTTY_MONITOR_H_
#define _VTTY_MONITOR_H_

#include <exec.h>
#include <kernel.h>

int monitor_init(module_t* module);
int monitor_quit(module_t* module);
int monitor_putchar(char c);
int monitor_setattr(unsigned char attrib);
int monitor_clear( void );
int monitor_moveto(int row, int col);

#endif