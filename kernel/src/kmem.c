#include "kmem.h"
#include "linkedlist.h"
#include "pmm.h"
#include "paging.h"

/* struct heap_data
 * purpose: encapsulates the data overhead needed by the kernel heap
 */
struct heap_data
{
	//struct header* hole[512];
	list_t holes; // a linked list of holes sorted by size
	u32 endptr;
	u32 startptr;
	u32 maxptr;
};

/* struct header
 * purpose: a structure placed just before every memory block, holding magic number, size, and allocation status
 */
struct header
{
	u32 magic;
	u32 size;
	list_t link; // link in the list of holes (is a whole if list_empty returns false)
};

/* struct footer
 * purpose: a structure placed just after over memory block holding magic number and pointer to its header.
 */
struct footer
{
	u32 magic;
	struct header* header;
};

// Function Prototypes
void* placement_kmalloc(size_t size, u32* phys, int align);
void placement_kfree(void* ptr);
void* heap_kmalloc(size_t size, u32* phys, int align);
void  heap_kfree(void* ptr);

// Global variables
extern void* end; // defined by the linker (real type is void, but gcc doesn't like that)
u32 placement_address = (u32)( &end );
void*(*kmalloc_int)(size_t, u32*, int) = placement_kmalloc;
void (*kfree_int)(void*) = placement_kfree;

void* kmalloc(size_t size)
{
	return kmalloc_int(size, NULL, 0);
}
void* kmalloc_a(size_t size)
{
	return kmalloc_int(size, NULL, 1);
}
void* kmalloc_p(size_t size, u32* phys)
{
	return kmalloc_int(size, phys, 0);
}
void* kmalloc_ap(size_t size, u32* phys)
{
	return kmalloc_int(size, phys, 1);
}
void kfree(void* p)
{
	kfree_int(p);
}

void* placement_kmalloc(size_t size, u32* phys, int align)
{
	if( align && (placement_address & 0xFFFFF000) ){
		placement_address = (placement_address & 0xFFFFF000) + 0x1000;
	}
	if( phys ){
		*phys = placement_address - KERNEL_VIRTUAL_BASE;
	}
	void* ret = (void*)placement_address;
	placement_address += size;
	return ret;
}

void placement_kfree(void*ptr)
{
	UNUSED(ptr); // shut up, GCC!
	printk("%1Vkfree: warning: you cannot kfree before kernel heap initialization.\n");
}

#define STEWIEOS_HEAP_MAGIC 0x2BADB00B
#define STEWIEOS_HEAP_OVERHEAD (sizeof(struct heap_data)+sizeof(struct header)+sizeof(struct footer))

static struct heap_data* heap_data;	// the heap overhead structure
static size_t heap_end;			// ending address of the heap
static size_t heap_start;		// starting address of the heap (heap_data+sizeof(struct heap_data))
static size_t heap_max;			// max ending address of the heap

static struct header* heap_align_block(struct header* header, size_t size, int align);
static struct header* heap_find_hole(size_t size, int align);
static void heap_merge_next(struct header* header);
static struct header* heap_merge_prev(struct header* header);
static void heap_insert_hole(struct header* header);
static struct header* heap_expand(u32 length);

/* function: heap_align_block
 * purpose: splits a block so that it has one piece whose address is page aligned, if align is true.
 * 		it will split the block and return the header for the block that is page aligned
 * 		which could be the same header. Both headers are reinserted into the hole list.
 * parameters:
 * 	header -- the header for the block you want to split
 * 	size -- the needed size of the page aligned block
 * 	align -- whether or not toe align the block
 * return:
 * 	the header for the newly page aligned block or null on error
 * 	it will only fail if there is not enough space for 'size' bytes
 * 	after page aligning.
 */
static struct header* heap_align_block(struct header* header, size_t size, int align)
{
	if( !align ) return header; // no alignment needed
	
	u32 addr = ((u32)header + sizeof(struct header));				// This is the data pointer which will eventually be returned to the user (what we want to be page aligned)
	u32 head_addr = (addr & 0xFFFFF000) + 0x1000 - sizeof(struct header);		// New header address, where the header needs to be to make addr page aligned (addr - sizeof(struct header))
	u32 block_end = ((u32)header) + header->size;					// Address of the end of the block pointed to be header
	struct footer* new_foot = NULL;							// The new footer for both the old unaligned block and the new algined block
	struct header* new_head = NULL;							// The new header for the aligned block
	
