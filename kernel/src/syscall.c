#include "syscall.h"
#include <task.h>
#include <fs.h>
#include <exec.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stewieos/error.h"
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
DECL_SYSCALL(syscall_mknod);
DECL_SYSCALL(syscall_shutdown);
DECL_SYSCALL(syscall_detach);
DECL_SYSCALL(syscall_access);
DECL_SYSCALL(syscall_message_send);
DECL_SYSCALL(syscall_message_pop);
DECL_SYSCALL(syscall_dup2);
DECL_SYSCALL(syscall_syslog);
DECL_SYSCALL(syscall_unlink);
DECL_SYSCALL(syscall_signal);
DECL_SYSCALL(syscall_sigret);
DECL_SYSCALL(syscall_setsigret);
DECL_SYSCALL(syscall_kill);

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
	[SYSCALL_SHUTDOWN] = syscall_shutdown,
	[SYSCALL_DETACH] = syscall_detach,
	[SYSCALL_ACCESS] = syscall_access,
	[SYSCALL_MESG_SEND] = syscall_message_send,
	[SYSCALL_MESG_POP] = syscall_message_pop,
	[SYSCALL_DUP2] = syscall_dup2,
	[SYSCALL_SYSLOG] = syscall_syslog,
	[SYSCALL_UNLINK] = syscall_unlink,
	[SYSCALL_SIGNAL] = syscall_signal,
	[SYSCALL_SIGRET] = syscall_sigret,
	[SYSCALL_SETSIGRET] = syscall_setsigret,
	[SYSCALL_KILL] = syscall_kill
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

void syscall_shutdown(struct regs* regs)
{
	regs->eax = (u32)sys_shutdown();
}

void syscall_detach(struct regs* regs)
{
	regs->eax = 0;
	task_detach((pid_t)regs->ebx);
}

void syscall_access(struct regs* regs)
{
	regs->eax = (u32)sys_access((const char*)regs->ebx, (int)regs->ecx);
}

void syscall_message_send(struct regs* regs)
{
	regs->eax = (u32)sys_message_send((pid_t)regs->ebx, (unsigned int)regs->ecx, (const char*)regs->edx, (size_t)regs->edi);
}

void syscall_message_pop(struct regs* regs)
{
	regs->eax = (u32)sys_message_pop((message_t*)regs->ebx, (unsigned int)regs->ecx, (unsigned int)regs->edx);
}

void syscall_dup2(struct regs* regs)
{
	regs->eax = (u32)sys_dup2((int)regs->ebx, (int)regs->ecx);
}

void syscall_unlink(struct regs* regs)
{
	regs->eax = (u32)sys_unlink((const char*)regs->ebx);
}

void syscall_syslog(struct regs* regs)
{
	int level = (int)regs->ebx;
	char* message = (char*)regs->ecx;

	syslog(level, message);

	// Remove formatting
	for(size_t i = 0; i < strlen(message); ++i){
		if( message[i] == '%' ){
			// If this is a formatting character, replace with space
			if( i == (strlen(message)-1) || message[i+1] != '%' ) message[i] = ' ';
			// If it is double '%', just leave it. snprintf will replace with one %.
			else i++;
		}
	}

	// Broadcast message
	//syslog(level, message);
}

void syscall_signal(struct regs* regs)
{
	regs->eax = (u32)signal_signal(current, (int)regs->ebx, (sighandler_t)regs->ecx);
}

void syscall_sigret(struct regs* regs ATTR((unused)))
{
	signal_return(current);
}

void syscall_setsigret(struct regs* regs)
{
	signal_set_return(current, (void*)regs->ebx);
}

void syscall_kill(struct regs* regs)
{
	struct task* task = task_lookup((pid_t)regs->ebx);
	if( task == NULL ){
		regs->eax = (u32)-ESRCH;
		return;
	}

	regs->eax = (u32)signal_kill(task, (int)regs->ecx);
}