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
#define TF_RUNNING 0x00000001
// A task requesting to be rescheduled (giving up its timeslice)
#define TF_RESCHED 0x00000002
// The task is waiting on another task
#define TF_WAITTASK 0x00000004
// The task is waiting on a signal
#define TF_WAITSIG 0x00000008
// The task is requesting to be killed
#define TF_EXIT 0x00000010

#define TASK_KSTACK_ADDR	0xFFFFD000
#define TASK_KSTACK_SIZE	0x2000
#define TASK_MAGIC_EIP		0xDEADCABB

struct task
{
	pid_t			t_id;					// task id
	u32			t_flags;				// the flags/state of this task
	u32			t_esp;					// the stack pointer
	u32			t_ebp;					// the base pointer
	u32			t_eip;					// the instruction pointer to jump to
	u32			t_eflags;				// the eflags before the last switch
	
	tick_t			t_ticks_left;				// the number of clock ticks left until we preempt
	
	int			t_result;				// result code from sys_exit
	
	struct page_dir*	t_dir;					// page directory for this task
	//struct vfs		t_vfs;					// this tasks virtual file system information
	
	list_t			t_sibling;				// the siblings of this task (in whatever list it resides)
	struct task*		t_waitingon;				// the task we are waiting on
};

void task_init( void );
void task_preempt(struct regs* regs);
void task_kill(struct task* task);

int sys_fork( void );
void sys_exit( int result );

#endif