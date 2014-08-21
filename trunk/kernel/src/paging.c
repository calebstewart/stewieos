#include "paging.h"
#include "kmem.h"
#include "pmm.h"
#include "elf/elf32.h"

page_dir_t* kerndir = NULL;
page_dir_t* curdir = NULL;
extern u32* physical_frame;
extern u32 physical_frame_count;
extern u32 placement_address;

void initial_switch_dir(void* cr3);

/* function: init_paging
 * parameters:
 * 	none.
 * returns:
 * 	none.
 * description:
 * 	Initializing the paging structures
 * 	and initial memory handling.
 */
void init_paging( multiboot_info_t* mb )
{
	// Calculate the memory size
	u32 memory_size = 1024*(mb->mem_lower + 1024 + mb->mem_upper);
	
	if( mb->flags & MULTIBOOT_INFO_MODS )
	{
		mb->mods_addr += KERNEL_VIRTUAL_BASE;
		multiboot_module_t* module = (multiboot_module_t*)(mb->mods_addr);
		for(u32 i = 0; i < mb->mods_count; i++)
		{
			if( (module[i].mod_end+KERNEL_VIRTUAL_BASE) > placement_address ){
				placement_address = module[i].mod_end + KERNEL_VIRTUAL_BASE;
			}
		}
	}
	
	if( mb->flags & MULTIBOOT_INFO_ELF_SHDR )
	{
		mb->u.elf_sec.addr += KERNEL_VIRTUAL_BASE;
		if( (mb->u.elf_sec.addr + (mb->u.elf_sec.num*mb->u.elf_sec.size)) > placement_address ){
			placement_address = (mb->u.elf_sec.addr + (mb->u.elf_sec.num*mb->u.elf_sec.size));
		}
	}
	
	physical_frame_count = memory_size / 0x1000; // number of frames
	physical_frame = kmalloc((size_t)(physical_frame_count / 8));
	memset(physical_frame, (int)0xFFFFFFFF, physical_frame_count/8);
	
	if( !(mb->flags & MULTIBOOT_INFO_MEM_MAP) ) {
		printk("\n%2Verror: no memory map included from the bootloader! Unable to boot...\n");
		asm volatile("cli;hlt");
	}
	
	mb->mmap_addr += KERNEL_VIRTUAL_BASE;
	u32 mmap_end = mb->mmap_addr + mb->mmap_length;
	
	for(multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mb->mmap_addr;\
		((u32)mmap) < mmap_end;\
		mmap = (multiboot_memory_map_t*)((u32)mmap + mmap->size + sizeof(multiboot_uint32_t)))
	{
		printk("%1Vmemory block: %p-%p (%s)\n", (u32)mmap->addr, (u32)mmap->addr + (u32)mmap->len - 1, mmap->type == 1 ? "Available" : "Reserved");
		if( mmap->type != 1 ) continue;
		multiboot_uint64_t frame = (u32)mmap->addr;
		while( frame < (mmap->addr + mmap->len) ){
			release_frame((u32)frame);
			frame += 0x1000;
		}
	}

	//while(1);
	
	kerndir = (page_dir_t*)kmalloc_a(sizeof(page_dir_t));
	memset(kerndir, 0, sizeof(page_dir_t));
	curdir = kerndir;
	curdir->phys = (u32)( &curdir->tablePhys[0] ) - KERNEL_VIRTUAL_BASE;

	// allocate the initial page tables for the kernel heap
	for( u32 a = 0xE0000000; a < 0xEFFFF000; a += 0x1000 ){
		get_page((void*)a, 1, kerndir);
	}
	
	u32 i = KERNEL_VIRTUAL_BASE;
	while( i < placement_address )
	{
		// identity map as well as higher half map
		page_t* page_low = get_page((void*)i, 1, kerndir);
		page_t* page_hi = get_page((void*)(i-KERNEL_VIRTUAL_BASE), 1, kerndir);
		reserve_frame(i-KERNEL_VIRTUAL_BASE);
		page_hi->present = 1;
		page_hi->user = 1;
		page_hi->rw = 0;
		page_hi->frame = ((i-KERNEL_VIRTUAL_BASE) / 0x1000) & 0x000FFFFF;
		memcpy(page_low, page_hi, sizeof(page_t));
		i+=0x1000;
	}
	
	// allocate the initial frames for the kernel heap
	for( u32 a = 0xE0000000; a < 0xE0010000; a += 0x1000 ){
		page_t* page = get_page((void*)a, 0, kerndir);
		alloc_frame(page, 0, 1);
	}
	
	register_interrupt(0xE, page_fault);
	
	//switch_page_dir(curdir);
	
	initial_switch_dir((void*)curdir->phys);
	
	init_kheap(0xE0000000, 0xE0010000, 0xF0000000);
}

