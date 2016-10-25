#ifndef _KHEAP_H_
#define _KHEAP_H_

#include <sys/types.h>
#include "stewieos/linkedlist.h"
#include "stewieos/spinlock.h"

#define KHEAP_MAGIC 0x2BADB002
#define KHEAP_OVHD (sizeof(header_t) + sizeof(footer_t))
#define heap_addr(head) ((void*)((size_t)(head) + sizeof(header_t)))
#define heap_foot(head) ((footer_t*)( (size_t)head + head->size - sizeof(footer_t)))
#define heap_head(addr) ((header_t*)( (size_t)addr - sizeof(header_t)))
#define heap_is_first(heap, head) ((u32)(heap->memstart) == (u32)(head))
#define heap_is_last(heap, head) ((u32)(heap->memend) == ((u32)(head) + head->size))
#define head_prev(head)	(((footer_t*)( (size_t)head - sizeof(footer_t)))->header)
#define head_next(head) ((header_t*)( (size_t)head + head->size ))

typedef struct header
{
	size_t magic;
	size_t size;
	list_t link;
} header_t;

typedef struct footer
{
	size_t magic;
	header_t* header;
} footer_t;

typedef struct heap
{
	void* memstart;
	void* memend;
	void* memmax;
	list_t holes;
	spinlock_t lock;
} heap_t;

// Initialize the heap structure and hole list
void heap_init(heap_t* heap, void* start, void* end);
// Expand the heap by at least 1 page, and at most `reqd` (rounded up to page boundary)
header_t* heap_expand(heap_t* heap, size_t reqd);
// Locate a header to hold at least `minimum`
header_t* heap_locate(heap_t* heap, size_t reqd, int align);
// Trim and optionally align a header, return the same or new header
header_t* heap_trim(heap_t* heap, header_t* head, size_t reqd, int align);
// Allocate a new block of memory
void* heap_alloc(heap_t* heap, size_t size, int align);
// Deallocate/free a block of memory
void heap_free(heap_t* heap, void* data);
// Compare two heads based on header size (for ordered list generation)
int heap_compare_headers(list_t* v1, list_t* v2);

static inline void head_init(header_t* head, size_t size)
{
	head->size = size;
	head->magic = KHEAP_MAGIC;
	INIT_LIST(&head->link);
	heap_foot(head)->header = head;
	heap_foot(head)->magic = KHEAP_MAGIC;
}

static inline void heap_add_hole(heap_t* heap, header_t* head)
{
	head_init(head, head->size);
	list_add_ordered(&head->link, &heap->holes, heap_compare_headers);
}

// Kernel Specific heap functions
void* kheap_alloc(size_t size, u32* phys, int align);
void kheap_free(void* data);

// The kernel heap structure
extern heap_t kernel_heap;

#endif 
