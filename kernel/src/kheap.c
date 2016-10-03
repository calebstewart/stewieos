#include "kheap.h"
#include "paging.h" 
#include "pmm.h"

// The global kernel heap
heap_t kernel_heap;
// Kernel memory allocation and freeing global functions
extern void*(*kmalloc_int)(size_t,u32*,int);
extern void(*kfree_int)(void*);

// Internal use only
void heap_free_nolock(heap_t* heap, void* data);

// Initialize the heap structure and allocate the initial
// memory required for the heap allocator.
void heap_init(heap_t* heap, void* start, void* end)
{

	// Initialize the contents of the heap
	heap->memstart = start;
	heap->memend = start;
	heap->memmax = end;
	spin_init(&heap->lock);
	INIT_LIST(&heap->holes);

	// This will expand by the minimum amount (PAGE_SIZE, likely)
	heap_expand(heap, 1);

	// Make sure the global heap allocator is setup
	kmalloc_int = kheap_alloc;
	kfree_int = kheap_free;
}

void* kheap_alloc(size_t size, u32* phys, int align)
{
	// Allocate the new memory block
	void* result = heap_alloc(&kernel_heap, size, align);

	// Store the physical address as well
	if( phys && result ){
		get_physical_addr(curdir, result, phys);
	}

	// Return the reuslt
	return result;
}

void kheap_free(void* data)
{
	heap_free(&kernel_heap, data);
}

// Expand the heap (up to the max)
// This function will expand at a minimum by one page size.
header_t* heap_expand(heap_t* heap, size_t ammnt)
{
	// Out of memory!
	if( heap->memmax == heap->memend ){
		return NULL;
	}

	// At least a page, and round up to a page
	if( PAGE_OFFSET(ammnt) != 0 ){
		ammnt = PAGE_ALIGN(ammnt) + 0x1000;
	}

	// We can only go to memmax
	if( ((size_t)heap->memstart + ammnt) > (size_t)heap->memmax ){
		ammnt = (size_t)heap->memmax - (size_t)heap->memstart;
	}

	// Allocate the amount memory we need, in pages (possible extra)
	u32 npages = ammnt / PAGE_SIZE;

	for(u32 p = 0; p < npages; ++p){
		page_t* page = get_page((void*)((u32)heap->memend + p*PAGE_SIZE), 0, kerndir);
		alloc_frame(page, 0, 1);
		invalidate_page((void*)((u32)heap->memend + p*PAGE_SIZE));
	}

	// Allocate the required pages
	// for( void* p = heap->memend;
	// 	(size_t)p < ((size_t)heap->memend + ammnt);
	// 	p = (void*)((size_t)p + PAGE_SIZE))
	// {
	// 	alloc_frame(get_page(p, 0, kerndir), 0, 1);
	// 	invalidate_page(p);
	// }

	// The new header
	header_t* head = (header_t*)heap->memend;
	head_init(head, ammnt);
	// head->magic = KHEAP_MAGIC;
	// head->size = ammnt;
	// INIT_LIST(&head->link);
	// // The new footer
	// //footer_t* foot = heap_foot(head); //(footer_t*)((size_t)heap + ammnt - sizeof(footer_t));
	// heap_foot(head)->magic = KHEAP_MAGIC;
	// heap_foot(head)->header = head;

	// Adjust the end pointer
	heap->memend = (void*)((size_t)heap->memend + ammnt);

	// Insert the new memory into the hole list
	//heap_add_hole(heap, head);
	heap_free_nolock(heap, heap_addr(head));

	return head;

}

// Locate a free block to hold the specified size
header_t* heap_locate(heap_t* heap, size_t reqd, int align)
{
	list_t* item;
	header_t* head;
	list_for_each_entry(item, &heap->holes, header_t, link, head) {
		// Check for broken blocks
		if( head->magic != KHEAP_MAGIC || heap_foot(head)->magic != KHEAP_MAGIC ){
			syslog(KERN_ERR, "heap: memory corruption at %p", head);
			list_rem(item);
			return heap_locate(heap, reqd, align);
		}
		// Big enough?
		if( head->size >= (reqd + KHEAP_OVHD) ){
			// attempt to align/trim
			header_t* temp = heap_trim(heap, head, reqd, align);
			// unsuccessfull -> couldn't align, keep looking
			if( temp == NULL ){
				//item = &heap->holes;
				continue;
			} else {
				return temp;
			}
		}
	}

	// Try and aquire more memory
	head = heap_expand(heap, reqd+KHEAP_OVHD);
	if( head == NULL || head->size < (reqd + KHEAP_OVHD) ){
		return NULL;
	}

	return heap_locate(heap, reqd, align);

	// Attempt to trim
	head = heap_trim(heap, head, reqd, align);
	if( head == NULL ){
		return NULL;
	}

	return head;
}

