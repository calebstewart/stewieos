#include "pmm.h"

u32* physical_frame = NULL;
u32 physical_frame_count = 0;

u32 find_free_frame( void )
{
	for(u32 i = 0; i < (physical_frame_count/32); ++i)
	{
		if( physical_frame[i] == 0xFFFFFFFF ) continue;
		for(u32 m = 0; m < 32; m++){
			if( !(physical_frame[i] & (u32)(1<<m)) ){
				return i*4*8 + m;
			}
		}
	}
	return (u32)-1;
}

void reserve_frame(u32 idx)
{
	idx /= 0x1000; // get a frame index instead of a frame address
	physical_frame[(int)(idx/32)] |= (u32)(1 << (idx % 32));
}

void release_frame(u32 idx)
{
	idx /= 0x1000; // get a frame index instead of a frame address
	physical_frame[(int)(idx/32)] &= (u32)(~(1 << (idx % 32)));
}

void alloc_frame(page_t* page, int user, int rw)
{
	//printk("PHYSICAL_ADDRESS: %p\n", physical_frame);
	if( page->frame != 0 ){
		return;
	}
	
	u32 idx = find_free_frame();
	if( idx == (u32)-1 ){
		printk("%2VOUT OF MEMORY!\n");
		while(1);
	}
	reserve_frame(idx*0x1000);
	page->present = 1;
	page->user = user ? 1 : 0;
	page->rw = rw ? 1 : 0;
	page->frame = idx;
	return;
}

void clone_frame(page_t* dst, page_t* src)
{
	dst->present = 1;
	dst->user = src->user;
	dst->rw = src->rw;
	dst->frame = src->frame;
}

void free_frame(page_t* page) 
{
	if( !page->frame ) return;
	
	release_frame(page->frame);
	page->frame = 0;
}
