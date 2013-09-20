#ifndef _DESCRIPTOR_TABLES_H_
#define _DESCRIPTOR_TABLES_H_

#include "kernel.h"

/* structure: gdt_entry
 * purpose:
 * 	encapsulates an entry in the gdt table
 * properties:
 * 	packed (gcc: don't align or add anything to this structure)
 */
struct gdt_entry
{
	u16	limit_low;		// low 16 bits of the limit
	u16	base_low;		// low 16 bits of the base
	u8	base_middle;		// middle 8 bits of the base
	u8	access;			// access flags
	u8	granularity;		// granularity flags
	u8	base_high;		// high 8 bits of the base
} __attribute__((packed));

/* structure: gdt_ptr
 * purpose:
 * 	tells the system where to find the gdt table,
 * 	and how big it is.
 * properties:
 * 	packed (gcc: don't align or add antyhing to this structure)
 */
struct gdt_ptr
{
	u16	limit;			// the size of the table-1
	u32	base;			// the address of the entry table
} __attribute__((packed));

/* structure: idt_entry
 * purpose:
 * 	encapsulates an entry in the idt table
 */
struct idt_entry
{
	u16	base_low;		// The lower 16 bits of the base
	u16	selector;		// the kernel segment selector
	u8	always0;		// Always zero
	u8	flags;			// Flags
	u16	base_high;		// The upper 16 bits of the base
} __attribute__((packed));

struct idt_ptr
{
	u16	limit;			// The length of the idt table -1
	u32	base;			// The pointer to the table
} __attribute__((packed));


int initialize_descriptor_tables( void );

#endif
