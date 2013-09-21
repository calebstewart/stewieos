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

/* Structure: regs
 * Purpose:
 * 	Encapsulates the contents of the stack after execution
 * 	of isr_stub or irq_stub (registers before the interrupt)
 */
struct regs
{
	u32 ds; // our user segment selector
	u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // from pusha
	u32 intno, err; // Interrupt number and error code
	u32 eip, cs,eflags, useresp, ss; // Pushed by the processor
};


int initialize_descriptor_tables( void );

void isr0( void );
void isr1( void );
void isr2( void );
void isr3( void );
void isr4( void );
void isr5( void );
void isr6( void );
void isr7( void );
void isr8( void );
void isr9( void );
void isr10( void );
void isr11( void );
void isr12( void );
void isr13( void );
void isr14( void );
void isr15( void );
void isr16( void );
void isr17( void );
void isr18( void );
void isr19( void );
void isr20( void );
void isr21( void );
void isr22( void );
void isr23( void );
void isr24( void );
void isr25( void );
void isr26( void );
void isr27( void );
void isr28( void );
void isr29( void );
void isr30( void );
void isr31( void );
void isr32( void );

void isr_handler(struct regs regs);


#endif
