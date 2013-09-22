#include "kernel.h"
#include "timer.h"
#include "descriptor_tables.h"

struct callback_info;
struct callback_info
{
	timer_callback_t callback;
	tick_t when;
	struct callback_info* next, *prev;
};

static unsigned int timer_freq = 0;						// The current timer frequency
static tick_t current_tick = 0;							// The current time (tick count since init)
static tick_t next_callback_tick = 0;						// The time of the next callbackt to be called
static struct callback_info callbacks[TIMER_MAX_CALLBACKS];			// The list of callback structures to be inserted (empty=>when==TIMER_CANCEL)
static struct callback_info* next_callback = (struct callback_info*)0;		// The next callbacks info structure

void timer_interrupt(struct regs*);
void timer_interrupt(struct regs* regs)
{
	current_tick++;
	// Check for the firing of the next callback ( use while in case some callbacks have the same time)
	while( next_callback && next_callback->when <= current_tick )
	{
		// call the callback
		next_callback->when = next_callback->callback(current_tick, regs);
		// Remove the callback from the list
		struct callback_info* tmp = next_callback;
		next_callback = tmp->next;
		if( next_callback ){
			next_callback->prev = NULL;
		}
		tmp->next = (struct callback_info*)0;
		tmp->prev = NULL;
		// If the timer wasn't cancelled, reinsert the timer into the list (ordered by when it should fire)
		if( tmp->when != TIMER_CANCEL ){
			if( next_callback ){
				struct callback_info *iter = next_callback;
				while(iter) {
					if( iter->when > tmp->when ){
						if( iter->prev ){
							iter->prev->next = tmp;
						}
						tmp->prev = iter->prev;
						iter->prev = tmp;
						tmp->next = iter;
					} // end if iter when > tmp when 
					else if( iter->next == NULL ){
						iter->next = tmp;
						tmp->prev = iter;
					} // end if iter next == NULL 
				} // end while iter
				if( next_callback->prev ){ // we added it before the beginning, fix the next_callback pointer
					next_callback = next_callback->prev;
				}
			} else {
				next_callback = tmp;
			}
		} // end tmp->when != TIMER_CANCEL
	} // end while next callback and next callback time <= current time
	
}


void init_timer(unsigned int freq)
{
	printk("Initializing programmable interval timer... ");
	// initialize timers
	for(int i = 0; i < TIMER_MAX_CALLBACKS; ++i){
		callbacks[i].when = TIMER_CANCEL;
	}
	
	timer_freq = freq;
	
	register_interrupt(IRQ0, &timer_interrupt);
	
	u32 divisor = ((u32)(1193180 / freq));
	
	asm volatile("cli");
	
	// PIT command byte
	outb(0x43, 0b00110110);
	// Get the high and low bytes of the divisor
	u8 low = (u8)( divisor & 0xFF );
	u8 high = (u8)( (divisor >> 8) & 0xFF );
	
	outb(0x40, low);
	outb(0x40, high);
	
	asm volatile("sti");
	
	printk(" done.\n");
}

unsigned int timer_get_freq( void )
{ return timer_freq; }

tick_t timer_get_ticks( void )
{
	return current_tick;
}

int timer_callback(tick_t when, timer_callback_t callback)
{
	int nr = -1;
	for(int i = 0; i < TIMER_MAX_CALLBACKS; ++i){
		if( callbacks[i].when == TIMER_CANCEL ){
			nr = i;
			break;
		}
	}
	if( nr == -1 ){
		return -1;
	}
	
	callbacks[nr].when = when;
	callbacks[nr].callback = callback;
	callbacks[nr].next = NULL;
	callbacks[nr].prev = NULL;
	
	u32 eflags = disablei();
	
	// insert the callback at the appropriate position
	if( next_callback )
	{
		struct callback_info *iter = next_callback;
		while(iter) {
			if( iter->when > callbacks[nr].when ){
				if( iter->prev ){
					iter->prev->next = &callbacks[nr];
				}
				callbacks[nr].prev = iter->prev;
				iter->prev = &callbacks[nr];
				callbacks[nr].next = iter;
			} // end if iter when > tmp when 
			else if( iter->next == NULL ){
				iter->next = &callbacks[nr];
				callbacks[nr].prev = iter;
			} // end if iter next == NULL 
			iter = iter->next;
		} // end while iter
		if( next_callback->prev ){ // we added it before the beginning, fix the next_callback pointer
			next_callback = next_callback->prev;
		}
	} else {
		next_callback = &callbacks[nr];
	}
	
	restore(eflags);
	
	return 0;
}