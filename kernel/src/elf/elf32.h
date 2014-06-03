#ifndef _ELF32_H_
#define _ELF32_H_

#include "kernel.h"
#include "exec.h"
#include <sys/types.h>

// Macro definitions

// Get the symbol ID from the relocation info
#define ELF32_R_SYM(i)		((i)>>8)
// Get the relocation type from the relocation info
#define ELF32_R_TYPE(i)		((unsigned char)(i))
// Create a relocation info value from a symbol and type
#define ELF32_R_INFO(s, t)	(((s)<<8) + ((unsigned char)(t)))

// Get the binding type for the symbol
#define ELF32_ST_BIND(i)	((i)>>4)
// Get the type for the symbol
#define ELF32_ST_TYPE(i)	((i)&0xf)
// BUild a symbol info object
#define ELF32_ST_INFO(b,t)	(((b)<<4)+((t)&0xf))

// Basic Type definitions
typedef u32 Elf32_Addr;		// Unsigned program addres
typedef u16 Elf32_Half;		// Unsigned medium integer
typedef u32 Elf32_Off;		// Unsigned file offset
typedef s32 Elf32_Sword;	// Signed large integer
typedef u32 Elf32_Word;		// Unsigned large integer

// Basic Constant Definitions 

// e_ident indexes
enum EI_INDEXES
{
	EI_MAG0	,	// File Identification
	EI_MAG1,	// ^^
	EI_MAG2,	// ^^
	EI_MAG3,	// ^^
	EI_CLASS,	// File Class
	EI_DATA,	// Data Encoding
	EI_VERSION,	// File version
	EI_PAD,		// Start of padding bytes
	EI_NIDENT = 16	// Size of e_ident[]
};

// File types
enum ET_TYPES
{
	ET_NONE,		// No file type
	ET_REL,			// Relocatable file
	ET_EXEC,		// Executable File
	ET_DYN,			// Shared Object File
	ET_CORE,		// Core file
	ET_LOPROC = 0xFF00,	// Processor Specific
	ET_HIPROC = 0xFFFF	// Processor Specific
};

// Architecture for a file
enum EM_MACHINES
{
	EM_NONE = 0,	// No machine
	EM_M32 = 1,	// AT&T WE 32100
	EM_SPARC = 2,	// SPARC
	EM_386 = 3,	// Intel 80386
	EM_68K = 4,	// Motorola 68000
	EM_88K = 5,	// Motorola 88000
	EM_860 = 7,	// Intel 80860
	EM_MIPS = 8,	// MIPS RS3000
};

// Object File Version
enum EV_VERSIONS
{
	EV_NONE,	// Invalid Version
	EV_CURRENT	// Current Version
};

// ELF Magic Numbers (e_ident values)
enum EI_MAGICS
{
	ELFMAG0 = 0x7f,
	ELFMAG1 = 'E',
	ELFMAG2 = 'L',
	ELFMAG3 = 'F',
};
// ELF Object Classes
enum EI_CLASSES
{
	ELFCLASSNONE,
	ELFCLASS32,
	ELFCLASS64
};
// ELF Object Data Encoding
enum EI_DATAENCODE
{
	ELFDATANONE,
	ELFDATALSB,
	ELFDATAMSB
};

// Check the ELF file identification bytes
// This assumes 32bit architecture and LSB
// Data encoding (32-bit Intel, likely)
#define EI_CHECK(elf) ( (elf)->e_ident[EI_MAG0] == ELFMAG0 && \
				(elf)->e_ident[EI_MAG1] == ELFMAG1 && \
				(elf)->e_ident[EI_MAG2] == ELFMAG2 && \
				(elf)->e_ident[EI_MAG3) == ELFMAG3 && \
				(elf)->e_ident[EI_CLASS] == ELFCLASS32 && \
				(elf)->e_ident[EI_DATA] == ELFDATALSB )
				
// Special Section Indexes
enum SHN_INDEXES
{
	SHN_UNDEF = 0,			// Undefined Section
	SHN_LORESERVE = 0xFF00,		// Lower bound of reserved indexes
	SHN_LOPROC = 0xFF00,		// Lower bound of processor specific reserves
	SHN_HIPROC = 0xFF1F,		// Upper bound of processor specific reserves
	SHN_ABS = 0xFFF1,		// Absolute values for the corresponding reference (not effected by relocation)
	SHN_COMMON = 0xFFF2,		// Common Symbols (unallocated C external variables)
	SHN_HIRESERVE = 0xFFFF		// Upper bound of the range of reserved indexes
};

