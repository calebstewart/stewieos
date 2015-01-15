#ifndef _STEWIEOS_EVENT_H_
#define _STEWIEOS_EVENT_H_

#include "kernel.h"

/* Event information structure */
typedef struct _event
{
	u32 ev_type;
	u32 ev_event;
	void* ev_data;
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
int event_raise(u32 type, u32 event, void* data);
/* Register an event handler function */
int event_listen(u32 mask, event_handler_func_t event, void* data);

#endif