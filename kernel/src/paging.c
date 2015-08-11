#include "paging.h"
#include "kmem.h"
#include "pmm.h"
#include "elf/elf32.h"
#include "task.h"
#include "error.h"

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
		Elf32_Shdr* shdr = (Elf32_Shdr*)mb->u.elf_sec.addr;
		for(size_t i = 0; i < mb->u.elf_sec.num; ++i){
			if( shdr[i].sh_type == SHT_SYMTAB || shdr[i].sh_type == SHT_STRTAB ){
				if( (shdr[i].sh_addr+KERNEL_VIRTUAL_BASE+shdr[i].sh_size) > placement_address ){
					placement_address = shdr[i].sh_addr+KERNEL_VIRTUAL_BASE+shdr[i].sh_size;
				}
			}
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
	for( u32 a = 0xD0000000; a < 0xF0000000; a += 0x1000 ){
		get_page((void*)a, 1, kerndir);
	}
	
// 	for(u32 a = 0xF0000000; a >= 0xF0000000; a += 0x1000){
// 		get_page((void*)a, 1, kerndir);
// 	}
	
	u32 i = KERNEL_VIRTUAL_BASE;
	while( i < placement_address )
	{
		// identity map as well as higher half map
		page_t* page_hi = get_page((void*)i, 1, kerndir);
		page_t* page_low = get_page((void*)(i-KERNEL_VIRTUAL_BASE), 1, kerndir);
		reserve_frame(i-KERNEL_VIRTUAL_BASE);
		page_hi->present = 1;
		page_hi->user = 1;
		page_hi->rw = 0;
		page_hi->frame = ((i-KERNEL_VIRTUAL_BASE) / 0x1000) & 0x000FFFFF;
		memcpy(page_low, page_hi, sizeof(page_t));
		i+=0x1000;
	}
	
	// allocate the actual initial pages for the kernel heap
	for( u32 a = 0xD0000000; a < 0xD0010000; a += 0x1000 ){
		page_t* page = get_page((void*)a, 0, kerndir);
		alloc_frame(page, 0, 1);
	}
	
	register_interrupt(0xE, page_fault);
	
	//switch_page_dir(curdir);
	
	initial_switch_dir((void*)curdir->phys);
	
	for( u32 a = 0x00100000; a < KERNEL_VIRTUAL_BASE; a += 0x1000){
		u32 dir_idx = (a >> 22) & 0x3FF;
		
		if( !kerndir->table[dir_idx] ) continue;
		
		for(u32 p = 0; p < 1024; ++p){
			free_frame(&kerndir->table[dir_idx]->page[p]);
		}
		kerndir->table[dir_idx] = NULL;
		kerndir->tablePhys[dir_idx] = 0;
		
// 		page_t* page = get_page((void*)a, 0, kerndir);
// 		if( !page ) continue;
// 		page->present = 0;
// 		page->frame = 0;
	}
	
	init_kheap(0xD0000000, 0xD0010000, 0xF0000000);
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
	if( dir != kerndir ){
		addr = addr;
	}
	
	page_t* page = get_page(addr, 1, dir);

	if( page->present != 0 ){
		page->rw = (rw > 0);
		page->user = (user > 0);
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

/* Retrieves the physical memory address referred to by the virtual
 * address provided. The physical address is stored in *phys, and
 * EOKAY is returned on success or -EFAULT if the virtual address
 * is not mapped.
 */
int get_physical_addr(page_dir_t* dir, void* virt, u32* phys)
{
	// Get the page entry
	page_t* page = get_page(virt, 0, dir);
	if( page == NULL ){
		*phys = 0;
		return -EFAULT;
	}
	
	// Check if it is mapped
	if( page->present == 0 ){
		*phys = 0;
		return -EFAULT;
	}
	
	// Calculate the address with the offset given in the virtual
	// address.
	*phys = (page->frame << 12) | ((u32)virt & 0xFFF);
	return 0;
}

void map_page(page_dir_t* dir, void* virt, u32 phys, int user, int rw)
{
	page_t* page = get_page(virt, 1, dir);
	
	page->present = 1;
	page->rw = rw > 0 ? 1 : 0;
	page->user = user > 0 ? 1 : 0;
	page->frame = (phys >> 12);
	
	invalidate_page((u32*)virt);
}

void unmap_page(page_dir_t* dir, void* virt)
{
	page_t* page = get_page(virt, 0, dir);
	if( page == NULL ) return; // no reason do allocate the table...
	
	page->present = 0;
	page->frame = 0;
	
	invalidate_page((u32*)virt);
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

void temporary_unmap(void)
{
	page_t* dst_page = &curdir->table[(0xFFFFF000>>22)&0x3ff]->page[(0xFFFFF000>>12)&0x3ff];
	// set the page to point to the same frame
	dst_page->present = 0;
	dst_page->rw = 0;
	dst_page->user = 0;
	dst_page->frame = 0;
	// invalidate the TLB buffer for this page
	invalidate_page((u32*)0xFFFFF000);
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
	asm volatile( "movl %%cr3,%%eax; movl %%eax,%%cr3" : : : "eax" );
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
	
	syslog(KERN_PANIC, "%2Vpage_fault: process %d caused a page fault at address 0x%x", current->t_pid, address);
	syslog(KERN_PANIC, "%2Vpage_fault: page fault data: instr 0x%x, flags ( %s%s%s%s).", regs->eip, present ? "p " : "", rw ? "ro ":"rw ", us?"us ":"", reserved?"rsvd ":"");
	
	/*
	syslog(KERN_PANIC, "%2VPAGE FAULT ( %s%s%s%s)Address: 0x%x", present ? "present " : "", rw ? "read-only ":"", us?"user-mode ":"", reserved?"reserved ":"", address);
	syslog(KERN_PANIC, "%2VError Code: 0x%X", regs->err);
	syslog(KERN_PANIC, "%2VEFLAGS: 0x%X", regs->eflags);
	syslog(KERN_PANIC, "%2VEIP: 0x%X", regs->eip);
	u32 cr0, cr3;
	asm volatile ("movl %%cr0,%0;": "=r"(cr0));
	asm volatile ("movl %%cr3,%0;": "=r"(cr3));
	syslog(KERN_PANIC, "%2VCR0: 0x%X", cr0);
	syslog(KERN_PANIC, "%2VCR3: 0x%X", cr3);
	syslog(KERN_PANIC, "%2VCS: 0x%XDS: 0x%X", regs->cs, regs->ds);*/
	
	syslog(KERN_PANIC, "%2Vpage_fault: killing task %d.", current->t_pid);
	sys_exit(-1);
	
	syslog(KERN_PANIC, "%2Vsystem: unable to kill task. entering infinite loop instead...");
	while(1) asm volatile("hlt");
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
			syslog(KERN_ERR, "copy_page_table: unable to allocate page table!");
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
		temporary_unmap();
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
		syslog(KERN_ERR, "copy_page_dir: unable to allocate new directory!\n");
		return NULL;
	}
	if( ((u32)dst) & 0xFFF ){
		syslog(KERN_PANIC, "error: page directory not page aligned!\n");
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
		} else if( kerndir->table[t] ){
			dst->table[t] = src->table[t];
			dst->tablePhys[t] = src->tablePhys[t];
		} else {
			if( copy_page_table(dst, src, t) != 0 ){
				syslog(KERN_ERR, "copy_page_dir: unable to copy page table!\n");
				return NULL;
			}
		}
	}
	
	return dst;
}

int strip_page_dir(page_dir_t* dir)
{
	int erase_table = 0;
	for(unsigned int t = 0; t < 1024; ++t)
	{
		if( !dir->table[t] || dir->table[t] == kerndir->table[t] ) continue;
		erase_table=1;
		for(unsigned int p = 0; p < 1024; ++p)
		{
			// Don't erase kernel or initial user stacks
			if( VADDR(t,p,0) >= TASK_KSTACK_ADDR && VADDR(t,p,0) < (TASK_KSTACK_ADDR+TASK_KSTACK_SIZE) ){
				erase_table = 0;
				continue;
			}
			if( VADDR(t,p,0) >= (TASK_STACK_START-TASK_STACK_INIT_BASE) && VADDR(t,p,0) < TASK_STACK_START ){
				erase_table = 0;
				continue;
			}
			free_frame(&dir->table[t]->page[p]);
		}
		if( erase_table ){
			kfree(dir->table[t]);
		}
	}
	return 0;
}

void free_page_dir(page_dir_t* dir)
{
	if( dir == curdir ){
		return;
	}
	
	for(int t = 0; t < 1024; ++t)
	{
		if( !dir->table[t] || dir->table[t] == kerndir->table[t] ) continue;
		for(int p = 0; p < 1024; ++p)
		{
			free_frame(&dir->table[t]->page[p]);
		}
		kfree(dir->table[t]);
	}
	
	kfree(dir);
}

void display_page_dir(page_dir_t* dir)
{
	u32 addr = 0;
	//u32 prev = 0;
	u32 start_addr = 0;
	u32 start_frame = 0;
	u32 length = 0;
	
	syslog(KERN_NOTIFY, "Page Directory Mappings (dir=0x%X)\n", dir);
	while( addr < 0xFFFFFFFF )
	{
		page_t* page = get_page((void*)addr, 0, dir);
		if( page && page->present ){
			if( page->frame == (start_frame+length) && length != 0 ){
				length++;
			} else if( length == 0 ){
				start_addr = addr;
				start_frame = page->frame;
				length = 1;
			} else {
				syslog(KERN_NOTIFY, "0x%08X-0x%08X -> 0x%08X-0x%08X\n", start_frame*0x1000, (start_frame+length)*0x1000, start_addr, start_addr+length*0x1000);
				start_addr = addr;
				start_frame = page->frame;
				length = 1;
			}
		}
		if( addr == ((u32)(-0x1000)) ) break;
		addr += 0x1000;
	}
	
}