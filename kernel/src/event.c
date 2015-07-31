#include "event.h"
#include "spinlock.h"
#include "error.h"
#include "kmem.h"

//static spinlock_t event_lock = 0;
static list_t event_handlers = LIST_INIT(event_handlers);

static inline int _count_bits(u32 a)
{
	int result = 0;
	for(int i = 0; i < 32; ++i){
		if( (a&0x1) ) result++;
		a = a >> 1;
	}
	return result;
}

int event_raise(u32 type, u32 event_code, u32 value)
{
	if( _count_bits(type) != 1 ){
		return -EINVAL;
	}
	
	list_t* iter;
	event_t event = {
		.ev_type = type,
		.ev_event = event_code,
		.ev_value = value,
	};
	
	list_for_each(iter, &event_handlers){
		event_handler_t* handler = list_entry(iter, event_handler_t, eh_link);
		if( (handler->eh_mask & type) != 0 ){
			handler->eh_event(&event, handler->eh_data);
		}
	}
	
	return 0;
}

int event_listen(u32 mask, event_handler_func_t handler_func, void* data)
{
	// we need to listen to at least one event
	if( mask == 0 ){
		return -EINVAL;
	}
	
	event_handler_t* handler = (event_handler_t*)kmalloc(sizeof(event_handler_t));
	if( !handler ){
		return -ENOMEM;
	}
	
	// Clea the memory
	memset(handler, 0, sizeof(*handler));
	
	// Fill the structure
	handler->eh_mask = mask;
	handler->eh_event = handler_func;
	handler->eh_data = data;
	INIT_LIST(&handler->eh_link);
	
	//spin_lock(&event_lock);
	
	// Add to the list
	list_add(&handler->eh_link, &event_handlers);
	
	//spin_unlock(&event_lock);
	
	return 0;
}

int event_unlisten(u32 mask, event_handler_func_t handler_func)
{
	if( mask == 0 ){
		return -1;
	}
	
	list_t* iter;
	
	list_for_each(iter, &event_handlers){
		event_handler_t* handler = list_entry(iter, event_handler_t, eh_link);
		if( handler->eh_mask == mask && handler->eh_event == handler_func ){
			list_rem(iter);
			return 0;
		}
	}
	
	return -1;
}