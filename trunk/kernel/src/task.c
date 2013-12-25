#include "task.h"
#include <errno.h>

extern u32 		initial_stack;		// defined in start.s
struct task		*current = NULL;	// always points to the current task
list_t			ready_tasks;		// list of ready tasks
list_t			task_globlist;		// global list of all tasks
//struct task		*ready_tasks;		// list of ready tasks
pid_t			next_pid = 0;		// the next process id

/* function: task_init
 * purpose:
 * 	initialize the tasking structure and initial task
 * parameters:
 * 	none.
 * return value:
 * 	none.
 * notes:
 * 	
 */
void task_init( void )
{
	struct task*		init = NULL;		// the initial task structure
//	u32			stack_addr = 0;		// the address within the new stack
//	u32*			stack_ptr = NULL;	// pointer within the stack.
	u32*			old_stack = NULL;	// pointer within the old stack
	u32*			old_base = NULL;	// old base pointer
	u32*			new_stack = NULL;	// pointer within the new stack
	u32*			new_base = NULL;	// new base pointer
	
	// initialize the task list
	INIT_LIST(&ready_tasks);
	INIT_LIST(&task_globlist);
	
	// allocate the initial
	init = (struct task*)kmalloc(sizeof(struct task));
	if(!init){
		printk("%2Verror: unable to allocate initial task structure!\n");
		return;
	}
	// zero the initial task
	memset(init, 0, sizeof(struct task));
	
	// setup some initial values
	init->t_pid = next_pid++;
	init->t_flags = TF_RUNNING;
	init->t_dir = copy_page_dir(curdir);
	init->t_parent = init;
	INIT_LIST(&init->t_sibling);
	INIT_LIST(&init->t_queue);
	INIT_LIST(&init->t_children);
	INIT_LIST(&init->t_globlink);
	//printk("%2Vtask_init: init->t_dir=%08X\n", init->t_dir);
	//while(1);
	// we are still using kerndir up to now
	switch_page_dir(init->t_dir);
	
	
	// allocate the kernel stack
	for(u32 stack_base = TASK_KSTACK_ADDR; stack_base < TASK_KSTACK_ADDR+TASK_KSTACK_SIZE; stack_base += 0x1000)
	{
		alloc_frame(get_page((void*)stack_base, 1, curdir), 0, 1);
	}
	
	// copy the memory from the old stack
	memcpy((void*)(TASK_KSTACK_ADDR), &initial_stack, TASK_KSTACK_SIZE);
	// now we need to find and fix all the EBP values on the stack
	// First, get the current ESP
	asm volatile ("mov %%esp,%0" :"=r"(old_stack));
	// Then Calculate the new esp based on initial_stack and old_stack and stack size.
	new_stack = (u32*)( (u32)old_stack - (u32)(&initial_stack) + TASK_KSTACK_ADDR );
	// iterate through every item on the stack, if the item has a value
	// within the old stack, adjust it to the new stac. It could be a false
	// positive, but it is the best we can do with a blind stack move.
	for(; (u32)new_stack < (TASK_KSTACK_ADDR+TASK_KSTACK_SIZE); new_stack = (u32*)((u32)new_stack + 4))
	{
		if( *new_stack >= (u32)&initial_stack && *new_stack < ((u32)&initial_stack + TASK_KSTACK_SIZE) )
		{
			*new_stack = *new_stack - (u32)&initial_stack + TASK_KSTACK_ADDR;
		}
	}
	
	// Get the current EBP
	asm volatile("mov %%ebp,%0":"=r"(old_base));
	// Calculate the new base address based on the old base adddress
	new_base = (u32*)( (u32)old_base - (u32)(&initial_stack) + TASK_KSTACK_ADDR );
	// Calculate the new stack address based on the old stack address
	new_stack = (u32*)( (u32)old_stack - (u32)(&initial_stack) + TASK_KSTACK_ADDR );
	// load the new stack pointer and base pointer
	asm volatile("mov %0,%%ebp; mov %1,%%esp"::"r"(new_base), "r"(new_stack));
	
	list_add(&init->t_queue, &ready_tasks);
	current = init;
}