/* function: alloc_page
 * parameters:
 * 	dir		- the directory to modify
 * 	addr		- the address/page to allocate the memory to
 * 	user		- should this page be marked as user?
 * 	rw		- Should this page be read/write?
 * returns:
 * 	0 on success or a negative error value.
 * description:
 * 	allocates a new physical page to the virtual address.
 * 	If there is already a page allocated to the address,
 * 	the function returns immediately.
 */
int alloc_page(page_dir_t* dir, void* addr, int user, int rw)
{
	page_t* page = get_page(addr, 1, dir);

	if( page->frame != 0 ){
		return 0;
	}
	
	alloc_frame(page, user, rw);
	
	return 0;
}

/* function: get_page
 * parameters:
 * 	vpaddress 	- the virtual address
 * 	make		- whether or not to create the table if needed
 * 	dir		- the directory to modify
 * returns:
 * 	page_t*		- the page structure for vpaddress
 * description:
 * 	translates the virtual address into a logical one, and returns
 * 	the page information structure. If the table does not exist
 * 	and make is true, we will create one and return the empty
 * 	page structure, otherwise, we will return NULL (page structures
 * 	will never be at address 0).
 */
page_t* get_page(void* vpaddress, int make, page_dir_t* dir)
{
	// translate the address into directory and table
	// indices
	u32 address = (u32)vpaddress;
	u32 dir_idx = (address >> 22) & 0x3FF;
	u32 tbl_idx = (address >> 12) & 0x3FF;
	//u32 pge_off = (address >> 0 ) & 0xFFF;
	// if the table exists, return the page structure
	if( dir->table[dir_idx] ){
		return &dir->table[dir_idx]->page[tbl_idx];
	}
	// create it if asked and necessary
	if( make )
	{
		u32 tmp;
		dir->table[dir_idx] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &tmp);
		memset((void*)dir->table[dir_idx], 0, sizeof(page_table_t));
		dir->tablePhys[dir_idx] = (u32)(tmp | 0x7); // The address + PRESENT, RW, US
		return &dir->table[dir_idx]->page[tbl_idx];
	}
	// it doesn't exist, return NULL
	return NULL;
}

void* temporary_map(u32 addr, page_dir_t* dir)
{
	// calculate the directory index, table index, and page offset
	// e.g. the table, page, and offset numbers, respectively.
	u32 tbl = ((u32)addr >> 22) & 0x3ff;
	u32 pge = ((u32)addr >> 12) & 0x3ff;
	if( !dir->table[tbl] ){
		return NULL;
	}
	
	// grab the source page frame address
	page_t* src_page = &dir->table[tbl]->page[pge];
	// grab the destination page frame (calculate table, and page on the fly)
	// this is always the same page. We are just remapping it.
	page_t* dst_page = &curdir->table[(0xFFFFF000>>22)&0x3ff]->page[(0xFFFFF000>>12)&0x3ff];
	// set the page to point to the same frame
	dst_page->present = 1;
	dst_page->rw = 1;
	dst_page->user = 0;
	dst_page->frame = src_page->frame;
	// invalidate the TLB buffer for this page
	invalidate_page((u32*)0xFFFFF000);
	
	// Return the temporary buffer pointer
	return (void*)0xFFFFF000;
}

/* function: invalidate_page
 * parameters:
 * 	ptr		- the logical address of the page
 * returns:
 * 	none.
 * description:
 * 	invalidates the TLB entry for this page, requiring
 * 	a main memory lookup for the next access to this
 * 	page
 */
void invalidate_page(u32* ptr)
{
	// We also tell GCC that memory is invalidated. It prevents a
	// "nasty bug" according to OSDEV.org
	asm volatile( "invlpg %0" : : "m"(*ptr) : "memory" );	
}

/* function: switch_page_dir
 * parameters:
 * 	dir		- the directory to switch to
 * returns:
 * 	none.
 * description:
 * 	switches the hardware cr3 register to point to the new directory
 * 	and invalidates all pages.
 */
void switch_page_dir(page_dir_t* dir)
{
	curdir = dir;
	asm volatile("mov %0,%%cr3" :: "r"(curdir->phys));
	//u32 cr0;
	//asm volatile("mov %%cr0,%0" :"=r"(cr0));
	//cr0 |= 0x80000000;
	//asm volatile("mov %0,%%cr0" ::"r"(cr0));
}

/* function: page_fault
 * parameters:
 * 	regs		- the registers structure from the interrupt routine
 * returns:
 * 	this function does not return.
 * description:
 * 	This function is called when the Page Fault exception is caught.
 * 	it will dump some useful information to the kernel log, and halt
 * 	the machine.
 */
