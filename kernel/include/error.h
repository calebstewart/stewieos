#ifndef _ERROR_H_
#define _ERROR_H_

#include <errno.h>

#define MAX_ERR		1024
#define ERR_PTR(err)	((void*)(err))
#define PTR_ERR(ptr)	((int)(ptr))
#define IS_ERR(val)	((int)(val) > -MAX_ERR && (int)(val) < 0)

#endif
