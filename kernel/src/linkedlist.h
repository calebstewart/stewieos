#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#include "kernel.h"

struct list;
typedef struct list list_t;

struct list
{
	struct list* prev;
	struct list* next;
};

#define LIST_INIT(name) { .prev = &name, .next = &name }
#define INIT_LIST(name) do{ (name)->prev = (name); (name)->next = (name); } while(0)

#define list_entry(list, type, member) ((type*)( (char*)(list) - (unsigned long)(&((type*)0)->member) ))

#define list_for_each(item, head) for( item = (head)->next; item != head; item = (item)->next )

// Is this an empty list?
static inline int list_empty(list_t* list)
{
	return (list->next == list);
}
// is this inside of another list?
static inline int list_inserted(list_t* list)
{
	return (list->next != list || list->prev != list);
}

static inline void _list_add(list_t* what, list_t* prev, list_t* next)
{
	prev->next = what;
	next->prev = what;
	what->prev = prev;
	what->next = next;
}

static inline void list_add(list_t* what, list_t* where)
{
	_list_add(what, where, where->next);
}

static inline void list_add_before(list_t* what, list_t* where)
{
	_list_add(what, where->prev, where);
}

static inline void _list_rem(list_t* prev, list_t* next)
{
	prev->next = next;
	next->prev = prev;
}

static inline void list_rem(list_t* what)
{
	_list_rem(what->prev, what->next);
	INIT_LIST(what);
}

#endif