	// the block is already page aligned
	if( (addr & 0xFFF) == 0 ) return header;
	// if there wasn't enough space before the next page boundary to fit a header and footer, so move to the next page
	if( (head_addr - (u32)header) < (sizeof(struct header)+sizeof(struct footer)) ){
		head_addr += 0x1000;
	}
	// After alignment, is there still enough space to hold our data?
	if( head_addr > block_end || (block_end - head_addr) < size ){
		return NULL; // nope, return
	}
	
	// initialize the new aligned block header
	new_head = ((struct header*)head_addr);
	new_head->magic = STEWIEOS_HEAP_MAGIC;
	INIT_LIST(&new_head->link);
	new_head->size = (block_end - head_addr);
	// initialize the new aligned block footer
	new_foot = ((struct footer*)( block_end - sizeof(struct footer)));
	new_foot->magic = STEWIEOS_HEAP_MAGIC;
	new_foot->header = new_head;
	
	// fix the unaligned block header
	list_rem(&header->link);
	header->size = (head_addr - (u32)header);
	header->magic = STEWIEOS_HEAP_MAGIC;
	// fix the unaligned block footer
	new_foot = (struct footer*)( (u32)header + header->size - sizeof(struct footer) );
	new_foot->magic = STEWIEOS_HEAP_MAGIC;
	new_foot->header = header;
	
	heap_insert_hole(header);
	heap_insert_hole(new_head);
	
	return new_head;
}

static void heap_split_block(struct header* header, size_t size)
{
	// is there enough room to split the block?
	if( size > header->size || (header->size - size) < (sizeof(struct header) + sizeof(struct footer)) ){
		return;
	}
	
	struct footer *new_foot = NULL;					// Pointer to the new block footer
	struct header *new_head = NULL;					// Pointer to the new block header

	// remove the old header from the hole list	
	list_rem(&header->link);

	// set up the new block's header
	new_head = (struct header*)( (u32)header + size );
	new_head->magic = STEWIEOS_HEAP_MAGIC;
	INIT_LIST(&new_head->link);
	new_head->size = header->size - size;
	// set up the new block's footer
	new_foot = (struct footer*)( (u32)new_head + new_head->size - sizeof(struct footer) );
	new_foot->magic = STEWIEOS_HEAP_MAGIC;
	new_foot->header = new_head;
	// resetup the old block's header and footer
	header->size = size;
	header->magic = STEWIEOS_HEAP_MAGIC;
	new_foot = (struct footer*)( (u32)header + header->size - sizeof(struct footer) );
	new_foot->magic = STEWIEOS_HEAP_MAGIC;
	new_foot->header = header;
	// re-insert the two headers into the hole list
	heap_insert_hole(header);
	heap_insert_hole(new_head);
	
	return;
}
	
static struct header* heap_find_hole(size_t size, int align)
{
	list_t* list = &heap_data->holes;
	list_t* item = NULL;
	list_for_each(item, list){
		struct header* entry = list_entry(item, struct header, link);
		if( entry->magic != STEWIEOS_HEAP_MAGIC ){
			printk("%2Vheap_find_hole: bad magic number for heap header %p!\n", entry);
			printk("%2V\tsystem halted.");
			asm volatile ("cli;hlt");
			return NULL;
		}
		// don't go any further, this entyr can't hold our data
		if( entry->size < size ){
			continue;
		}
		// Try to align the block on a page boundary and return the new header pointer
		struct header* new_head = heap_align_block(entry, size, align);
		// we couldn't align it, so we need to keep looking for a suitable header
		if(new_head == NULL){
			continue;
		}

		// we're now aligned if needed, lets try and free some space if possible
		heap_split_block(new_head, size);
		// we have a good block now!
		return new_head;
	}
	while( 1 )
	{
		struct header* head = heap_expand(size);
		struct header* new_head = heap_align_block(head, size, align);
		if( new_head == NULL ){
			continue;
		}
		heap_split_block(new_head, size);
		return new_head;
	}
}

