ENTRY(start)
OUTPUT_FORMAT(elf32-i386)
STARTUP(./src/start.s.o);

/*kern_base = 0xC0000000;*/
SECTIONS
{

	. = 0x100000;
	
	.init.text . :
	{
		*(.init.text)
	}
	.init.data ALIGN(0x1000) :
	{
		*(.init.data)
	}

	. += KERNEL_VIRTUAL_BASE;

	.text ALIGN(0x1000) : AT(ADDR(.text) - 0xC0000000)
	{
		code = .; _code = .; __code = .;
		*(.text)
		_end_text = .;
	}
	
	.data ALIGN(0x1000) : AT(ADDR(.data) - 0xC0000000)
	{
		data = .; _data = .; __data = .;
		*(.data);
		*(.rodata);
		_end_data = .;
	}
	
	.bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xC0000000)
	{
		bss = .; _bss = .; __bss = .;
		*(.bss)
	}
	
	end = .; _end = .; __end = .;
	_phys_end = . - 0xC0000000;	/* This is the physical ending location of the kernel (for hte PMM) */
}
