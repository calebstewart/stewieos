#ifndef _STEWIEOS_SYS_TYPES_
#define _STEWIEOS_SYS_TYPES_

// Put major in the high 16 bits and minor in the low 16 bits
#define makedev(maj, min) ((dev_t)((((int)(maj)) << 16) | (((int)(min)) & 0xFFFF)))
#define major(dev) ((unsigned int)( ((dev) >> 16) & 0xFFFF ))
#define minor(dev) ((unsigned int)( (dev) & 0xFFFF ))

#endif