static void heap_insert_hole(struct header* header)
{
	if( list_inserted(&header->link) ) return; // we are already in the hole list
	
	header->magic = STEWIEOS_HEAP_MAGIC;
	if( list_empty(&heap_data->holes) ){
		list_add(&header->link, &heap_data->holes);
		return;
	}
	list_t* item = NULL;
	list_for_each(item, &heap_data->holes) {
		struct header* entry = list_entry(item, struct header, link);
		if( header->size < entry->size ){
			list_add_before(&header->link, item);
			return;
		}
		if( item->next == &heap_data->holes ){
			list_add(&header->link, item); // add after the last item
			return;
			//break; // otherwise, we will read this as the next item and continue forever D:
		}
	}
}

void* heap_kmalloc(size_t size, u32* phys, int align)
{
	if( size == 0 ){
		return NULL;
	}
	
	struct header* header = heap_find_hole(size+sizeof(struct header)+sizeof(struct footer), align);
	if(!header)
	{
		printk("%1Vheap_kmalloc: there are no more holes big enough, and we haven't implemented heap_expand!\nheap_kmalloc: out of memory!\n");
		return NULL;
	}
	
	list_rem(&header->link);
	
	u32 data_addr = ((u32)header + sizeof(struct header));
	
	if( phys ){
		//*phys = ((u32)header + sizeof(struct header)) - KERNEL_VIRTUAL_BASE;
		page_t* page = get_page((void*)( data_addr & 0xFFFFF000 ), 0, kerndir);
		u32 frame = ((u32)(page->frame << 12)); // shift the frame address back
		frame += (u32)(data_addr & 0x00000FFF);
		*phys = frame;
	}
	
	if( (u32)header == 0xe000fab8 ){
		//while(1);
		debug_message("found the bitch.\n", 0);
	}
	
	return (void*)( (char*)(header) + sizeof(struct header) );
}

static void heap_merge_next(struct header* header)
{
	u32 head_end = ((u32)header) + header->size;
	if( head_end >= heap_end ) return; // we are the last hole, we can't merge forward
	
	struct footer* footer = (struct footer*)( head_end - sizeof(struct footer) );
	struct header* next_head = (struct header*)(head_end);
	// not a valid block, return
	if( next_head->magic != STEWIEOS_HEAP_MAGIC ){
		printk("%2Vheap_merge_next: corrupted heap header magic number for block %p.\n", header);
		return;
	}
	// not a hole
	if( !list_inserted(&next_head->link) ){
		return;
	}
	
	// merge the two blocks
	list_rem(&next_head->link); // remove the next block from the list 
	header->size += next_head->size; // increase the size of this block
	footer = (struct footer*)( (u32)header + header->size - sizeof(struct footer) );
	footer->header = header;
	footer->magic = STEWIEOS_HEAP_MAGIC;
	list_rem(&header->link); // remove this block (we need to transplant it for the new size
	
	heap_insert_hole(header); // reinsert into the correct location
}

/* function: heap_expand
 * purpose:
 * 	expands the heap by a given ammount, although it will
 * 	always expand by a multiple of the page boundary (4kB)
 * parameters:
 * 	length - how much more data you need (no need to be page aligned)
 * return value:
 * 	the newly created header (is already a hole when returned).
 */
static struct header* heap_expand(u32 length)
{
	// page align the length
	if( (length & 0xFFF) != 0 ){
		length = (length & 0xFFFFF000) + 0x1000;
	}
	
	// Check if we have the room to grow
	if( (heap_data->endptr+length) >= heap_data->maxptr ){
		printk("%2Vheap_expand: unable to further expand kernel heap!\n");
		asm volatile ("cli;hlt");
	}
	
	// Calculate the new header placement, and the counting
	// variable for allocation of new frames
	u32 p = heap_data->endptr;
	struct header* header = (struct header*)( heap_data->endptr );
	// Increase the heap data end pointer
	heap_data->endptr += length;
	// Allocate the necessary frames
	while( p < heap_data->endptr ){
		alloc_frame(get_page((void*)p, 0, kerndir), 0, 1);
		invalidate_page((void*)p);
		p += 0x1000;
	}
	
	// Setup the header
	header->size = length;
	header->magic = STEWIEOS_HEAP_MAGIC;
	INIT_LIST(&header->link);
	// Setup the footer
	struct footer* footer = (struct footer*)( (u32)header + header->size - sizeof(struct footer) );
	footer->header = header;
	footer->magic = STEWIEOS_HEAP_MAGIC;
	// Insert the new hole
	heap_insert_hole(header);
	
