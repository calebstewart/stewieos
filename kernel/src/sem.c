#include "stewieos/sem.h"
#include "stewieos/task.h"
#include "stewieos/linkedlist.h"

/* Allocate a new semaphore object */
sem_t* sem_alloc(int max_units)
{
	sem_t* sem = (sem_t*)kmalloc(sizeof(sem_t));
	if( sem == NULL ){
		return NULL;
	}
	
	sem->units = max_units;
	sem->earliest_timeout = 0xFFFFFFFF;
	spin_init(&sem->lock);
	INIT_LIST(&sem->tasks);
	
	return sem;
}

/* Free an existing semaphore object */
int sem_free(sem_t* sem)
{
	spin_lock(&sem->lock);
	
	if( !list_empty(&sem->tasks) ){
		spin_unlock(&sem->lock);
		return -EBUSY;
	}
	
	kfree(sem);
	
	return 0;
}

/* Registered for timer callback in order to timeout waiting tasks */
tick_t sem_timeout(tick_t now, struct regs* regs ATTR((unused)), sem_t* sem)
{
	list_t* iter = NULL, *temp = NULL;
	tick_t earliest = 0xFFFFFFFF;
	
	spin_lock(&sem->lock);
	
	// Look for timed out tasks
	list_for_each(iter, &sem->tasks)
	{
		struct task* task = list_entry(iter, struct task, t_semlink);
		// wake up all timed out tasks
		if( task->t_timeout <= now ){
			task->t_timeout = 0;
			temp = iter->prev;
			list_rem(&task->t_semlink);
			iter = temp;
			task_wakeup(task);
		} else if( earliest > task->t_timeout ){
			earliest = task->t_timeout;
		}
	}
	
	spin_unlock(&sem->lock);
	
	if( earliest == 0xFFFFFFFF ){
		return TIMER_CANCEL;
	}
	
	return earliest;
}

/* Wait for an open unit on the semaphore for up to 'timeout' ticks */
int sem_wait(sem_t* sem, tick_t timeout)
{
	spin_lock(&sem->lock);
	
	// we don't have anymore units. we'll wait.
	if( sem->units == 0 ){
		if( timeout == 0 ) return -ETIMEDOUT;
		
		list_add_before(&current->t_semlink, &sem->tasks);
		current->t_timeout = timer_get_ticks() + timeout;
		
		// setup a callback for the timeout
		if( sem->earliest_timeout > current->t_timeout ){
			syslog(KERN_WARN, "sem_wait: setting timer callback for %d..", current->t_timeout);
			sem->earliest_timeout = current->t_timeout;
			timer_callback(sem->earliest_timeout, sem, (timer_callback_t)sem_timeout);
		}
		
		spin_unlock(&sem->lock);
		task_waitio(current);
		
		// check if we timed out or if we aquired the semaphore
		if( current->t_timeout == 1 ){
			return 0;
		} else {
			return -ETIMEDOUT;
		}
	}
	
	// decrement the number of units and return
	sem->units--;
	
	spin_unlock(&sem->lock);
	
	return 0;
}

/* Release a unit from the semaphore */
void sem_signal(sem_t* sem)
{
	spin_lock(&sem->lock);
	
	// if there are no waiting tasks, just increase the units and return
	if( list_empty(&sem->tasks) ){
		sem->units++;
		spin_unlock(&sem->lock);
		return;
	}
	
	// if there are waiting tasks, set timeout to 1 (meaning we aquired the unit) and wakeup the task
	struct task* task = list_entry(list_first(&sem->tasks), struct task, t_semlink);
	task->t_timeout = 1;
	list_rem(&task->t_semlink);
	task_wakeup(task);
	
	spin_unlock(&sem->lock);
	
	return;
}