//#include "stewieos/kernel.h"
#include "stewieos/descriptor_tables.h"
#include "stewieos/task.h"
#include "syscall.h"

// Function Prototypes
extern void flush_gdt(void* addr); // assembly function to present the new gdt to the system
extern void flush_idt(void* addr); // assembly function to present the new idt to the system
static void gdt_set_gate(int n, u32 base, u32 limit, u8 access, u8 granularity); // set a gdt entry
static void idt_set_gate(uint n, void(*base)(void), u16 selector, u8 flags); // set an idt entry
static int initialize_gdt(void);
static int initialize_idt(void);

#define GDT_SIZE 6

// Global Variables
struct gdt_entry	gdt_table[GDT_SIZE];			// Global Descriptor Table
struct gdt_ptr		gdt_ptr;			// Global Descriptor Table 
struct idt_entry	idt_table[256];			// Interrupt Descriptor Table
struct idt_ptr		idt_ptr;			// Interrupt Descriptor Table Pointer
isr_callback_t		isr_callback[256];		// Interrupt handlers for IRQs and ISRs
void*				isr_context[256];		// context pointers for the callbacks
tss_entry_t		tss_entry = {			// Task State Segment Entry
				.esp0 = TASK_KSTACK_ADDR+TASK_KSTACK_SIZE,
				.ss0 = 0x10,
				.iomap_base = sizeof(tss_entry_t),
};


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
	
	u32 eflags = disablei();
	eflags = eflags & ~(1 << 12);
	eflags &= ~(1 << 13);
	restore(eflags);
	return 0;
}

static int initialize_gdt( void )
{
	gdt_ptr.limit = sizeof(struct gdt_entry)*GDT_SIZE - 1;
	gdt_ptr.base = (u32)&gdt_table;
	
	gdt_set_gate(0x00, 0, 0, 0, 0);			// Null Segment
	gdt_set_gate(0x01, 0, 0xFFFFFFFF, 0x9A, 0xCF);	// Kernel Code Segment
	gdt_set_gate(0x02, 0, 0xFFFFFFFF, 0x92, 0xCF);	// Kernel Data Segment
	gdt_set_gate(0x03, 0, 0xFFFFFFFF, 0xFA, 0xCF);	// User Code Segment
	gdt_set_gate(0x04, 0, 0xFFFFFFFF, 0xF2, 0xCF);	// User Data Segment
	gdt_set_gate(0x05, (u32)&tss_entry, sizeof(tss_entry), 0x89, 0x40);
	
	flush_gdt(&gdt_ptr);
	
	return 0;
}

static void gdt_set_gate(int n, u32 base, u32 limit, u8 access, u8 gran)
{
	if( n >= GDT_SIZE || n < 0 ) return;

	gdt_table[n].base_low = (base & 0xFFFF);
	gdt_table[n].base_middle = (u8)((base & 0xFF0000) >> 16);
	gdt_table[n].base_high = (u8)((base & 0xFF000000) >> 24);
	gdt_table[n].limit_low = limit & 0xFFFF;
	gdt_table[n].granularity = (u8)((limit >> 16) & 0x0F);
	gdt_table[n].granularity = (u8)(gdt_table[n].granularity | (gran & 0xF0));
	gdt_table[n].access = access;
}

