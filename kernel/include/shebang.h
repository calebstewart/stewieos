#ifndef _SHEBANG_H_
#define _SHEBANG_H_

#include "kernel.h"
#include "exec.h"

extern exec_type_t shebang_script_type;

int shebang_load(exec_t* exec);
int shebang_check(const char* filename, exec_t* exec);

#endif
