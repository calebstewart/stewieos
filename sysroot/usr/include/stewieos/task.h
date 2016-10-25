#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "stewieos/fs.h"
#include "stewieos/kernel.h"
#include "stewieos/kmem.h"
#include "stewieos/paging.h"
#include "stewieos/descriptor_tables.h"
#include "stewieos/timer.h"
#include "stewieos/pmm.h"
#include "stewieos/spinlock.h"
#include "stewieos/ksignal.h"
#include <sys/message.h>

// A running task
#define TF_RUNNING ((u32)(1<<0))
// A task requesting to be rescheduled (giving up its timeslice)
#define TF_RESCHED ((u32)(1<<1))
// The task is waiting on another task
#define TF_WAITTASK ((u32)(1<<2))
// The task is waiting on a signal
#define TF_WAITSIG ((u32)(1<<3))
// The task is requesting to be killed
#define TF_EXIT ((u32)(1<<4))
// The has been killed, and is waiting to be waited for
#define TF_ZOMBIE ((u32)(1<<5))
// The task has just performed an execve and needs the scheduler to "finish up"
#define TF_EXECVE ((u32)(1<<6))
// The task is waiting on IO
#define TF_WAITIO ((u32)(1<<7))
// The task is waiting on a message
#define TF_WAITMESG ((u32)(1<<8))
// Executing a signal handler
#define TF_SIGNAL ((u32)(1<<9))
// Task has been stopped
#define TF_STOPPED ((u32)(1<<10))

#define TF_WAITMASK (TF_WAITIO | TF_WAITSIG | TF_WAITTASK | TF_WAITMESG | TF_STOPPED)

#define T_ISZOMBIE(task) ((task)->t_flags & TF_ZOMBIE)
#define T_RUNNING(task) ((task)->t_flags & TF_RUNNING)
#define T_EXECVE(task)	((task)->t_flags & TF_EXECVE)
#define T_WAITING(task)	((task)->t_flags & TF_WAITMASK)
#define T_EXITING(task)	((task)->t_flags & TF_EXIT)

#define TASK_KSTACK_ADDR	0xFFFFD000
#define TASK_KSTACK_SIZE	0x2000
#define TASK_MAGIC_EIP		0xDEADCABB

// Special stack devoted to signal handlers
#define TASK_SIGNAL_STACK		(KERNEL_VIRTUAL_BASE)
#define TASK_SIGNAL_STACK_SIZE	(0x1000)
// User stack
#define TASK_STACK_START		(KERNEL_VIRTUAL_BASE-TASK_SIGNAL_STACK_SIZE)
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

typedef struct _message_container
{
	list_t link;
	message_t message;
} message_container_t;

typedef struct _message_queue
{
	spinlock_t lock;
	list_t queue;
} message_queue_t;

/* Process Task Structure */
struct task
{
	pid_t				t_pid;					// process id
	pid_t				t_gid;					// process group id
	pid_t				t_sid;					// session id
	uid_t				t_uid;
	u32					t_flags;				// the flags/state of this task
	u32					t_esp;					// the stack pointer
	u32					t_ebp;					// the base pointer
	u32					t_eip;					// the instruction pointer to jump to
	u32					t_eflags;				// the eflags before the last switch
	pid_t				t_waitfor;				// what are we waiting on? See waitpid.
	
	tick_t				t_ticks_left;				// the number of clock ticks left until we preempt
	tick_t				t_timeout;				// Usually just the semaphore timeout
	
	int					t_status;				// result code from sys_exit
	
	mode_t				t_umask;				// The current umask
	
	u32					t_dataend;				// End of the data (used for sbrk)
	
	char				t_fpu[512];				// FPU state (For FXSAVE and FXRSTOR)
	
	struct page_dir*	t_dir;					// page directory for this task
	struct vfs			t_vfs;					// this tasks virtual file system information
	struct regs			t_regs;					// Register Information for the task switch after an execve
	message_queue_t		t_mesgq;				// message queue for ipc
	signal_state_t		t_signal;				// Signal State for this process
	
	list_t				t_sibling;				// the link in the parents children list
	list_t				t_children;				// list of child tasks (forked processes)
	list_t				t_queue;				// link in the current queue
	list_t				t_globlink;				// link in the global list
	list_t				t_ttywait;				// link in the tty wait list
	list_t				t_semlink;				// semaphore wait list
	struct task*		t_parent;				// the parent of this task
};

void task_init( void );
void task_preempt(struct regs* regs);
void task_switch(struct task* task);
void task_free(struct task* task);
void task_kill(struct task* task, int status);
// lookup a task based on process id
struct task* task_lookup(pid_t pid);

void task_wait(struct task* task, int wait_flag);
void task_waitio(struct task* task);
void task_wakeup(struct task* task);

pid_t task_getfg( void );
void task_setfg(pid_t pid);

pid_t task_setsid( void );

// detach a task from its parent and relinquish foregroundness, if needed, to the parent
void task_detach(pid_t pid);

caddr_t sys_sbrk(int incr);
pid_t sys_getpid( void );
int sys_fork( void );
void sys_exit( int result );
pid_t sys_waitpid(pid_t pid, int* status, int options);

int sys_message_send(pid_t pid, unsigned int type, const char* what, size_t length);
int sys_message_pop(message_t* message, unsigned int id, unsigned int flags);
// Raise a signal for yourself or another process
int sys_kill(pid_t pid, int signum);
// Set your signal handler
int sys_signal(int signum, sighandler_t* handler);
// Return from a signal
int sys_sigret( void );

pid_t task_spawn(int kern);
pid_t worker_spawn(void(*worker)(void*), void* context);

tick_t task_sleep_monitor(tick_t now, struct regs* regs, void* context);
int task_sleep(struct task* task, u32 milli);

extern struct task* current;

#endif
