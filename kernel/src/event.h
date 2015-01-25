#ifndef _STEWIEOS_EVENT_H_
#define _STEWIEOS_EVENT_H_

#include "kernel.h"
#include "linkedlist.h"

enum _event_type
{
	ET_KEY = (1<<0),
	ET_ABS = (1<<1),
	ET_REL = (1<<2),
};

enum _key_events
{
	EVENT_KEYUP,
	EVENT_KEYDOWN,
	EVENT_SCANCODE
};

enum _key_state
{
	KEY_RELEASED,
	KEY_PRESSED,
};

typedef struct _key_event
{
	u32 scancode;
	u8 state;
} key_event_t;

/* Event information structure */
typedef struct _event
{
	u32 ev_type; // event type
	u32 ev_event; // event code
	void* ev_data; // event data
	size_t ev_length; // length of the data argument
} event_t;

/* Event handler event notification function */
typedef void(*event_handler_func_t)(event_t*, void*);

/* Event handler data */
typedef struct _event_handler
{
	u32 eh_mask; // which events we subscribe to
	event_handler_func_t eh_event; // the handler function
	void* eh_data; // the data to pass to the event handler
	list_t eh_link; // link in the handler list
} event_handler_t;

/* Raise a system event */
int event_raise(u32 type, u32 event, void* data, size_t length);
/* Register an event handler function */
int event_listen(u32 mask, event_handler_func_t event, void* data);
/* Remove a previous listen registration */
int event_unlisten(u32 mask, event_handler_func_t event);

#endif