	// check if we can merge with the previous header and return
	// whatever merge_prev returns
	return heap_merge_prev(header);
}

static struct header* heap_merge_prev(struct header* header)
{
	if( ((u32)header) == (heap_start+sizeof(struct heap_data)) ) return header; // this is the start of the heap, we can't merge backward
	
	struct footer* prev_foot = (struct footer*)(((u32)header) - sizeof(struct footer));
	// previous block is invalid, can't safely merge
	if( prev_foot->magic != STEWIEOS_HEAP_MAGIC ){
		syslog(KERN_ERR, "heap_merge_prev: corrupt footer at 0x%p (magic=0x%X).", prev_foot, prev_foot->magic);
		syslog(KERN_ERR, "heap_merge_prev: possible header for corrupt foot: 0x%p.", prev_foot->header);
		return header;
	}
	struct header* prev_head = prev_foot->header;
	// previous block is invalid, can't safely merge
	if( prev_head->magic != STEWIEOS_HEAP_MAGIC ){
		printk("%2Vheap_merge_prev: corrupted heap header magic number for block %p.\n", prev_head);
		return header;
	}
	
	// previous block isn't a hole, we can't merge
	if( !list_inserted(&prev_head->link) ){
		return header;
	}
	
	list_rem(&header->link);
	list_rem(&prev_head->link);
	
	// eat the header!
	prev_head->size += header->size;
	prev_foot = (struct footer*)(((u32)prev_head) + prev_head->size - sizeof(struct footer));
	prev_foot->header = prev_head;
	
	heap_insert_hole(prev_head);
	
	return prev_head;
}

void heap_kfree(void* data)
{
	if( data == NULL ){
		return;
	}
	
	struct header* header = ((struct header*)( (char*)(data) - sizeof(struct header) ));
	
	if( (u32)header == 0xe000fab8 ){
		debug_message("found the bitch.",0);
	}
	
	if( header->magic != STEWIEOS_HEAP_MAGIC ){
		printk("%2Vheap_kfree: corrupted heap header magic number!\n");
		printk("%2Vheap_kfree: aborting data deallocation (unstable pointer).\n");
		return;
	}
	
	struct footer* footer = (struct footer*)( (u32)header + header->size - sizeof(struct footer) );
	footer->magic = STEWIEOS_HEAP_MAGIC;
	footer->header = header;
	
	heap_merge_next(header);
	header = heap_merge_prev(header);
	
	heap_insert_hole(header);
	
	return;
}

void init_kheap(u32 where, u32 end_addr, u32 max_addr)
{
	// check for valid where, and max_addr values
	if( where < KERNEL_VIRTUAL_BASE ){
		printk("%2Vinit_kheap: starting heap address must be less than %#X\n", KERNEL_VIRTUAL_BASE);
		return;
	}
	if( max_addr > 0xF0000000 ){
		printk("%2Vinit_kheap: maximum heap address may not exceed %#X\n", 0xF0000000);
		return;
	}
	if( (end_addr - where) < STEWIEOS_HEAP_OVERHEAD ){
		printk("%2Vinit_kheap: you must provide at least %d bytes to accomodate the heap overhead.\n", STEWIEOS_HEAP_OVERHEAD);
		return;
	}
	

	// initialize the heap data structure
	struct heap_data* data = (struct heap_data*)(where);
	memset(data, 0, sizeof(struct heap_data));
	heap_data = data;
	heap_data->endptr = end_addr;
	heap_data->startptr = where;
	heap_data->maxptr = where + 0x0FFFFFFF;
	INIT_LIST(&heap_data->holes);
	
	// initialize the first header as a hole in the list
	struct header* header = (struct header*)( where + sizeof(struct heap_data) );
	header->size = (end_addr-where) - sizeof(struct heap_data);
	header->magic = STEWIEOS_HEAP_MAGIC;
	INIT_LIST(&header->link);
	list_add(&header->link, &data->holes);
	// initialize the headers footer, pointing back
	struct footer* footer = (struct footer*)( end_addr - sizeof(struct footer) );
	footer->magic = STEWIEOS_HEAP_MAGIC;
	footer->header = header;
	
	kmalloc_int = heap_kmalloc;
	kfree_int = heap_kfree;
	heap_start = where;
	heap_end = end_addr;
	heap_max = max_addr;
}
	