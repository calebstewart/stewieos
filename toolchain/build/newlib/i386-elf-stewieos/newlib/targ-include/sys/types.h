#ifndef _STEWIEOS_SYS_TYPES_
#define _STEWIEOS_SYS_TYPES_

// This is the default newlib include file. we just need to add a few things
#include <sys/_default_types.h>

// Put major in the high 16 bits and minor in the low 16 bits
#define makedev(maj, min) ((dev_t)((((int)(maj)) << 8) | (((int)(min)) & 0xFFFF)))
#define major(dev) ((unsigned int)( ((dev) >> 8) & 0xFF ))
#define minor(dev) ((unsigned int)( (dev) & 0xFF ))


#endif