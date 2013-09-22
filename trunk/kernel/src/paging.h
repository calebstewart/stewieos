#ifndef _PAGING_H_
#define _PAGING_H_

#include "kernel.h"
#include "descriptor_tables.h"

typedef struct page
{
	u32 present		: 1;	// Is this page entry present?
	u32 rw			: 1;	// Writable?
	u32 user		: 1;	// Are users aloud to read/write?
	u32 accessed		: 1;	// Has the page been accessed since last refresh
	u32 dirty		: 1;	// Has the page been written to since last refresh
	u32 unused		: 7;	// Unused bits
	u32 frame		: 20;	// The Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table
{
	page_t page[1024];
} page_table_t;

typedef struct page_dir
{
	page_table_t* table[1024];
	
	u32 tablePhys[1024];
	
	u32 phys;
} page_dir_t;

void init_paging( void );

void switch_page_dir(page_dir_t* dir);

page_t* get_page(void* address, int create, page_dir_t* dir);

void page_fault(struct regs* regs);

#endif