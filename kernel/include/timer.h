#ifndef _TIMER_H_
#define _TIMER_H_

#include <time.h>

#define TIMER_MAX_CALLBACKS 128				// maximum number of timer callbacks
#define TIMER_CANCEL ((unsigned long)-1)		// cancel the timer
#define TIMER_IN(t) (timer_get_ticks() + (t))		// fire the time in t ticks from now

struct regs;
typedef unsigned long tick_t;
typedef tick_t (*timer_callback_t)(tick_t, struct regs*, void* context);

// Initialize the PIT to fire freq times per second
void init_timer(unsigned int freq);

// Get the current frequency of the PIT
unsigned int timer_get_freq( void );

// Get the current clock ticks up to this point
tick_t timer_get_ticks( void );

// Setup a timer callback to fire at a specific time
int timer_callback(tick_t when, void* context, timer_callback_t callback);

time_t timer_get_time( void );
void timer_sync_time( void );

#endif