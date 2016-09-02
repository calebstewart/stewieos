#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <sys/syscall.h>
#include "descriptor_tables.h"

typedef void(*syscall_handler_t)(struct regs* regs);

#define DECL_SYSCALL(name) void name(struct regs* regs)

void syscall_handler(struct regs* regs);

#endif