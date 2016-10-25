#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#include "stewieos/kernel.h"

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

#define list_for_each_entry(item, head, type, member, entry) for( item = (head)->next, entry = list_entry(item, type, member); item != head; item = (item)->next, entry = list_entry(item, type, member) )

#define list_finish_for_each(item, head) item = (head)->prev

#define list_next(item, type, member, head) ( (item)->next == head ? (type*)(NULL) : list_entry((item)->next, type, member) )

#define list_is_last(item, head) ((item)->next == head)

#define list_is_first(item, head) ( (head)->next == item )

// is item the only item in the list contained at head?
#define list_is_lonely(item, head) ((item)->next == head && (item)->prev == head)

// If the item is initialized but not inserted, it will be the only item
// in its own list (prev/next will point to self), so list_is_lonely on
// itself will return true, which means it is not inserted, and we will
// return false.
#define list_inserted(item) !list_is_lonely(item,item)

// Returns -1 for a < b, 0 for a == b, and 1 for a > b
typedef int(*list_compare_func_t)(list_t*, list_t*);

static inline list_t* list_first(list_t* head)
{
	return head->next;
}
static inline list_t* list_last(list_t* head)
{
	return head->prev;
}

// Is this an empty list?
static inline int list_empty(list_t* list)
{
	return (list->next == list);
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

static inline void list_add_ordered(list_t* what,
									list_t* where,
									list_compare_func_t compare)
{
	if( list_empty(where) ){
		list_add(what, where);
		return;
	}
	list_t* item;
	list_for_each(item, where){
		if( compare(what, item) <= 0 ){
			list_add_before(what, item);
			return;
		}
	}
	list_add_before(what, where);
}
#endif