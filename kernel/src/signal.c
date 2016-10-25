/*
* @Author: Caleb Stewart
* @Date:   2016-07-18 00:23:11
* @Last Modified by:   Caleb Stewart
* @Last Modified time: 2016-10-15 14:08:29
*/
#include <task.h>
#include "stewieos/error.h"
#include "stewieos/ksignal.h"

void signal_default_term(struct task* task, int sig);
void signal_default_core(struct task* task, int sig);
void signal_default_cont(struct task* task, int sig);
void signal_default_stop(struct task* task, int sig);
void signal_default_ignore(struct task* task, int sig);

void(*signal_default[NSIG])(struct task*,int) = {
	[SIGHUP] = signal_default_term,
	[SIGINT] = signal_default_term,
	[SIGQUIT] = signal_default_core,
	[SIGILL] = signal_default_core,
	[SIGABRT] = signal_default_core,
	[SIGFPE] = signal_default_core,
	[SIGKILL] = signal_default_term,
	[SIGPIPE] = signal_default_term,
	[SIGALRM] = signal_default_term,
	[SIGTERM] = signal_default_term,
	[SIGUSR1] = signal_default_term,
	[SIGUSR2] = signal_default_term,
	[SIGCHLD] = signal_default_ignore,
	[SIGCONT] = signal_default_cont,
	[SIGSTOP] = signal_default_stop,
};

void signal_init(struct task* task)
{
	for(int i = 0; i < NSIG; ++i){
		LOWER_SIGNAL(task, i);
		task->t_signal.handler[i] = SIG_DFL;
	}
	task->t_signal.nraised = 0;
}

void signal_copy(struct task* dst, struct task* src)
{
	memcpy(&dst->t_signal, &dst->t_signal, sizeof(src->t_signal));
}

void signal_save(struct task* task)
{
	// If we are already handling a signal
	if( (task->t_flags & TF_SIGNAL) != 0 ) return;

	// Save the current state information
	task->t_signal.eflags = task->t_eflags;
	task->t_signal.eip = task->t_eip;
	task->t_signal.esp = task->t_esp;
	task->t_signal.ebp = task->t_ebp;
	memcpy(task->t_signal.fpu, task->t_fpu, 512);

}

int signal_check(struct task* task)
{
	int sig = -1;
	void* temp = NULL;

	// If no signals have been raised, just return
	if( task->t_signal.nraised == 0 ) return 0;

	// Look for a raised signal
	for(sig = 0; sig < NSIG; ++sig)
	{
		if( SIGNAL_RAISED(task, sig) ) break;
	}

	// This shouldn't happen...
	if( sig == NSIG ){
		task->t_signal.nraised = 0;
		return 0;
	}

	// Save the current process state
	signal_save(task);

	// Unmask this signal
	LOWER_SIGNAL(task, sig);

	if( task->t_signal.handler[sig] == SIG_DFL ) {
		signal_default[sig](task, sig);
		signal_check(task);
		return 0;
	} else if( task->t_signal.handler[sig] == SIG_IGN ) {
		// Didn't see it!
		return 0;
	}

	//syslog(KERN_WARN, "dispatching signal for process %d", task->t_pid);

	// Setup the task to return to the signal
	// handler
	task->t_eflags = 0x200200;
	task->t_eip = (u32)task->t_signal.handler[sig];
	task->t_esp = TASK_SIGNAL_STACK-8;
	task->t_ebp = 0;
	task->t_flags |= TF_SIGNAL;

	// EIP is 0, so we can't return. This is why the
	// handler must call 
	temp = temporary_map(TASK_SIGNAL_STACK-0x1000, task->t_dir);
	*((u32*)( (u32)temp + 0x1000 - 4 )) = (u32)sig;
	// Return address (EIP)
	*((u32*)( (u32)temp + 0x1000 - 8 )) = (u32)task->t_signal.handler_return;

	return 1;
}

// This sets where the signal handler should return to.
// This function must be in user space and must call
// signal_return in order to successfully return from
// the signal handler. It should have the following
// prototype:
//
// void user_return_func( void ) __attribute__((noreturn));
void signal_set_return(struct task* task, void* userfunc)
{
	//syslog(KERN_WARN, "setting signal return pointer.");
	task->t_signal.handler_return = userfunc;
}

void signal_return(struct task* task)
{
	// We weren't signaled. Just ignore this call.
	if( (task->t_flags & TF_SIGNAL) == 0 ) return;

	// If there was another signal, don't reset
	if( signal_check(task) == 0 )
	{
		// Reset the return parameters to the main
		// thread of execution.
		task->t_eflags = task->t_signal.eflags;
		task->t_eip = task->t_signal.eip;
		task->t_esp = task->t_signal.esp;
		task->t_ebp = task->t_signal.ebp;
		memcpy(task->t_fpu, task->t_signal.fpu, 512);
		task->t_flags &= ~TF_SIGNAL;
	}

	if( task == current ){
		task_switch(current);
	}

}

int signal_kill(struct task* task, int sig)
{
	if( sig < 0 || sig >= NSIG ) {
		return -EINVAL;
	}

	//syslog(KERN_WARN, "raising signal for process %d.", task->t_pid);
	RAISE_SIGNAL(task, sig);

	// Reschedule if the task is not already in a signal handler
	if( task == current ){
		task->t_ticks_left = 0;
		schedule();
	}

	return 0;
}

sighandler_t signal_signal(struct task* task, int sig, sighandler_t handler)
{
	if( sig < 0 || sig >= NSIG ){
		return ERR_PTR(-EINVAL);
	}

	// SIGKILL and SIGSTOP can't be modified
	switch( sig )
	{
		case SIGKILL:
		case SIGSTOP:
			return ERR_PTR(-EINVAL);
			break;
	}

	sighandler_t old = task->t_signal.handler[sig];

	task->t_signal.handler[sig] = handler;

	//syslog(KERN_WARN, "setting signal %d to %p", sig, handler);

	return old;
}

void signal_default_term(struct task* task, int sig)
{
	task_kill(task, -SIGKILL);
}
void signal_default_core(struct task* task, int sig)
{
	task_kill(task, -SIGKILL);
}
void signal_default_cont(struct task* task, int sig)
{
	task_wakeup(task);
}
void signal_default_stop(struct task* task, int sig)
{
	task_wait(task, TF_STOPPED);
}
void signal_default_ignore(struct task* task, int sig)
{
	return;
}