void page_fault(struct regs* regs)
{
	u32 address = 0;
	asm volatile ("mov %%cr2,%0" : "=r"(address));
	
	int present = !(regs->err & 0x1);
	int rw = regs->err & 0x2;
	int us = regs->err & 0x4;
	int reserved = regs->err & 0x8;
	//int id = regs->err & 0x10;
	
	printk("%2VPAGE FAULT ( %s%s%s%s)\nAddress: %p", present ? "present " : "", rw ? "read-only ":"", us?"user-mode ":"", reserved?"reserved ":"", address);
	printk("%2VError Code: 0x%X\n", regs->err);
	printk("%2VEFLAGS: 0x%X\n", regs->eflags);
	printk("%2VEIP: 0x%X\n", regs->eip);
	u32 cr0, cr3;
	asm volatile ("movl %%cr0,%0;": "=r"(cr0));
	asm volatile ("movl %%cr3,%0;": "=r"(cr3));
	printk("%2VCR0: 0x%X\n", cr0);
	printk("%2VCR3: 0x%X\n", cr3);
	printk("%2VCS: 0x%X\nDS: 0x%X\n", regs->cs, regs->ds);
	
	while(1);
}

/*
 * function: copy_page_table
 * parameters:
 * 	dest	- The destination directory
 * 	src	- the source directory
 * 	t	- the table index to copy
 * returns:
 * 	none.
 * description:
 * 	This function copies a table and its contents over into a new directory
 * 	The new table has the saame content and permissions as the old table.
 * 
 * 	If the table in dstdir does not exist, this function will create it.
 */
int copy_page_table(page_dir_t* dstdir, page_dir_t* srcdir, int t)
{
	if( !dstdir->table[t] )
	{
		u32 phys = 0;
		dstdir->table[t] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &phys);
		if(!dstdir->table[t]){
			printk("%2Vcopy_page_table: unable to allocate page table!");
			return -1;
		}
		dstdir->tablePhys[t] = phys | 0x7;
	}
	
	// this is to copy the physical frame
	void* temp_buffer = kmalloc(0x1000);
	
	// Copy every page 
	for(int p = 0; p < 1024; ++p)
	{
		page_t* dst = &dstdir->table[t]->page[p],
		      * src = &srcdir->table[t]->page[p];
		// initialize the page to zero
		memset(dst, 0, sizeof(page_t));
		// check for unnallocated page
		if( !src->present ){
			continue; // ignore this page
		}
		// Allocate a new frame, and use the same security settings
		// as the old frame
		alloc_frame(dst, src->user, src->rw);
		dst->accessed = src->accessed ? 1 : 0;
		dst->dirty = src->dirty ? 1 : 0;
		dst->unused = src->unused;
		// copy the actual data
		void* temp_map_ptr = temporary_map(((u32)t << 22)|((u32)p<<12), srcdir);
		memcpy(temp_buffer, temp_map_ptr, 0x1000);
		temp_map_ptr = temporary_map(((u32)t<<22)|((u32)p<<12), dstdir);
		memcpy(temp_map_ptr, temp_buffer, 0x1000);
		//copy_physical_frame((u32)(dst->frame << 12), (u32)(src->frame << 12));
	}
	// free the frame
	kfree(temp_buffer);
	
	return 0;
}
		
/* function: copy_page_dir
 * parameters:
 * 	src		- the source directory to copy
 * returns:
 * 	page_dir_t*	- a newly allocated copy of src
 * description:
 * 	copies every frame of src into a new destination directory.
 * 	If a frame is a kernel frame (i.e. exists in the kernel page table)
 * 	it is linked not copied.
 */
page_dir_t* copy_page_dir(page_dir_t* src)
{
	// Allocate the new directory structure
	u32 tmp;
	page_dir_t* dst = (page_dir_t*)kmalloc_ap(sizeof(page_dir_t), &tmp);
	// check for success
	if(!dst){
		printk("%2Vcopy_page_dir: unable to allocate new directory!\n");
		return NULL;
	}
	if( ((u32)dst) & 0xFFF ){
		printk("%2Verror: page directory not page aligned!\n");
		while(1);
	}
	memset((char*)dst + 0x1000, 0, 0x1004);
	memset(dst, 0, 0x1000);
	//memset(dst, 0, sizeof(page_dir_t));
	dst->phys = tmp;
	
	for(int t = 0; t < 1024; ++t)
	{
		if( !src->table[t] ){
			dst->table[t] = NULL;
			dst->tablePhys[t] = 0;
		} else if( src->table[t] == kerndir->table[t] ){
			dst->table[t] = src->table[t];
			dst->tablePhys[t] = src->tablePhys[t];
		} else {
			if( copy_page_table(dst, src, t) != 0 ){
				printk("%2Vcopy_page_dir: unable to copy page table!\n");
				return NULL;
			}
		}
	}
	
	return dst;
}