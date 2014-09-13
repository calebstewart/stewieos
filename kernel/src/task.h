#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "fs.h"
#include "kernel.h"
#include "kmem.h"
#include "paging.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "pmm.h"

// A running task
#define TF_RUNNING ((u32)0x00000001)
// A task requesting to be rescheduled (giving up its timeslice)
#define TF_RESCHED ((u32)0x00000002)
// The task is waiting on another task
#define TF_WAITTASK ((u32)0x00000004)
// The task is waiting on a signal
#define TF_WAITSIG ((u32)0x00000008)
// The task is requesting to be killed
#define TF_EXIT ((u32)0x00000010)
// The has been killed, and is waiting to be waited for
#define TF_ZOMBIE ((u32)0x00000020)
// The task has just performed an execve and needs the scheduler to "finish up"
#define TF_EXECVE ((u32)0x00000040)

#define T_ISZOMBIE(task) ((task)->t_flags & TF_ZOMBIE)
#define T_RUNNING(task) ((task)->t_flags & TF_RUNNING)
#define T_EXECVE(task)	((task)->t_flags & TF_EXECVE)

#define TASK_KSTACK_ADDR	0xFFFFD000
#define TASK_KSTACK_SIZE	0x2000
#define TASK_MAGIC_EIP		0xDEADCABB

#define TASK_STACK_START	KERNEL_VIRTUAL_BASE
#define TASK_STACK_INIT_SIZE	0x4000
#define TASK_STACK_INIT_BASE	(KERNEL_VIRTUAL_BASE-TASK_STACK_INIT_SIZE)
#define TASK_MAX_ARG_SIZE	(TASK_STACK_INIT_SIZE/2)

#define TASK_MAX_OPEN_FILES 1024

/* NOTE These should be moved to sys/wait.h */
#define WNOHANG 0x00000001

// This is a more convenient way to reschedule
// as task_preempt will do it, but you have to
// pass a NULL regs struct to it (regs is
// unused anyway).
#define schedule() task_preempt((struct regs*)(NULL))

struct task
{
	pid_t			t_pid;					// process id
	pid_t			t_gid;					// process group id
	uid_t			t_uid;
	u32			t_flags;				// the flags/state of this task
	u32			t_esp;					// the stack pointer
	u32			t_ebp;					// the base pointer
	u32			t_eip;					// the instruction pointer to jump to
	u32			t_eflags;				// the eflags before the last switch
	pid_t			t_waitfor;				// what are we waiting on? See waitpid.
	
	tick_t			t_ticks_left;				// the number of clock ticks left until we preempt
	
	int			t_status;				// result code from sys_exit
	
	mode_t			t_umask;				// The current umask
	
	struct page_dir*	t_dir;					// page directory for this task
	struct vfs		t_vfs;					// this tasks virtual file system information
	struct regs		t_regs;					// Register Information for the task switch after an execve
	
	list_t			t_sibling;				// the link in the parents children list
	list_t			t_children;				// list of child tasks (forked processes)
	list_t			t_queue;				// link in the current queue
	list_t			t_globlink;				// link in the global list
	struct task*		t_parent;				// the parent of this task
};

void task_init( void );
void task_preempt(struct regs* regs);
void task_kill(struct task* task);
// lookup a task based on process id
struct task* task_lookup(pid_t pid);

pid_t sys_getpid( void );
int sys_fork( void );
void sys_exit( int result );
pid_t sys_waitpid(pid_t pid, int* status, int options);

extern struct task* current;

#endif
