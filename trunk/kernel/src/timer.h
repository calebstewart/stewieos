#ifndef _TIMER_H_
#define _TIMER_H_

#define TIMER_MAX_CALLBACKS 128
#define TIMER_CANCEL ((unsigned long)-1)

struct regs;
typedef unsigned long tick_t;
typedef tick_t (*timer_callback_t)(tick_t, struct regs*);

// Initialize the PIT to fire freq times per second
void init_timer(unsigned int freq);

// Get the current frequency of the PIT
unsigned int timer_get_freq( void );

// Get the current clock ticks up to this point
tick_t timer_get_ticks( void );

// Setup a timer callback to fire at a specific time
int timer_callback(tick_t when, timer_callback_t callback);

#endif