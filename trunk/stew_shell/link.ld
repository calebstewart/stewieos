OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
	. = 0x00000000;
	
	.module_info : {
		*(.module_info);
	}

	.text : ALIGN(0x1000) {
	_TEXT_START = .;
		*(.text);
	_TEXT_END = .;
	}

	.data : ALIGN(0x1000) {
	_DATA_START = .;
		*(.data);
	_DATA_END = .;
	}
	
	.bss : ALIGN(0x1000) {
	_BSS_START = .;
		*(.data);
	_BSS_END = .;
	}

}