/* function: task_preempt
 * purpose:
 * 	decrement the current tasks tick counter and
 * 	preempt it if needed.
 * parameters:
 * 	regs - the registers structure from the interrupt handler
 * return value: 
 * 	none.
 */
void task_preempt(struct regs* regs)
{
	UNUSED(regs);
	u32 esp;					// the old stack pointer
	u32 ebp;					// the old base pointer
	u32 eip;					// the old instruction pointer
	
	// There are no more tasks!
	if( !current || list_is_lonely(&current->t_queue, &ready_tasks) )
	{
		return;
	}
	
	// we have a timing schedule, follow it
	if( current->t_ticks_left != 0 && (current->t_flags & TF_RUNNING) && !(current->t_flags & TF_RESCHED))
	{
		current->t_ticks_left--;
		return;
	}
	
	// read the current/old esp and ebp
	asm volatile("mov %%esp,%0" : "=r"(esp));
	asm volatile("mov %%ebp,%0" : "=r"(ebp));
	// read the address of the line after this one
	eip = read_eip();
	
	// we do some magic to return this if we just switched
	if( eip == TASK_MAGIC_EIP ){
		return;
	}
	
	current->t_eflags = disablei(); // interrupts are already disabled, so this just returns the current eflags
	current->t_eip = eip;
	current->t_esp = esp;
	current->t_ebp = ebp;
	current->t_flags &= ~TF_RESCHED; // remove the reschedule flag

	// if this task just stopped running, it is no longer in the ready_tasks queue
	if( T_RUNNING(current) )
		current = list_next(&current->t_queue, struct task, t_queue, &ready_tasks);
	else
		current = list_entry(list_first(&ready_tasks), struct task, t_queue);
	// Check for end of list or dead task
	while( !current || (current->t_flags & TF_EXIT) )
	{
		if(!current)
		{
			current = list_next(&ready_tasks, struct task, t_queue, &ready_tasks);
			continue;
		}
		if(current->t_flags & TF_EXIT)
		{
			struct task* dead = current;
			current = list_next(&current->t_queue, struct task, t_queue, &ready_tasks);
			task_kill(dead);
			continue;
		}
	}
			
	
	eip = current->t_eip;
	esp = current->t_esp;
	ebp = current->t_ebp;
	curdir = current->t_dir;
	current->t_ticks_left = 15;
	
	/*
	 * push the flags register onto the current stack
	 * load the new EIP
	 * load the new stack
	 * load the new base register
	 * load the new page directory
	 * load the read_eip magic return value
	 * read the new processes flags off the new processes stack
	 * jump to the new EIP
	 */
	asm volatile("\
			cli;\
			mov %1,%%esp;\
			mov %2,%%ebp;\
			mov %3,%%cr3;\
			mov $0xDEADCABB,%%eax;\
			pushl %4;\
			popf;\
			jmp *%0;"
				: : "b"(eip),"c"(esp),"d"(ebp),"S"(curdir->phys), "D"(current->t_eflags));
}

/* function: task_kill
 * purpose:
 * 	destroy all data from the task. The task structure may be
 * 	kept in memory until its parent is killed or it is wait'd on
 * 	in order to preserve its result code for another task.
 * arguments:
 * 	the task to kill
 * return value:
 * 	none.
 */
