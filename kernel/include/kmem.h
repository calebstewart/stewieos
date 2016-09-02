#ifndef _KMEM_H_
#define _KMEM_H_

#include "kernel.h"
#include "kheap.h"

#define kernel_alloc(type) (type*)kmalloc(sizeof(type))

/* Function: kmalloc* family
 * Parameters:
 * 	size_t size -- The size of the buffer to be allocated
 * 	u32*   phys -- A pointer to a buffer to hold the physical address of the data
 * Purpose:
 * 	Allocate small blocks of kernel memory
 * Notes:
 * 	Early in the boot sequence, this is a permenant allocation. 
 * 	The function is set to placement_* versions where the new
 * 	data is simply appended to the kernel data area
 */
void* kmalloc(size_t size);
void* kmalloc_a(size_t size);
void* kmalloc_p(size_t size, u32* phys);
void* kmalloc_ap(size_t size, u32* phys);

/* Function: kfree
 * Parameters:
 * 	void* data -- the data you wish to free
 * Purpose:
 * 	Free a chunk of data recently allocated with kmalloc
 * Notes:
 * 	kfree will only work with data allocated after the paging initialization.
 * 	before paging initialization, the kmalloc functions point to a permanent
 * 	allocation function.
 */
void kfree(void* data);

void init_kheap(u32, u32, u32);

#endif /* ifndef _KMEM_H_ */