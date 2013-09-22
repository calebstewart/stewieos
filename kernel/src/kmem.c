#include "kmem.h"

// Function Prototypes
void* placement_kmalloc(size_t size);
void* placement_kmalloc_a(size_t size);
void* placement_kmalloc_p(size_t size, u32* phys);
void* placement_kmalloc_ap(size_t size, u32* phys);

// Global variables
extern void* end; // defined by the linker (real type is void, but gcc doesn't like that)
u32 placement_address = (u32)( &end );
void*(*kmalloc)(size_t) = placement_kmalloc;
void*(*kmalloc_a)(size_t) = placement_kmalloc_a;
void*(*kmalloc_p)(size_t, u32*) = placement_kmalloc_p;
void*(*kmalloc_ap)(size_t, u32*) = placement_kmalloc_ap;

/* Function: placement_kmalloc
 * Parameters:
 * 	size_t size -- Size of the buffer to allocate
 * Purpose:
 * 	permanently allocate blocks of data early in the boot sequence.
 */
void* placement_kmalloc(size_t size)
{
	void* ret = (void*)(placement_address);
	placement_address += size;
	return ret;
}

/* Function: placement_kmalloc_a
 * Parameters:
 * 	size_t size -- Size of the buffer to allocate
 * Purpose:
 * 	permanently allocate a page aligned block of data early in the boot sequence.
 */
void* placement_kmalloc_a(size_t size)
{
	if( placement_address & 0xFFFFF000 ){
		placement_address = (placement_address & 0xFFFFF000) + 0x1000;
	}
	void* ret = (void*)(placement_address);
	placement_address += size;
	return ret;
}

/* Function: placement_kmalloc_p
 * Parameters:
 * 	size_t size -- Size of the buffer to allocate
 * 	u32*   phys -- a pointer to hold the physical address of the buffer
 * Purpose:
 * 	permanently allocate blocks of data early in the boot sequence.
 * Note:
 * 	With the placement version, the physical and virtual addresses are the same.
 */
void* placement_kmalloc_p(size_t size, u32* phys)
{
	*phys = placement_address;
	void* ret = (void*)(placement_address);
	placement_address += size;
	return ret;
}

/* Function: placement_kmalloc_p
 * Parameters:
 * 	size_t size -- Size of the buffer to allocate
 * 	u32*   phys -- a pointer to hold the physical address of the buffer
 * Purpose:
 * 	permanently allocate blocks of data early in the boot sequence.
 * Note:
 * 	With the placement version, the physical and virtual addresses are the same.
 */
void* placement_kmalloc_ap(size_t size, u32* phys)
{
	if( placement_address & 0xFFFFF000 ){
		placement_address = (placement_address & 0xFFFFF000) + 0x1000;
	}
	*phys = placement_address;
	void* ret = (void*)(placement_address);
	placement_address += size;
	return ret;
}

