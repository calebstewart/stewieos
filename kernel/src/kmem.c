#include "kmem.h"

// Function Prototypes
void* placement_kmalloc(size_t size, u32* phys, int align);
void placement_kfree(void* ptr);
void* heap_kmalloc(size_t size, u32* phys, int align);
void* heap_kfree(void* ptr);

// Global variables
extern void* end; // defined by the linker (real type is void, but gcc doesn't like that)
u32 placement_address = (u32)( &end );
void*(*kmalloc_int)(size_t, u32*, int) = placement_kmalloc;

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

struct heap_data
{
	struct header* hole[512];
};

struct header
{
	u32 magic;
	u32 size;
	u8 hole;
};
struct footer
{
	u32 magic;
	struct header* header;
};

#define STEWIEOS_HEAP_MAGIC 0x2BADB00B
#define STEWIEOS_HEAP_OVERHEAD (sizeof(struct heap_data)+sizeof(struct header)+sizeof(struct footer))

void init_kheap(u32, u32);
void init_kheap(u32 where, u32 max_addr)
{
	// check for valid where, and max_addr values
	if( where > STEWIEOS_USER_BASE ){
		printk("%2Vinit_kheap: starting heap address must be less than %#X\n", STEWIEOS_USER_BASE);
		return;
	}
	if( max_addr >= STEWIEOS_USER_BASE ){
		printk("%2Vinit_kheap: maximum heap address may not exceed %#X\n", STEWIEOS_USER_BASE);
		return;
	}
	if( (max_addr - where) < STEWIEOS_HEAP_OVERHEAD ){
		printk("%2Vinit_kheap: you must provide at least %d bytes to accomodate the heap overhead.\n", STEWIEOS_HEAP_OVERHEAD);
		return;
	}
	
	struct heap_data* data = (struct heap_data*)(where);
	memset(data, 0, sizeof(struct heap_data));
	
	data->hole[0] = (struct header*)( where + sizeof(struct heap_data) );
	data->hole[0]->hole = 1;
	data->hole[0]->size = (max_addr - where) - sizeof(struct heap_data);
	data->hole[0]->magic = STEWIEOS_HEAP_MAGIC;
	struct footer* footer = (struct footer*)( max_addr - sizeof(struct footer) );
	footer->magic = STEWIEOS_HEAP_MAGIC;
	footer->header = data->hole[0];
	
}
	