// Section Types
enum ElfSectionType
{
	SHT_NULL = 0,			// Inactive section header
	SHT_PROGBITS,			// Program defined information
	SHT_SYMTAB,			// Symbol table information
	SHT_STRTAB,			// String table
	SHT_RELA,			// Relocation Entries with explicit addends (e.g. Elf32_Rela)
	SHT_HASH,			// Symbol hash table (used for Dynamic linking)
	SHT_DYNAMIC,			// Dynamic linking information
	SHT_NOTE,			// File marking information
	SHT_NOBITS,			// Contains no data, but resembles SHT_PROGBITS
	SHT_REL,			// Relocation entries without explicit addends (e.g. Elf32_Rel)
	SHT_SHLIB,			// Reserved, but undefined (non-ABI conformant!)
	SHT_DYNSYM,			// Symbol table information strictly for dynamic linking
	SHT_LOPROC = 0x70000000,	// Low bound (inclusive) for processor specific semantics
	SHT_HIPROC = 0x7fffffff,	// Hi bound (inclusive) for processor specific semantics
	SHT_LOUSER = 0x80000000,	// Low bound of application specific reserved indexes
	SHT_HIUSER = 0xFFFFFFFF		// Upper bound of application specific reserved indexes
};

// Section Attribute Flags
enum ElfSectionAttributeFlags
{
	SHF_WRITE = 0x1,		// Data should be writable during process execution
	SHF_ALLOC = 0x2,		// Occupies memory during process execution
	SHF_EXECINSTR = 0x4,		// Contains Executable Machine Instructions
	SHF_MASKPROC = 0xF0000000,	// Reserved bits for processor specific semantics
};

// Symbol Binding Attributes
enum ElfSymbolBinding
{
	STB_LOCAL = 0,			// Only visible within the current object file
	STB_GLOBAL,			// Visible to all object files being combined.
	STB_WEAK,			// Resembles global symbol, but has lower precedence
	STB_LOPROC = 13,		// Lower bound for inclusive range of processor specific semantics
	STB_HIPROC = 15,		// Upper bound for inclusive range of processor specific semantics
};

// Symbol Type Attributes
enum ElfSymbolType
{
	STT_NOTYPE,			// Symbols type is not specified
	STT_OBJECT,			// Associated with a data object, such as a variable, an array, etc.
	STT_FUNC,			// Function or other executable code
	STT_SECTION,			// Represents a section, used primarily for relocation
	STT_FILE,			// Conventionally, gives the name of the source file associated with the object file.
	STT_LOPROC = 13,		// Lower bound for inclusive range of processor specific semantics
	STT_HIPROC = 15,		// Upper bound for inclusive range of processor specific semantics
};

// Special Symbol table indexes
enum ElfSymbolTableIndexes
{
	STN_UNDEF = 0,
};

/* Relocation Types
 * Calculation Meanings:
 * 	A   - Addend used to compute the value of the relocatable field
 * 	B   - Base address at which the object is loaded
 * 	G   - Offset into the global offset table where the entry will reside during execution
 * 	GOT - Address of the global offset table
 * 	L   - Section offset or address of the procedure linkage table for a symbol.
 * 	P   - Place (section offset or address) of the storage unit being relocated (computed using r_offset)
 * 	S   - Value of the symbol whose index resides in the relocation entry
 */
enum ElfRelocationTypes
{
//	Relocation Name			   Relocation Calculation
	R_386_NONE = 0,			// No relocation
	R_386_32,			// S + A
	R_386_PC32,			// S + A - P
	R_386_GOT32,			// G + A - P
	R_386_PLT32,			// L + A - P
	R_386_COPY,			// No relocation
	R_386_GLOB_DAT,			// S
	R_386_JMP_SLOT,			// S
	R_386_RELATIVE,			// B + A
	R_386_GOTOFF,			// S + A - GOT
	R_386_GOTPC,			// GOT + A - P
};

// Program Header Types
enum ProgramHeaderTypes
{
	PT_NULL = 0,			// Array element unused (other members undefined)
	PT_LOAD,			// Loadable segment
	PT_DYNAMIC,			// Dynamic Linking information
	PT_INTERP,			// Null-terminated path name to invoke as an interpreter.
	PT_NOTE,			// Auxiliary Information
	PT_SHLIB,			// Reserved with no specified semantics (Non-conformant to ABI)
	PT_PHDR,			// Program header table
	PT_LOPROC = 0x70000000,		// Lower bound for processor-specific semantics
	PT_HIPROC = 0x7fffffff,		// Upper bound for processor-specific semantics
};

// Data Structures

/* structure: Elf32_Ehdr
 * purpose: The ELF header identifies the file as an ELF exectuble
 * 		and houses the needed data to load and link the 
 * 		executable.
 */
