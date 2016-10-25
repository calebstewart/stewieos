#ifndef _SEM_H_
#define _SEM_H_

#include "stewieos/kernel.h"
#include "stewieos/spinlock.h"
#include "stewieos/linkedlist.h"
#include "stewieos/timer.h"
#include "stewieos/error.h"

#define SEM_FOREVER ((tick_t)-1)
#define SEM_NOWAIT	((tick_t)0)

typedef struct _sem_t
{
	int units; // the number of units currently available
	spinlock_t lock; // the spinlock protecting the semaphore
	list_t tasks; // the tasks waiting on a unit
	tick_t earliest_timeout; // the earliest timeout scheduled
} sem_t;

tick_t sem_timeout(tick_t now, struct regs* regs, sem_t* sem);

/* Function: sem_alloc
 * Parameters:
 * 			max_units - The number of units the semaphore can give out.
 * Returns:
 * 			A newly allocated semaphore or NULL when out of memory.
 */
sem_t* sem_alloc(int max_units);
/* Function: sem_free
 * Parameters:
 * 			sem - The semaphore to free
 * Returns:
 * 			It will return -EBUSY if there are tasks currently waiting
 * 			on the semaphore. Otherwise, 0 is returned on success or
 * 			some other -E* error value.
 */
int sem_free(sem_t* sem);
/* Function: sem_wait
 * Parameters:
 * 			sem - The semaphore to wait on
 * 			timeout - The number of ticks to wait. SEM_FOREVER to wait forever 
 * 						and SEM_NOWAIT to return immediately if there are no
 * 						units.
 * Returns:
 * 			-ETIMEDOUT 	- The timeout was reached before units were available.
 * 			0			- You acquired a unit from the semaphore.
 * Description:
 * 			Attempt to retrieve a unit from the semaphore, and wait for a 
 * 			specified amount of time if none are available. The task will
 * 			sleep until a unit is available.
 */
int sem_wait(sem_t* sem, tick_t timeout);
/* Function: sem_signal
 * Parameters:
 * 			sem - The semaphore to signal
 * Description:
 * 			Release a reference to a unit, and possibly wake up a waiting
 * 			task.
 */
void sem_signal(sem_t* sem);

/* Mutex's are simply binary semaphore's */
typedef sem_t mutex_t;

/* Inline versions of semaphore functions for the appearance of mutex's */
static inline mutex_t* mutex_alloc( void )
	{ return (mutex_t*)sem_alloc(1); }
static inline int mutex_free(mutex_t* mutex)
	{ return sem_free((sem_t*)mutex); }
static inline int mutex_lock(mutex_t* mutex, tick_t timeout)
	{ return sem_wait((sem_t*)mutex, timeout); }
static inline void mutex_unlock(mutex_t* mutex)
	{ sem_signal((sem_t*)mutex); }


#endif
