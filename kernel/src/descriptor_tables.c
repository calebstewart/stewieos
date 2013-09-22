//#include "kernel.h"
#include "descriptor_tables.h"

// Function Prototypes
extern void flush_gdt(void* addr); // assembly function to present the new gdt to the system
extern void flush_idt(void* addr); // assembly function to present the new idt to the system
static void gdt_set_gate(int n, u32 base, u32 limit, u8 access, u8 granularity); // set a gdt entry
static void idt_set_gate(uint n, void(*base)(void), u16 selector, u8 flags); // set an idt entry
static int initialize_gdt(void);
static int initialize_idt(void);

// Global Variables
struct gdt_entry	gdt_table[5];			// Global Descriptor Table
struct gdt_ptr		gdt_ptr;			// Global Descriptor Table 
struct idt_entry	idt_table[256];			// Interrupt Descriptor Table
struct idt_ptr		idt_ptr;			// Interrupt Descriptor Table Pointer
isr_callback_t		isr_callback[256];		// Interrupt handlers for IRQs and ISRs


/* function: initialize_descriptor_tables
 * parameters: none
 * purpose:
 * 	Load the interrupt and global descriptor tables
 * 	and initialize their values.
 */
int initialize_descriptor_tables()
{
	printk("installing global descriptor table... ");
	initialize_gdt();
	printk("done.\n");
	printk("installing interrupt descriptor table... ");
	initialize_idt();
	printk("done.\n");
	asm volatile ("int $0xF");
	return 0;
}

static int initialize_gdt( void )
{
	gdt_ptr.limit = sizeof(struct gdt_entry)*5 - 1;
	gdt_ptr.base = (u32)&gdt_table;
	
	gdt_set_gate(0x00, 0, 0, 0, 0);			// Null Segment
	gdt_set_gate(0x01, 0, 0xFFFFFFFF, 0x9A, 0xCF);	// Kernel Code Segment
	gdt_set_gate(0x02, 0, 0xFFFFFFFF, 0x92, 0xCF);	// Kernel Data Segment
	gdt_set_gate(0x03, 0, 0xFFFFFFFF, 0xFA, 0xCF);	// User Code Segment
	gdt_set_gate(0x04, 0, 0xFFFFFFFF, 0xF2, 0xCF);	// User Data Segment
	
	flush_gdt(&gdt_ptr);
	
	return 0;
}

static void gdt_set_gate(int n, u32 base, u32 limit, u8 access, u8 gran)
{
	if( n >= 5 || n < 0 ) return;

	gdt_table[n].base_low = (base & 0xFFFF);
	gdt_table[n].base_middle = (u8)((base & 0xFF0000) >> 16);
	gdt_table[n].base_high = (u8)((base & 0xFF000000) >> 24);
	gdt_table[n].granularity = (u8)((limit & 0xF0000) >> 16);
	gdt_table[n].granularity = (u8)(gdt_table[n].granularity | (gran & 0xF0));
	gdt_table[n].access = access;
}

static int initialize_idt( void )
{
	idt_ptr.limit = sizeof(struct idt_entry)*256 - 1;
	idt_ptr.base = (u32)&idt_table;
	
	idt_set_gate(0, isr0, 0x08, 0x8E);
	idt_set_gate(1, isr1, 0x08, 0x8E);
	idt_set_gate(2, isr2, 0x08, 0x8E);
	idt_set_gate(3, isr3, 0x08, 0x8E);
	idt_set_gate(4, isr4, 0x08, 0x8E);
	idt_set_gate(5, isr5, 0x08, 0x8E);
	idt_set_gate(6, isr6, 0x08, 0x8E);
	idt_set_gate(7, isr7, 0x08, 0x8E);
	idt_set_gate(8, isr8, 0x08, 0x8E);
	idt_set_gate(9, isr9, 0x08, 0x8E);
	idt_set_gate(10, isr10, 0x08, 0x8E);
	idt_set_gate(11, isr11, 0x08, 0x8E);
	idt_set_gate(12, isr12, 0x08, 0x8E);
	idt_set_gate(13, isr13, 0x08, 0x8E);
	idt_set_gate(14, isr14, 0x08, 0x8E);
	idt_set_gate(15, isr15, 0x08, 0x8E);
	idt_set_gate(16, isr16, 0x08, 0x8E);
	idt_set_gate(17, isr17, 0x08, 0x8E);
	idt_set_gate(18, isr18, 0x08, 0x8E);
	idt_set_gate(19, isr19, 0x08, 0x8E);
	idt_set_gate(20, isr20, 0x08, 0x8E);
	idt_set_gate(21, isr21, 0x08, 0x8E);
	idt_set_gate(22, isr22, 0x08, 0x8E);
	idt_set_gate(23, isr23, 0x08, 0x8E);
	idt_set_gate(24, isr24, 0x08, 0x8E);
	idt_set_gate(25, isr25, 0x08, 0x8E);
	idt_set_gate(26, isr26, 0x08, 0x8E);
	idt_set_gate(27, isr27, 0x08, 0x8E);
	idt_set_gate(28, isr28, 0x08, 0x8E);
	idt_set_gate(29, isr29, 0x08, 0x8E);
	idt_set_gate(30, isr30, 0x08, 0x8E);
	idt_set_gate(31, isr31, 0x08, 0x8E);
	
	// Remap all the IRQs to known interrupt
	// vectors (32-47)
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 0x28);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);
	
	idt_set_gate(32, irq0, 0x08, 0x80);
	idt_set_gate(33, irq1, 0x08, 0x80);
	idt_set_gate(34, irq2, 0x08, 0x80);
	idt_set_gate(35, irq3, 0x08, 0x80);
	idt_set_gate(36, irq4, 0x08, 0x80);
	idt_set_gate(37, irq5, 0x08, 0x80);
	idt_set_gate(38, irq6, 0x08, 0x80);
	idt_set_gate(39, irq7, 0x08, 0x80);
	idt_set_gate(40, irq8, 0x08, 0x80);
	idt_set_gate(41, irq9, 0x08, 0x80);
	idt_set_gate(42, irq10, 0x08, 0x80);
	idt_set_gate(43, irq11, 0x08, 0x80);
	idt_set_gate(44, irq12, 0x08, 0x80);
	idt_set_gate(45, irq13, 0x08, 0x80);
	idt_set_gate(46, irq14, 0x08, 0x80);
	idt_set_gate(47, irq15, 0x08, 0x80);

	flush_idt((void*)&idt_ptr);

	return 0;
}

static void idt_set_gate(uint n, void(*base)(void), u16 sel, u8 flags)
{
	if( n >= 256 ) return;
	idt_table[n].base_low = (((u32)base) & 0xFFFF);
	idt_table[n].base_high = (u16)((((u32)base) >> 16) & 0xFFFF);
	idt_table[n].flags = flags;
	idt_table[n].selector = sel;
	idt_table[n].always0 = 0;
}

void isr_handler(struct regs regs)
{
	printk("%2VCAUGHT INTERRUPT SERVICE ROUTINE!\n");
	
}

void irq_handler(struct regs regs)
{
	// Is this coming from the second PIC?
	if( regs.intno >= 40 )
	{
		// Send the reset signal to the second PIC
		outb(0xA, 0x20);
	}
	// Send the reset signal to the first PIC
	outb(0x20, 0x20);
	
	if( isr_callback[regs.intno] != 0 ){
		isr_callback[regs.intno](&regs);
	}
	
	printk("%V1CUAGHT INTERRUPT REQUEST %#X!\n", regs.intno-32);
}

void register_interrupt(u8 n, isr_callback_t callback)
{
	isr_callback[n] = callback;
}