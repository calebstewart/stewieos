#include "event.h"
#include "spinlock.h"
#include "error.h"
#include "kmem.h"
#include "task.h"
#include "sem.h"

//static spinlock_t event_lock = 0;
static list_t event_handlers = LIST_INIT(event_handlers);
static list_t event_list = LIST_INIT(event_list);
mutex_t* event_mutex = NULL, *event_handler_mutex = NULL;
pid_t event_monitor_pid;

int event_init( void )
{
	// create the event mutex
	event_mutex = mutex_alloc();
	if( event_mutex == NULL ){
		return -ENOMEM;
	}

	// create the event handler mutex
	event_handler_mutex = mutex_alloc();
	if( event_mutex == NULL ){
		return -ENOMEM;
	}
	
	// start the event process
	event_monitor_pid = worker_spawn(event_monitor, event_mutex);
	
	return 0;
}

void event_monitor(void* data ATTR((unused)))
{
	list_t* item;
	event_handler_t* handler;
	int code = 0;

	while( 1 )
	{
		// sleep while there are no events
		if( list_empty(&event_list) ){
			task_wait(current, TF_WAITIO);
		}

		// Quickly lock, grab the next event and unlock
		mutex_lock(event_mutex, SEM_FOREVER);
		event_t* event = list_entry(list_last(&event_list), event_t, ev_link);
		list_rem(&event->ev_link);
		mutex_unlock(event_mutex);

		// make sure we don't add or remove handlers while we traverse the list
		mutex_lock(event_handler_mutex, SEM_FOREVER);

		// Iterate over all event handlers and dispatch the message to the appropriate ones
		list_for_each_entry(item, &event_handlers, event_handler_t, eh_link, handler) {
			if( (handler->eh_mask & event->ev_type) == 0 ) continue;

			// The handler can request to skip all of the following handlers (consume the event)
			switch((code = handler->eh_event(event, handler->eh_data)))
			{
				case EVENT_REMOVE:
					list_finish_for_each(item, &event_handlers);
					break;
				case EVENT_CONTINUE:
					break;
				default:
					syslog(KERN_WARN, "bad return code %d from event handler", code);
			}
		}

		// Unlock the handler mutex
		mutex_unlock(event_handler_mutex);

		// Release the event structure
		kfree(event);

	}

}

int event_raise(u32 type, u32 event_code, u32 value)
{
	if( BITCOUNT(type) != 1 ){
		return -EINVAL;
	}
	
	// construct the event
	event_t* event = kernel_alloc(event_t);
	if( event == NULL ){
		syslog(KERN_ERR, "out of memory for event allocation!");
		return -ENOMEM;
	}
	event->ev_type = type;
	event->ev_event = event_code;
	event->ev_value = value;
	INIT_LIST(&event->ev_link);

	// Lock the event matrix
	mutex_lock(event_mutex, SEM_FOREVER);

	// Add the event to the queue
	list_add(&event->ev_link, &event_list);

	// wake up the task if this is the first event in queue
	if( list_is_lonely(&event->ev_link, &event_list) ){
		task_wakeup(task_lookup(event_monitor_pid));
	}

	// unlock the list
	mutex_unlock(event_mutex);
	
	return 0;
}

event_handler_t* event_listen(u32 mask, event_handler_func_t handler_func, void* data)
{
	// we need to listen to at least one event
	if( mask == 0 ){
		return ERR_PTR(-EINVAL);
	}
	
	event_handler_t* handler = (event_handler_t*)kmalloc(sizeof(event_handler_t));
	if( !handler ){
		return ERR_PTR(-ENOMEM);
	}
	
	// Clea the memory
	memset(handler, 0, sizeof(*handler));
	
	// Fill the structure
	handler->eh_mask = mask;
	handler->eh_event = handler_func;
	handler->eh_data = data;
	INIT_LIST(&handler->eh_link);
	
	mutex_lock(event_handler_mutex, SEM_FOREVER);
	
	// Add to the list
	list_add(&handler->eh_link, &event_handlers);
	
	mutex_unlock(event_handler_mutex);
	
	return handler;
}

int event_unlisten(event_handler_t* handler)
{
	if( handler == NULL ){
		return -EINVAL;
	}

	mutex_lock(event_handler_mutex, SEM_FOREVER);

	list_rem(&handler->eh_link);
	kfree(handler);
	
	mutex_unlock(event_handler_mutex);
	
	return 0;
}