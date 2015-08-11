#ifndef _SEM_H_
#define _SEM_H_

#include "kernel.h"
#include "spinlock.h"
#include "linkedlist.h"
#include "timer.h"
#include "error.h"

#define SEM_FOREVER ((tick_t)-1)

typedef struct _sem_t
{
	int units; // the number of units currently available
	spinlock_t lock; // the spinlock protecting the semaphore
	list_t tasks; // the tasks waiting on a unit
	tick_t earliest_timeout; // the earliest timeout scheduled
} sem_t;

tick_t sem_timeout(tick_t now, struct regs* regs, sem_t* sem);

sem_t* sem_alloc(int max_units);
int sem_free(sem_t* sem);

int sem_wait(sem_t* sem, tick_t timeout);
void sem_signal(sem_t* sem);
int sem_avail(sem_t* sem);

#endif
