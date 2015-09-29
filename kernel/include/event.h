#ifndef _STEWIEOS_EVENT_H_
#define _STEWIEOS_EVENT_H_

#include "kernel.h"
#include "linkedlist.h"
#include "keys.h"

enum _event_type
{
	EVENT_KEY = (1<<0),
	EVENT_ABSOLUTE = (1<<1),
	EVENT_RELATIVE = (1<<2),
	EVENT_LED = (1<<3),
};

// Event results returned from handlers
enum _event_result
{
	EVENT_REMOVE, // remove the event from the queue. No other handlers will process it
	EVENT_CONTINUE, // Continue letting other handlers process the event (consume it)
};

enum _led_events
{
	LED_CAPSLOCK,
	LED_NUMLOCK,
};

enum _key_state
{
	KEY_RELEASED,
	KEY_PRESSED,
};

typedef struct _key_event
{
	u32 scancode;
	vkey_t key;
	u8 state;
} key_event_t;

/* Event information structure */
typedef struct _event
{
	u32 ev_type; // event type
	u32 ev_event; // event code
	u32 ev_value; // event data
	list_t ev_link; // link in the event list
} event_t;

/* Event handler event notification function */
typedef int(*event_handler_func_t)(event_t*, void*);

/* Event handler data */
typedef struct _event_handler
{
	u32 eh_mask; // which events we subscribe to
	event_handler_func_t eh_event; // the handler function
	void* eh_data; // the data to pass to the event handler
	list_t eh_link; // link in the handler list
} event_handler_t;

/* Initialize the event subsystem */
int event_init( void );

/* Raise a system event */
int event_raise(u32 type, u32 event, u32 value);
/* Register an event handler function */
event_handler_t* event_listen(u32 mask, event_handler_func_t event, void* data);
/* Remove a previous listen registration */
int event_unlisten(event_handler_t* handler);

void event_monitor(void* unused);

#endif