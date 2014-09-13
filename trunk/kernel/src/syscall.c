#include "syscall.h"
#include <task.h>
#include <fs.h>
#include <exec.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "error.h"

DECL_SYSCALL(syscall_exit);
DECL_SYSCALL(syscall_open);
DECL_SYSCALL(syscall_close);
DECL_SYSCALL(syscall_read);
DECL_SYSCALL(syscall_write);
DECL_SYSCALL(syscall_fstat);
DECL_SYSCALL(syscall_mount);
DECL_SYSCALL(syscall_umount);
DECL_SYSCALL(syscall_fork);
DECL_SYSCALL(syscall_execve);
DECL_SYSCALL(syscall_getpid);
DECL_SYSCALL(syscall_lseek);

syscall_handler_t syscall[SYSCALL_COUNT] = {
	[SYSCALL_EXIT] = syscall_exit,
	[SYSCALL_OPEN] = syscall_open,
	[SYSCALL_CLOSE] = syscall_close,
	[SYSCALL_READ] = syscall_read,
	[SYSCALL_WRITE] = syscall_write,
	[SYSCALL_FSTAT] = syscall_fstat,
	[SYSCALL_MOUNT] = syscall_mount,
	[SYSCALL_UMOUNT] = syscall_umount,
	[SYSCALL_FORK] = syscall_fork,
	[SYSCALL_EXECVE] = syscall_execve,
	[SYSCALL_GETPID] = syscall_getpid,
	[SYSCALL_LSEEK] = syscall_lseek,
};

void syscall_handler(struct regs* regs)
{
	if( regs->eax >= SYSCALL_COUNT ){
		regs->eax = (u32)-ENOSYS;
		return;
	}
	syscall[regs->eax](regs);
}

void syscall_exit(struct regs* regs)
{
	sys_exit((int)regs->ebx);
}

void syscall_open(struct regs* regs)
{
	regs->eax = (u32)sys_open((const char*)regs->ebx, (int)regs->ecx, (mode_t)regs->edx);
}

void syscall_close(struct regs* regs)
{
	regs->eax = (u32)sys_close((int)regs->ebx);
}

void syscall_read(struct regs* regs)
{
	regs->eax = (u32)sys_read((int)regs->ebx, (char*)regs->ecx, (size_t)regs->edx);
}

void syscall_write(struct regs* regs)
{
	//printk("%s", regs->ecx);
	//regs->eax = regs->edx;
	//return;
	regs->eax = (u32)sys_write((int)regs->ebx, (const char*)regs->ecx, (size_t)regs->edx);
}

void syscall_lseek(struct regs* regs)
{
	regs->eax = (u32)sys_lseek((int)regs->ebx, (off_t)regs->ecx, (int)regs->edx);
}

void syscall_fstat(struct regs* regs)
{
	regs->eax = (u32)sys_fstat((int)regs->ebx, (struct stat*)regs->ecx);
}

void syscall_mount(struct regs* regs)
{
	regs->eax = (u32)sys_mount((const char*)regs->ebx, (const char*)regs->ecx, (const char*)regs->edx, (unsigned long)regs->edi, (void*)regs->esi);
}

void syscall_umount(struct regs* regs)
{
	regs->eax = (u32)sys_umount((const char*)regs->ebx, (unsigned int)regs->ecx);
}

void syscall_fork(struct regs* regs)
{
	regs->eax = (u32)sys_fork();
}

void syscall_execve(struct regs* regs)
{
	regs->eax = (u32)sys_execve((const char*)regs->ebx, (char**)regs->ecx, (char**)regs->edx);
}

void syscall_getpid(struct regs* regs)
{
	regs->eax = (u32)current->t_pid;
}