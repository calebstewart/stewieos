#include "paging.h"
#include "kmem.h"
#include "pmm.h"

page_dir_t* kerndir = NULL;
page_dir_t* curdir = NULL;
extern u32* physical_frame;
extern u32 physical_frame_count;
extern u32 placement_address;

void initial_switch_dir(void* cr3);

void init_paging( void )
{
	// 32MB of memory for now... we will fix it later
	u32 memory_size = 0x1000000;
	
	physical_frame_count = memory_size / 0x1000; // number of frames
	physical_frame = kmalloc((size_t)(physical_frame_count / 8));
	memset(physical_frame, 0, physical_frame_count/8);
	
	kerndir = (page_dir_t*)kmalloc_a(sizeof(page_dir_t));
	memset(kerndir, 0, sizeof(page_dir_t));
	curdir = kerndir;
	curdir->phys = (u32)( &curdir->tablePhys[0] ) - KERNEL_VIRTUAL_BASE;
	
	u32 i = KERNEL_VIRTUAL_BASE;
	while( i < placement_address )
	{
		// get a new frame and map it
		alloc_frame(get_page((void*)i, 1, kerndir), 1, 0);
		// identity map the same frame
		clone_frame(get_page((void*)(i - KERNEL_VIRTUAL_BASE), 1, kerndir), get_page((void*)i, 1, kerndir));
		i+=0x1000;
	}
	
	register_interrupt(0xE, page_fault);
	
	//switch_page_dir(curdir);
	
	initial_switch_dir((void*)curdir->phys);
}

page_t* get_page(void* vpaddress, int make, page_dir_t* dir)
{
	u32 address = (u32)vpaddress;
	address /= 0x1000;
	u32 tbl = address / 1024;
	if( dir->table[tbl] ){
		return &dir->table[tbl]->page[address%1024];
	}
	if( make )
	{
		u32 tmp;
		dir->table[tbl] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &tmp);
		memset((void*)dir->table[tbl], 0, 0x1000);
		dir->tablePhys[tbl] = (u32)(tmp | 0x7); // The address + PRESENT, RW, US
		return &dir->table[tbl]->page[address % 1024];
	}
	return NULL;
}

void switch_page_dir(page_dir_t* dir)
{
	curdir = dir;
	asm volatile("mov %0,%%cr3" :: "r"(curdir->phys));
	u32 cr0;
	asm volatile("mov %%cr0,%0" :"=r"(cr0));
	cr0 |= 0x80000000;
	asm volatile("mov %0,%%cr0" ::"r"(cr0));
}

void page_fault(struct regs* regs)
{
	u32 address = 0;
	asm volatile ("mov %%cr2,%0" : "=r"(address));
	
	int present = !(regs->err & 0x1);
	int rw = regs->err & 0x2;
	int us = regs->err & 0x4;
	int reserved = regs->err & 0x8;
	//int id = regs->err & 0x10;
	
	printk("%2VPAGE FAULT ( %s%s%s%s)\n\tAddress: %p", present ? "present " : "", rw ? "read-only ":"", us?"user-mode ":"", reserved?"reserved ":"", address);
	while(1);
}