typedef struct {
	unsigned char	e_ident[EI_NIDENT];	// Initial bytes mark the file as an object file
	Elf32_Half	e_type;			// Object file type (ET_*)
	Elf32_Half	e_machine;		// Architecture required for this file (EM_*)
	Elf32_Word	e_version;		// Object file version (EV_*)
	Elf32_Addr	e_entry;		// The virtual address for application entry pointer
	Elf32_Off	e_phoff;		// Program header tables file offset
	Elf32_Off	e_shoff;		// Section Header table's file offset
	Elf32_Word	e_flags;		// Processor specific flags (EF_*)
	Elf32_Half	e_ehsize;		// Size of the ELF Header
	Elf32_Half	e_phentsize;		// Size of one program header
	Elf32_Half	e_phnum;		// Number of entries in the program header
	Elf32_Half	e_shentsize;		// Size of one section header
	Elf32_Half	e_shnum;		// Number of section header entries
	Elf32_Half	e_shstrndx;		// Section header table index for the section name string table
} Elf32_Ehdr;

/* structure: Elf32_Shdr
 * purpose: describes a object file section. All data in an object
 * 		is held in sections.
 */
typedef struct {
	Elf32_Word	sh_name;		// Index into section header string table
	Elf32_Word	sh_type;		// Section type (See SHT_*)
	Elf32_Word	sh_flags;		// Bitfield flag array (See SHF_*)
	Elf32_Addr	sh_addr;		// If the section is to be loaded into memory, this is the load address
	Elf32_Off	sh_offset;		// Offset to the first byte of the section (in the file)
	Elf32_Word	sh_size;		// Size of the section in bytes
	Elf32_Word	sh_link;		// Section header table index link (depends on sh_type)
	Elf32_Word	sh_info;		// Extra info (depends on sh_type)
	Elf32_Word	sh_addralign;		// Address alignment (sh_addr % sh_addralign must equal zero)
	Elf32_Word	sh_entsize;		// Size of each table entry in the section (if any)
} Elf32_Shdr;

/* structure: Elf32_Phdr
 * purpose: Describes segment or other information the system needs
 * 		to prepare the program execution.
 */
typedef struct {
	Elf32_Word	p_type;			// What kind of segment this array element describes
	Elf32_Off	p_offset;		// Offset from the beginning of the file where the segment resides
	Elf32_Addr	p_vaddr;		// Virtual address at which the segment resides in memory
	Elf32_Addr	p_paddr;		// On systems where physical addressing is relevant, this member describes the physical memory address
	Elf32_Word	p_filesz;		// Number of bytes the segment occupies in the file
	Elf32_Word	p_memsz;		// Number of bytes the segment occupies in memory (may be zero)
	Elf32_Word	p_flags;		// Flags relevant to the segment
	Elf32_Word	p_align;		// Alignment for p_vaddr (no alignment if ==0 or ==1)
} Elf32_Phdr;

/* structure: Elf32_Sym
 * purpose: Symbol table entry describes application symbols
 * 		their names, and addresses within the memory image
 */
typedef struct {
	Elf32_Word	st_name;		// Index into string table giving the symbol's name
	Elf32_Addr	st_value;		// Symbol value (absolute value, address, etc.)
	Elf32_Word	st_size;		// Size of the symbol data/object
	unsigned char	st_info;		// Symbol type and binding attributes
	unsigned char	st_other;		// Holds zero with no defined meaning
	Elf32_Half	st_shndx;		// The section that this symbol is defined in relation to
} Elf32_Sym;

typedef struct {
	Elf32_Addr	r_offset;		// Location to apply the relocation action (byte offset from beginning of section)
	Elf32_Word	r_info;			// Contains the section index and symbol table index
} Elf32_Rel;

typedef struct {
	Elf32_Addr	r_offset;		// Location to apply the relocation action (byte offset from beginning of section)
	Elf32_Word	r_info;			// Contains the section index and symbol table index
	Elf32_Sword	r_addend;		// Constant addend used to compute the value to be stored into the relocatable field
} Elf32_Rela;

typedef struct {
	Elf32_Sword	d_tag;
	union {
			Elf32_Word d_val;
			Elf32_Addr d_ptr;
	} d_un;
} Elf32_Dyn;

/* Old Structure:
 * file -> 0
 * ehdr -> 4
 * phdr -> 56
 * shdr -> 60
 * rel  -> 64
 * relhdr -> 68
 * relsz -> 72
 * dyn -> 76
 * dynsz -> 80
 * symtab -> 84
 * symhdr -> 88
 * shstrtab -> 92
 * strtab -> 96
 * baseaddr -> 100
 * endaddr -> 104
 * loadaddr -> 108
 */
