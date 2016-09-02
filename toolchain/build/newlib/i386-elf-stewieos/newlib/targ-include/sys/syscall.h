#ifndef _STEWIEOS_SYSCALL_H_
#define _STEWIEOS_SYSCALL_H_

enum _syscall_indexes
{
	SYSCALL_EXIT,
	SYSCALL_FORK,
	SYSCALL_WAITPID,
	SYSCALL_EXECVE,
	SYSCALL_BRK,
	SYSCALL_GETPID,
	SYSCALL_KILL,
	SYSCALL_MKNOD,
	SYSCALL_OPEN,
	SYSCALL_CLOSE,
	SYSCALL_READ,
	SYSCALL_WRITE,
	SYSCALL_LSEEK,
	SYSCALL_FSTAT,
	SYSCALL_IOCTL,
	SYSCALL_ACCESS,
	SYSCALL_RENAME,
	SYSCALL_FCNTL,
	SYSCALL_MKDIR,
	SYSCALL_RMDIR,
	SYSCALL_UNLINK,
	SYSCALL_MOUNT,
	SYSCALL_UMOUNT,
	SYSCALL_CHDIR,
	SYSCALL_INSMOD,
	SYSCALL_RMMOD,
	SYSCALL_SBRK,
	SYSCALL_ISATTY,
	SYSCALL_READDIR,
	SYSCALL_GETCWD,
	SYSCALL_SHUTDOWN,
	SYSCALL_DETACH,
	SYSCALL_MESG_SEND,
	SYSCALL_MESG_POP,
	SYSCALL_DUP2,
	SYSCALL_SYSLOG,
	SYSCALL_SIGNAL,
	SYSCALL_SIGRET,
	SYSCALL_SETSIGRET,
	SYSCALL_COUNT
};

#define SYSCALL0(f, result) asm volatile ("int $0x80" : "=a"(result) : "a"(f))
#define SYSCALL1(f, result, p0) asm volatile ("int $0x80" : "=a"(result) : "a"(f), "b"(p0))
#define SYSCALL2(f, result, p0, p1) asm volatile ("int $0x80" : "=a"(result) : "a"(f), "b"(p0), "c"(p1))
#define SYSCALL3(f, result, p0, p1, p2) asm volatile ("int $0x80" : "=a"(result) : "a"(f), "b"(p0), "c"(p1), "d"(p2))
#define SYSCALL4(f, result, p0, p1, p2, p3) asm volatile ("int $0x80" : "=a"(result) : "a"(f), "b"(p0), "c"(p1), "d"(p2), "S"(p3))
#define SYSCALL5(f, result, p0, p1, p2, p3, p4) asm volatile ("int $0x80" : "=a"(result) : "a"(f), "b"(p0), "c"(p1), "d"(p2), "S"(p3), "D"(p4))

#endif