void debug_interrupt(struct regs* regs);
void debug_interrupt(struct regs* regs)
{
	regs->eflags &= ~0x100;
	return;
	u32 dr6 = 0;
	asm volatile ("movl %%dr6,%0" : "=r"(dr6));
	printk("%1VDebugging Exception!\n");
	printk("%1VInstruction Pointer: 0x%X\n", regs->eip);
	printk("%1VEFLAGS: 0x%X\n", regs->eflags);
	printk("%1VDR6: 0x%X\n", dr6);
	asm volatile ("hlt");
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
	
	idt_set_gate(0x80, syscall_intr, 0x08, 0xEE);
	
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
	
	idt_set_gate(32, irq0, 0x08, 0x8E);
	idt_set_gate(33, irq1, 0x08, 0x8E);
	idt_set_gate(34, irq2, 0x08, 0x8E);
	idt_set_gate(35, irq3, 0x08, 0x8E);
	idt_set_gate(36, irq4, 0x08, 0x8E);
	idt_set_gate(37, irq5, 0x08, 0x8E);
	idt_set_gate(38, irq6, 0x08, 0x8E);
	idt_set_gate(39, irq7, 0x08, 0x8E);
	idt_set_gate(40, irq8, 0x08, 0x8E);
	idt_set_gate(41, irq9, 0x08, 0x8E);
	idt_set_gate(42, irq10, 0x08, 0x8E);
	idt_set_gate(43, irq11, 0x08, 0x8E);
	idt_set_gate(44, irq12, 0x08, 0x8E);
	idt_set_gate(45, irq13, 0x08, 0x8E);
	idt_set_gate(46, irq14, 0x08, 0x8E);
	idt_set_gate(47, irq15, 0x08, 0x8E);
	
	register_interrupt(0x01, debug_interrupt);
	
	flush_idt((void*)&idt_ptr);
	
	register_interrupt(0x80, syscall_handler);
	
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

const char* g_fault_names[] = {
	"Divide-by-Zero Error",
	"Debug",
	"Non-Maskable Interrupt",
	"Breakpoint",
	"Overflow",
	"Bound Range Exceeded",
	"Invalid Opcode",
	"Device Not Available",
	"Double Fault",
	"Coprocessor Segment Overrun",
	"Invalid TSS",
	"Segment Not Present",
	"Stack-Segment Fault",
	"General Protection Fault",
	"Page Fault",
	"Reserved",
	"x87 Floating-Point Exception",
	"Alignment Check",
	"Machine Check",
	"SIMD Floating-Point Exception",
	"Virtualization Exception",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Security Exception",
	"Reserved",
};

void isr_handler(struct regs regs)
{
	if( isr_callback[regs.intno] ){
		isr_callback[regs.intno](&regs, isr_context[regs.intno]);
		return;
	}
	if( regs.intno < 32 ){
		printk("%2V\nUnhandled Exception: %s (0x%X)\n", g_fault_names[regs.intno], regs.intno);
	} else {
		printk("%2V\nUnhandled Exception: 0x%X\n", regs.intno);
	}
	printk("%2VError Code: 0x%X\n", regs.err);
	printk("%2VEFLAGS: 0x%X\n", regs.eflags);
	printk("%2VEIP: 0x%X\n", regs.eip);
	u32 cr0, cr3;
	asm volatile ("movl %%cr0,%0;": "=r"(cr0));
	asm volatile ("movl %%cr3,%0;": "=r"(cr3));
	printk("%2VCR0: 0x%X\n", cr0);
	printk("%2VCR3: 0x%X\n", cr3);
	printk("%2VCS: 0x%X\nDS: 0x%X\n", regs.cs, regs.ds);
	
	printk("%2VGDT Table:\n");
	for(int i = 0; i < 5; ++i){
		printk("%2V%02X%02X%04X %04X %02X %02X\n", gdt_table[i].base_high, gdt_table[i].base_middle, gdt_table[i].base_low, gdt_table[i].limit_low, gdt_table[i].access, gdt_table[i].granularity);
	}
	//printk("%2VCAUGHT INTERRUPT SERVICE ROUTINE!\n");
	
	while(1);
}

void irq_handler(struct regs regs)
{
	// Is this coming from the second PIC?
	if( regs.intno >= 40 )
	{
		// Send the reset signal to the second PIC
		outb(0xA0, 0x20);
	}
	// Send the reset signal to the first PIC
	outb(0x20, 0x20);

	if( isr_callback[regs.intno] != 0 ){
		isr_callback[regs.intno](&regs, isr_context[regs.intno]);
	}

}

void register_interrupt(u8 n, void(*callback)(struct regs*))
{
	register_interrupt_context(n, NULL, (isr_callback_t)callback);
}
void register_interrupt_context(u8 n, void* context, isr_callback_t callback)
{
	isr_callback[n] = callback;
	isr_context[n] = context;
}
void unregister_interrupt(u8 n)
{
	isr_callback[n] = NULL;
}