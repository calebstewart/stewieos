#include "syscall.h"
#include <task.h>
#include <fs.h>
#include <exec.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "error.h"
#include <dirent.h>

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
DECL_SYSCALL(syscall_insmod);
DECL_SYSCALL(syscall_rmmod);
DECL_SYSCALL(syscall_sbrk);
DECL_SYSCALL(syscall_isatty);
DECL_SYSCALL(syscall_waitpid);
DECL_SYSCALL(syscall_readdir);
DECL_SYSCALL(syscall_chdir);
DECL_SYSCALL(syscall_getcwd);
DECL_SYSCALL(syscall_mknod)

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
	[SYSCALL_INSMOD] = syscall_insmod,
	[SYSCALL_RMMOD] = syscall_rmmod,
	[SYSCALL_SBRK] = syscall_sbrk,
	[SYSCALL_ISATTY] = syscall_isatty,
	[SYSCALL_WAITPID] = syscall_waitpid,
	[SYSCALL_READDIR] = syscall_readdir,
	[SYSCALL_CHDIR] = syscall_chdir,
	[SYSCALL_GETCWD] = syscall_getcwd,
	[SYSCALL_MKNOD] = syscall_mknod,
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
	//printk("%s\n", regs->ecx);
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

void syscall_insmod(struct regs* regs)
{
	regs->eax = (u32)sys_insmod((const char*)regs->ebx);
}

void syscall_rmmod(struct regs* regs)
{
	regs->eax = (u32)sys_rmmod((const char*)regs->ebx);
}

void syscall_sbrk(struct regs* regs)
{
	regs->eax = (u32)sys_sbrk((int)regs->ebx);
}

void syscall_isatty(struct regs* regs)
{
	regs->eax = (u32)sys_isatty((int)regs->ebx);
}

void syscall_waitpid(struct regs* regs)
{
	regs->eax = (u32)sys_waitpid((pid_t)regs->ebx, (int*)regs->ecx, (int)regs->edx);
}

void syscall_readdir(struct regs* regs)
{
	regs->eax = (u32)sys_readdir((int)regs->ebx, (struct dirent*)regs->ecx, (size_t)regs->edx);
}

void syscall_chdir(struct regs* regs)
{
	regs->eax = (u32)sys_chdir((const char*)regs->ebx);
}

void syscall_getcwd(struct regs* regs)
{
	regs->eax = (u32)sys_getcwd((char*)regs->ebx, (size_t)regs->ecx);
}

void syscall_mknod(struct regs* regs)
{
	regs->eax = (u32)sys_mknod((const char*)regs->ebx, (mode_t)regs->ecx, (dev_t)regs->edx);
}