void task_kill(struct task* dead)
{
	
	if( dead == current )
	{
		debug_message("unable to kill current task.", 0);
		return;
	}
	
	if( dead->t_pid == 0 )
	{
		debug_message("warning: attempted to kill init task!", 0);
		
		// We need to return the init task to a stable running state
		dead->t_flags = TF_RUNNING;
		return;
	}
	
	if( !list_empty(&dead->t_children) )
	{
		debug_message("warning: unable to kill parent task with running children!", 0);
		debug_message("\ttask %d is not killed, but is not running either.", dead->t_pid);
		return;
	}
	
	debug_message("killing task with id=%d", dead->t_pid);
	
	// remove the task from the running queue
	list_rem(&dead->t_queue);
	dead->t_flags = TF_ZOMBIE;
	
	// we haven't implemented this yet...
	//free_page_dir(dead->t_dir);
	
	// The task is not actually dead yet.
	// It needs to notify the parent of it's death,
	// then the parent will do what it needs to.
	// if the parent isn't listening, the child
	// will be cleaned up upon the parents death.
	
	if( dead->t_parent->t_flags & TF_WAITTASK )
	{
		if( dead->t_parent->t_waitfor == -1 || \
			(dead->t_parent->t_waitfor == 0 && dead->t_gid == dead->t_parent->t_pid) || \
			(dead->t_parent->t_waitfor < -1 && dead->t_gid == (-dead->t_parent->t_waitfor)) || \
			(dead->t_parent->t_waitfor > 0 && dead->t_parent->t_waitfor == dead->t_pid ) )
		{
			dead->t_parent->t_waitfor = dead->t_pid;
			dead->t_parent->t_status = dead->t_status;
			dead->t_parent->t_flags &= ~TF_WAITTASK;
			dead->t_parent->t_flags |= TF_RUNNING;
			list_add(&dead->t_parent->t_queue, &ready_tasks); 
		}
	}
	
}

/* function: task_lookup
 * purpose:
 * 	look for a task with a given process id
 * parameters:
 * 	pid - the process id to search for
 * return value:
 * 	the task structure for the given task
 * 	or NULL if it doesn not exist.
 */
struct task* task_lookup(pid_t pid)
{
	list_t* iter = NULL;
	list_for_each(iter, &task_globlist)
	{
		struct task* task = list_entry(iter, struct task, t_globlink);
		if( task->t_pid == pid ) return task;
	}
	return NULL;
}

/* function: sys_fork
 * purpose:
 * 	duplicate a running process
 * 	the function assumes you are calling from kernel state, and therefore
 * 	are within the task->t_kernel_stack stack (e.g. esp points within it).
 * arguments:
 * 	none.
 * return value:
 * 	for the parent: the PID of the child
 * 	for the child: zero.
 * notes:
 */
int sys_fork( void )
{
	struct task*	task = NULL;	// the new task structure
	u32 		esp = 0,	// the current esp
			ebp = 0;	// the current ebp
	u32		eflags = 0;	// the saved eflags value
	
	// disable interrupts
	eflags = disablei();
	// allocate the task structure
	task = (struct task*)kmalloc(sizeof(struct task));
	memset(task, 0, sizeof(struct task));
	
	// grab the esp and ebp values
	asm volatile("mov %%esp,%0" : "=r"(esp));
	asm volatile("mov %%ebp,%0" : "=r"(ebp));
	
	// setup the initial values for the new task
	task->t_pid = next_pid++;
	task->t_flags  = 0;
	task->t_esp = esp;
	task->t_ebp = ebp;
	task->t_parent = current;
	INIT_LIST(&task->t_children);
	INIT_LIST(&task->t_sibling);
	INIT_LIST(&task->t_queue);
	INIT_LIST(&task->t_globlink);

	// copy the page directory
	task->t_dir = copy_page_dir(current->t_dir);
	
	if( !task->t_dir ){
		printk("%2Vsys_fork: unable to copy page directory!\n");
		kfree(task);
		restore(eflags);
		return -1;
	}
	
	// this is where the new task will start
	u32 eip = read_eip();
	
	// we have already switched, so we must be the child (we just travelled back in time o.O)
	if( eip == 0xDEADCABB ){
		return 0;
	}
	// this means we are the parent, and we need to setup the child
	
	task->t_eip = eip;
	
	// link the task into the list
//	task->t_next = current->t_next;
//	current->t_next = task;
	
	// add the task just after the current task
	list_add(&task->t_queue, &ready_tasks);
	list_add(&task->t_sibling, &current->t_children);
	list_add(&task->t_globlink, &task_globlist);
	
	task->t_eflags = eflags;
	
	// the task is ready to run
	// and the parent will give up its timeslice
	task->t_flags = TF_RUNNING;
	current->t_flags |= TF_RESCHED;
	
	
	restore(eflags);
	
	return task->t_pid;
}

