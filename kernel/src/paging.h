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
	u32 tablePhys[1024];
	page_table_t* table[1024];
	
	u32 phys;
} page_dir_t;

// Variables that may be needed by other source files
extern page_dir_t* kerndir;		// the kernel page directory mappings
extern page_dir_t* curdir;		// the current page directory (should match current->p_dir)

void init_paging( void );

/*! \brief Change virtual address spaces
 * 
 * Switches the current address space to new given address
 * space.
 * 
 * \param dir the page directory to switch to
 */
void switch_page_dir(page_dir_t* dir);

/*! \brief Find the page structure for a given address
 * 
 * Searches a directory to find the page entry for the given
 * virtual address. If create > 0, then the page table will
 * be allocated if needed.
 * 
 * \param address the virtual address to look up.
 * \param create either 0 or 1 indicating whether or not to create
 * 			the page table.
 * \param dir the directory to retrieve the entry from.
 * \return The pointer to the page entry or NULL if the table is
 * 		currently unallocated and create==0.
 * 
 */
page_t* get_page(void* address, int create, page_dir_t* dir);

/*! \brief Map a virtual address to a physical address
 * 
 * Maps a given virtual address to a physical address in a given
 * address space. You may also specify user and rw attributes.
 * 
 * \param dir the directory to map the address
 * \param virt the virtual address
 * \param phys the physical address to map
 * \param user either 1 or 0 indicating a user or kernel page
 * \param rw either 1 or zero indicating read/write or read only
 * 
 */
void map_page(page_dir_t* dir, void* virt, u32 phys, int user, int rw);

/*! \brief Unmaps a virtual address
 * 
 * Removes the mapping of a virtual address withing a given
 * address space.
 * 
 * \param dir the directory to unmap the address from
 * \param virt the virtual address to unmap
 * 
 */
void unmap_page(page_dir_t* dir, void* virt);

/*! \brief Copy a page directory and its contents
 * 
 * A function taking a page directory and copying the address space
 * into a newly created page directory.
 * 
 * \note This function does not switch directories. It simply creates
 * 		the new directory.
 * \param dir the directory to copy
 * \return A newly allocated page directory structure, identical in
 *		contents to the input directory.
 * \see page_dir_t
 */
page_dir_t* copy_page_dir(page_dir_t* dir);

/*! \brief Copy a page table and its contents
 * 
 * A function taking a dst and src directory and copying the given
 * table. If the table doesn't exist within the dst dir, it is created.
 * 
 * \param dest the destination directory
 * \param srce the source directory
 * \param tbl  the table index for both directories
 * \see paging.h copy_page_dir
 */
int copy_page_table(page_dir_t* dest, page_dir_t* srce, int tbl);


/*! \brief Translate a virtual to physical address.
 * 
 * Takes a virtual address and looks up the physical address
 * to which it is mapped.
 * 
 * \param dir The page directory in which to look up the address.
 * \param virt The virtual address to look up.
 * \return The physical address to which the virtual address is mapped
 * \note There is no good way to return for an unmapped page, so
 * 		you should check if the page is actually mapped before
 * 		using the function.
 */
u32 get_physical_addr(page_dir_t* dir, void* virt);

/*! \brief Copy the data from a physical frame of memory
 * 
 * Copies all data from one frame into the other frame.
 * This function disables paging for a moment in order
 * to copy the frame using physical addressess (accross
 * directories).
 * 
 * \param dest The destination frame address
 * \param srce The source frame address
 * \return none.
 * \note This is implemented in paging_asm.s
 */
void copy_physical_frame(u32 dest, u32 srce);

/*! \brief Temporarily map a page from one directory into another
 * 
 * This function maps a given page from one directory into the current
 * at address 0xFFFFF000. This is a temporary mapping, and should be
 * used sparingly as other kernel functions may want this area in memory.
 * 
 * \param addr the virtual address in the other directory to map
 * \param other the other page directory
 * \return the address to the temporary page (0xFFFFF000)
 */
void* temporary_map(u32 addr, page_dir_t* other);

/*! \brief Flush a Translation Lookaside Buffer Entry
 * 
 * This function flushes an entry from the TLB so that
 * a PDE/PTE lookup from main memory is required on the
 * next access to this memory location.
 * 
 * \param ptr the logical address of the invalidated page
 */
void invalidate_page(u32* ptr);

void page_fault(struct regs* regs);

#endif