// Trim a block and align to a page boundary if possible.
// Returns:
//		- `head` if asked to align and successfull.
//		- possibly a new header if the block was split to align.
//		- NULL on failure (unable to align block, not enough rooom)
//			* On failure, the heap remains unchanged.
//			* The hole list is untouched.
header_t* heap_trim(heap_t* heap, header_t* head, size_t reqd, int align)
{
	// No space for a trim and no need to align
	if( head->size <= (reqd + KHEAP_OVHD*2) && !align ){
		return head;
	}

	if( align && !PAGE_ALIGNED(((size_t)head + sizeof(header_t))) )
	{
		header_t* temp = (header_t*)(PAGE_ALIGN((size_t)head) + PAGE_SIZE - sizeof(header_t));
		// Can't align, because we can't split this block appropriately
		if( ((size_t)temp - (size_t)head) <= KHEAP_OVHD ){
			return NULL;
		}
		// Can't align, because not enough space after alignment.
		if( (head->size - ((size_t)temp - (size_t)head)) < (reqd+KHEAP_OVHD) ){
			return NULL;
		}

		// Initialize the aligned header
		head_init(temp, head->size-((size_t)temp - (size_t)head));
		// temp->size = head->size - ((size_t)temp - (size_t)head);
		// temp->magic = KHEAP_MAGIC;
		// INIT_LIST(&temp->link);
		// heap_foot(temp)->magic = KHEAP_MAGIC;
		// heap_foot(temp)->header = temp;

		// Resize and remove the header
		list_rem(&head->link);
		head_init(head, (size_t)temp - (size_t)head);
		// head->size = ((size_t)temp - (size_t)head);
		// heap_foot(head)->magic = KHEAP_MAGIC;
		// heap_foot(head)->header = head;
		// list_rem(&head->link);

		// Reinsert the header and the temp header
		heap_free_nolock(heap, heap_addr(head));
		heap_add_hole(heap, temp);
		// heap_free_nolock(heap, heap_addr(head));
		// heap_free_nolock(heap, heap_addr(temp));

		// The temp header is now aligned and is our primary
		head = temp;
	}

	// Can't trim, not enough space.
	if( head->size < (reqd+KHEAP_OVHD+KHEAP_OVHD) ){
		return head;
	}

	// Initialize the unused/trimmed block
	header_t* temp = (header_t*)( (size_t)head + reqd + KHEAP_OVHD );
	head_init(temp, head->size - reqd - KHEAP_OVHD);
	// temp->magic = KHEAP_MAGIC;
	// temp->size = ( head->size - reqd - KHEAP_OVHD );
	// heap_foot(temp)->magic = KHEAP_MAGIC;
	// heap_foot(temp)->header = temp;

	// Trim the block
	list_rem(&head->link);
	head_init(head, reqd + KHEAP_OVHD);
	// head->size = reqd + KHEAP_OVHD;
	// heap_foot(head)->magic = KHEAP_MAGIC;
	// heap_foot(head)->header = head;

	// Insert the trimmed block into the list
	//heap_add_hole(heap, temp);
	//heap_free_nolock(heap, heap_addr(head));
	heap_free_nolock(heap, heap_addr(temp));
	heap_add_hole(heap, head);

	return head;
}

void* heap_alloc(heap_t* heap, size_t reqd, int align)
{
	// Lock the heap
	spin_lock(&heap->lock);

	// Find a block big enough for us that is aligned
	header_t* head = heap_locate(heap, reqd, align);
	if( head == NULL ){
		syslog(KERN_PANIC, "out of memory!");
		return NULL;
	}

	// Remove from the hole list
	list_rem(&head->link);

	// Unlock the heap
	spin_unlock(&heap->lock);

	// Return the address
	return heap_addr(head);
}

// Used internally. Basically just places the block into the
// free block list
void heap_free_nolock(heap_t* heap, void* addr)
{
//	list_t* item;
//	header_t* entry;
	header_t* head;

	// Invalid address
	if( (size_t)addr < (size_t)heap->memstart || (size_t)addr >= (size_t)heap->memend ){
		return;
	}

	// Find the header
	head = heap_head(addr);

	// Combine backwards
	while( !heap_is_first(heap, head) && list_inserted(&head_prev(head)->link) )
	{
		//syslog(KERN_WARN, "merging backwards");
		header_t* prev = head_prev(head);
		list_rem(&prev->link);
		head_init(prev, prev->size + head->size);
		head = prev;
	}

	// Combine forwards
	while( !heap_is_last(heap, head) && list_inserted(&head_next(head)->link) )
	{
		//syslog(KERN_WARN, "merging forwards");
		header_t* next = head_next(head);
		list_rem(&next->link);
		head_init(head, head->size + next->size);
	}

	// Either not a block or you've corrupted it
	if( head->magic != KHEAP_MAGIC ){
		return;
	}

	// If they messed that up, fix it...
	heap_foot(head)->magic = KHEAP_MAGIC;
	heap_foot(head)->header = head;
	INIT_LIST(&head->link);

	heap_add_hole(heap, head);

	//list_add_ordered(&head->link, &heap->holes, heap_compare_headers);

	// // Easy if the hole list is empty
	// if( list_empty(&heap->holes) ){
	// 	list_add(&head->link, &heap->holes);
	// } else {
	// 	// Add, sorted by block size
	// 	list_for_each_entry(item, &heap->holes, header_t, link, entry) 
	// 	{
	// 		if( entry->size >= head->size ){
	// 			list_add_before(&head->link, item);
	// 			break;
	// 		}
	// 		if( item->next == &heap->holes ){
	// 			list_add(&head->link, item);
	// 			break;
	// 		}
	// 	}
	// }

}

void heap_free(heap_t* heap, void* addr)
{
	// Lock the heap
	spin_lock(&heap->lock);
	// Actually free the block
	heap_free_nolock(heap, addr);
	// unlock the heap
	spin_unlock(&heap->lock);

}

int heap_compare_headers(list_t* v1, list_t* v2)
{
	if( list_entry(v1, header_t, link)->size < list_entry(v2, header_t, link)->size ) return -1;
	else if( list_entry(v1, header_t, link)->size == list_entry(v2, header_t, link)->size ) return 0;
	else return 1;
}