/* function: sys_exit
 * purpose:
 * 	sets the result field of the task, and the exit
 * 	flag. It then waits for a task switch, and never
 * 	returns. Waiting tasks will be notified.
 * parameters:
 * 	none.
 * return value:
 * 	does not return.
 */
void sys_exit( int result )
{
	if( current->t_pid == 0 )
	{
		debug_message("warning: attempted to kill init task!", 0);
		return;
	}
	
	u32 eflags = disablei();
	
	current->t_status = result;
	current->t_flags = TF_EXIT;
	
	schedule();
	
	restore(eflags);
	
	while(1);
	
}

/* function: sys_waitpid
 * purpose:
 * 	waits for a given child process to change state.
 * parameters:
 * 	pid - process id of the child to wait on
 * 	status - the status information from the process
 * 	options - optional wait flags
 * return value:
 * 	the pid of the process that changed state.
 */
pid_t sys_waitpid(pid_t pid, int* status, int options)
{
	// we only support two options at the moment.
	if( options != 0 && options != WNOHANG ){
		return (pid_t)-EINVAL;
	}
	
	u32 eflags = disablei();
	list_t* iter = NULL;
	int need_wait = 1;
	
	// This means we want to wait on a specific child process
	if( pid > 0 )
	{
		// first check if the process is already changed state
		struct task* task = task_lookup(pid);
		if( task == NULL ){
			restore(eflags);
			return (pid_t)-ECHILD;
		} else if( task->t_parent != current ){
			restore(eflags);
			return (pid_t)-ECHILD;
		} else if( T_ISZOMBIE(task) ){
			if( status ) 
				*status = task->t_status;
			pid = task->t_pid;
			need_wait = 0;
		}
	} else {
	// This means we are waiting on a group of processes
		// First check if any of these processes have already changed state
		list_for_each(iter, &current->t_children){
			struct task* task = list_entry(iter, struct task, t_sibling);
			// has it changed state, and
			// are we looking for any child, or
			// this task is in the group which we are the leader of or
			// this task is in the specified group
			// we should store the status and return the pid
			if( T_ISZOMBIE(task) && (pid == -1 || (pid == 0 && task->t_gid == current->t_pid) || (pid < -1 && task->t_pid == (-pid))) ){
				if( status )
					*status = task->t_status;
				pid = task->t_pid;
				need_wait = 0;
			}
		}
	}
	
	// This means no task was found that had already changed states
	if( need_wait == 1 )
	{
		// the process asked not to hang
		if( options  & WNOHANG ){
			restore(eflags);
			return 0;
		}
		
		// This means that the task has not yet changed states
		// store the pid that we are waiting on
		current->t_waitfor = pid;
		// Set the wait flag and reschedule flag
		current->t_flags &= ~TF_RUNNING;
		current->t_flags |= TF_WAITTASK;
		// Remove ourself from the running queue
		list_rem(&current->t_queue);
		
		// reschedule the task system (we already set the reschedule flag for this task
		// so it will pick a new one, and not return until TF_WAITTASK is unset).
		schedule();

		// the task_kill function for the other task will store
		// its' pid and status information in our task structure
		pid = current->t_waitfor;
		*status = current->t_status;
	}
	
	struct task* task = task_lookup(pid);
	list_rem(&task->t_sibling); // remove from the parent-child list
	list_rem(&task->t_globlink); // remove from the global task list
	// the tasks runtime data should have already been free'd, we just need
	// to free the task structure and remove it from lists.
	kfree(task);
	
	restore(eflags);
	
	// return the tasks process id
	return pid;
}