typedef struct {
	char path[256]; // The path of the executable
	struct file* file; // File structure (If currently open, otherwise NULL)
	Elf32_Ehdr ehdr; // Loaded header structure 
	Elf32_Shdr* shdr; // If non-null, section header table. Length=ehdr->e_shnum
	Elf32_Phdr* phdr; // If non-null, program header table. Length=ehdr->e_phnum
	char* strtab; // If non-null, Loaded string table
	char* shstrtab; // If non-null, section string table
	Elf32_Addr base; // The base address selected by the compiler (computed)
	Elf32_Addr top; // the top address of the executable
	Elf32_Sym* symtab; // the symbol table
	Elf32_Shdr* symhdr; // the symbol table section header
} Elf32_Exec;

/* Functions implemented in MOSS kernel
 * 	These are probably the ones I need, since it worked before...
 */
// int elf_read_section(Elf32_Exec* exec, Elf32_Shdr* shdr, void* buffer);
// unsigned char get_machine_encoding( void );
// Elf32_Exec* elf_read_exec(const char* path);
// Elf32_Sym* elf_find_symbol(Elf32_Exec* exec, const char* name);
// Elf32_Sym* elf_find_closest_symbol(Elf32_Exec* elf, void* addr);
// const char* elf_find_string(Elf32_Exec* exec, Elf32_Word idx);
// Elf32_Addr elf_resolve_symbol(Elf32_Exec* exec, Elf32_Sym* sym, int link_to_kernel);
// void elf_close_exec(Elf32_Exec* exec);
// void elf_relocate(Elf32_Exec* exec, Elf32_Shdr* hdr, Elf32_Rel* rel);
// int elf_load_module(const char* path, struct module* module);
// int elf_read_header(Elf32_Exec* exec);
// int elf_read_program_header(struct file* file, Elf32_Ehdr* ehdr, Elf32_Half phndx, Elf32_Phdr* phdr);
// int elf_read_section_header(struct file* file, Elf32_Ehdr* ehdr, Elf32_Half shndx, Elf32_Shdr* shdr);
// int elf_load_segment(Elf32_Exec* exec, Elf32_Phdr* phdr);
// int elf_load_executable(const char* path, Elf32_Exec** pExec);

int elf_check_ident(Elf32_Ehdr* elf, Elf32_Half type);

int elf_apply_rel(struct file* file, Elf32_Ehdr* header, Elf32_Rel* relocation);
int elf_apply_rela(struct file* file, Elf32_Ehdr* header, Elf32_Rela* relocation);

// Read the ELF Header
int elf_read_header(struct file* file, Elf32_Ehdr* header);
// Read the ELF Section Header List. The buffer must be (e_shentsize*e_shnum) bytes long.
int elf_read_section_header_list(struct file* file, Elf32_Ehdr* header, Elf32_Shdr* buffer);
// Read the contents of an ELF Section. The buffer must be sh_size bytes long
// If the section type SHT_NULL or SHT_NOBITS, then the buffer is unmodified.
int elf_load_section(struct file* file, Elf32_Ehdr* header, Elf32_Shdr* shn, char* buffer);

// Initialize a module that has already been read into memory
// and return its module information structure
module_t* elf_init_module(void* file_data, size_t length);

// Apply a relocation to a ET_REL type object
// the object file should be read in in entirety 
// at "ehdr" (so data following ehdr is the file)
int elf_apply_relocation(Elf32_Ehdr* ehdr, Elf32_Shdr* bss, Elf32_Shdr* relshn, Elf32_Rel* rel, Elf32_Shdr* target, Elf32_Shdr* symshn);
// Get a symbol pointer from an id and resolve it 
// if possible. If the function cannot resolve the
// symbol, it returns an error which can be checked
// and retreieved with IS_ERR and PTR_ERR respectively.
Elf32_Sym* elf_resolve_symbol(Elf32_Ehdr* ehdr, Elf32_Shdr* symshn, Elf32_Shdr* bss, Elf32_Word id);

Elf32_Sym* elf_find_symbol(Elf32_Shdr* shdr, Elf32_Sym* tab, const char* name);

// Check if this is a loadable executable
int elf_check_file(struct file* file);
// Load the file as an executable into memory
int elf_load_exec(exec_t* exec);
// Load the file as a loadable module into the kernel
int elf_load_module(exec_t* file);
// Load the file as a shared library
int elf_load_shared(struct file* file);

// Global Variable Definitions

// The kernel section header table
extern Elf32_Word g_shdr_num;
extern Elf32_Shdr* g_shdr;
extern Elf32_Shdr* g_symhdr;
extern Elf32_Sym* g_symtab;
extern Elf32_Shdr* g_shstrhdr;
extern char* g_shstrtab;

#endif