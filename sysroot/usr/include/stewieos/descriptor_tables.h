#ifndef _DESCRIPTOR_TABLES_H_
#define _DESCRIPTOR_TABLES_H_

#include "stewieos/kernel.h"

#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

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

// A struct describing a Task State Segment.
struct tss_entry_struct
{
	u32 prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list.
	u32 esp0;       // The stack pointer to load when we change to kernel mode.
	u32 ss0;        // The stack segment to load when we change to kernel mode.
	u32 esp1;       // everything below here is unusued now.. 
	u32 ss1;
	u32 esp2;
	u32 ss2;
	u32 cr3;
	u32 eip;
	u32 eflags;
	u32 eax;
	u32 ecx;
	u32 edx;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;
	u32 es;         
	u32 cs;        
	u32 ss;        
	u32 ds;        
	u32 fs;       
	u32 gs;         
	u32 ldt;      
	u16 trap;
	u16 iomap_base;
} __packed;

typedef struct tss_entry_struct tss_entry_t;

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

// Defines the prototype for interrupt routine callbacks
typedef void (*isr_callback_t)(struct regs* regs, void* context);


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

void irq0( void );
void irq1( void );
void irq2( void );
void irq3( void );
void irq4( void );
void irq5( void );
void irq6( void );
void irq7( void );
void irq8( void );
void irq9( void );
void irq10( void );
void irq11( void );
void irq12( void );
void irq13( void );
void irq14( void );
void irq15( void );

void syscall_intr( void );

void isr_handler(struct regs regs);
void irq_handler(struct regs regs);

// register a function as the interrupt handler
void register_interrupt(u8 n, void(*callback)(struct regs*));
void register_interrupt_context(u8 n, void* context, isr_callback_t callback);
void unregister_interrupt(u8 n);


#endif
