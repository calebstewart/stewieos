#include "kernel.h"
#include "elf/elf32.h"
#include "exec.h"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "error.h"
#include "kmem.h"
#include "multiboot.h"
#include "dentry.h"
#include "paging.h"

// The kernel section header and accompanying information
Elf32_Word g_shdr_num = 0;
Elf32_Shdr* g_shdr = NULL;
Elf32_Shdr* g_symhdr = NULL;
Elf32_Shdr* g_shstrhdr = NULL;
Elf32_Shdr* g_strhdr = NULL;
char* g_shstrtab = NULL;
char* g_strtab = NULL;
Elf32_Sym* g_symtab = NULL;

int elf_relocate(struct file* file, Elf32_Ehdr* ehdr, Elf32_Shdr* shtab, Elf32_Rel* rel, Elf32_Addr B, Elf32_Addr target, Elf32_Sym* stab, char* symstr, size_t symstrlen, Elf32_Sword* pA);
int elf_resolve(struct file* file, Elf32_Ehdr* ehdr, Elf32_Sym* symbol, char* strtab, size_t symstrlen);
void elf_allocate_range(Elf32_Addr start, Elf32_Addr end, int user, int rw);
void elf_modify_range(Elf32_Addr start, Elf32_Addr end, int user, int rw);
int elf_check_exec(exec_t* exec);

// Defines the elf executable loader interface
exec_type_t elf_exec_type = {
	.name = "ELF32",
	.descr = "Intel 32-bit Executable and Linkable Format",
	.load_exec = elf_load_exec,
	.load_module = elf_load_module,
	.check_exec = elf_check_exec,
};

#define ELF_SHDR(elf, shndx) ((Elf32_Shdr*)( (Elf32_Addr)(elf) + (elf)->e_shoff + (shndx)*sizeof(Elf32_Shdr) ))

void elf_register( void )
{
	register_exec_type(&elf_exec_type);
	
	if( !(g_multiboot->flags & MULTIBOOT_INFO_ELF_SHDR) ){
		printk("elf_register: no kernel symbol information found!\n");
		return;
	}
	
	multiboot_elf_section_header_table_t* kerntbl = &g_multiboot->u.elf_sec;
	g_shdr = (Elf32_Shdr*)kerntbl->addr;
	g_shstrhdr = &g_shdr[kerntbl->shndx];
	g_shstrtab = (char*)(g_shdr[kerntbl->shndx].sh_addr + KERNEL_VIRTUAL_BASE);
	
	for(size_t i = 0; i < kerntbl->num; ++i)
	{
		if( strcmp(&g_shstrtab[g_shdr[i].sh_name], ".strtab") == 0 ){
			g_strhdr = &g_shdr[i];
			g_strtab = (char*)(g_shdr[i].sh_addr + KERNEL_VIRTUAL_BASE);
		} else if( strcmp(&g_shstrtab[g_shdr[i].sh_name], ".symtab") == 0 ){
			g_symtab = (Elf32_Sym*)(g_shdr[i].sh_addr + KERNEL_VIRTUAL_BASE);
			g_symhdr = &g_shdr[i];
		}
	}
}

void elf_allocate_range(Elf32_Addr start, Elf32_Addr end, int user, int rw)
{
	while( start <= end ){
		alloc_page(curdir, (void*)start, user, rw);
		start += 0x1000;
	}
}

void elf_modify_range(Elf32_Addr start, Elf32_Addr end, int user, int rw)
{
	while( start <= end ){
		page_t* page = get_page(curdir, 1, (void*)start);
		page->rw = (unsigned char)(rw & 1);
		page->user = (unsigned char)(user & 1);
		start += 0x1000;
	}
}

int elf_check_exec(exec_t* exec)
{
	Elf32_Ehdr* ehdr = (Elf32_Ehdr*)exec->buffer;
	
	if( elf_check_ident(ehdr, ET_EXEC) != 0 ){
		return 0;
	}
	
	return 1;
}

int elf_load_exec(exec_t* exec)
{
	Elf32_Ehdr* ehdr = (Elf32_Ehdr*)exec->buffer;
	Elf32_Phdr* phdr = NULL;
	
	// Check if this is a valid elf file of type ET_EXEC
	if( elf_check_ident(ehdr, ET_EXEC) != 0 ){
		return 0;
	}
	
	phdr = (Elf32_Phdr*)kmalloc(ehdr->e_phnum * ehdr->e_phentsize);
	if( !phdr ){
		return -ENOMEM;
	}
	
	file_seek(exec->file, ehdr->e_phoff, SEEK_SET);
	file_read(exec->file, phdr, ehdr->e_phnum * ehdr->e_phentsize);
	
	for(Elf32_Word i = 0; i < ehdr->e_phnum; ++i)
	{
		if( phdr[i].p_type == PT_LOAD )
		{
			// Allocate the virtual pages for this program segment
			elf_allocate_range(phdr[i].p_vaddr & 0xFFFFF000, (phdr[i].p_vaddr + phdr[i].p_memsz) & 0xFFFFF000, 1, 1);
			// Read in the segment
			file_seek(exec->file, phdr[i].p_offset, SEEK_SET);
			file_read(exec->file, (void*)phdr[i].p_vaddr, phdr[i].p_filesz);
			// Clear extra "uninitialized" memory
			if( (phdr[i].p_memsz - phdr[i].p_filesz) > 0  ) {
				memset((void*)( phdr[i].p_vaddr + phdr[i].p_filesz ), 0, phdr[i].p_memsz - phdr[i].p_filesz);
			}
		}
		
	}
	
	exec->entry = (void*)ehdr->e_entry;
	
	return 0;
}

module_t* elf_load_module(struct file* file)
{
	Elf32_Ehdr ehdr; // elf header
	void* module_base = NULL; // address to load the module
	size_t module_length = 0; // the total length of the module in memory
	Elf32_Shdr* shtab = NULL; // section header table
	char* shstrtab = NULL; // Section header string table
	module_t* module_info = NULL; // the module info structure (loaded from the .module_info section)
	Elf32_Shdr* module_shdr = NULL; // module_info section header
	
	// Read in the Elf Header
	file_seek(file, 0, SEEK_SET);
	file_read(file, &ehdr, sizeof(Elf32_Ehdr));
	
	// Check for correct magic numbers, file type, etc.
	if( elf_check_ident(&ehdr, ET_REL) ){
		return NULL;
	}
	
	// Allocate space for the section header table
	shtab = (Elf32_Shdr*)kmalloc(ehdr.e_shnum * ehdr.e_shentsize);
	if(!shtab){
		return ERR_PTR(-ENOMEM);
	}
	
	// Read the section header table
	file_seek(file, ehdr.e_shoff, SEEK_SET);
	file_read(file, shtab, ehdr.e_shnum * ehdr.e_shentsize);
	
	// Allocate space for the section header string table
	shstrtab = (char*)kmalloc(shtab[ehdr.e_shstrndx].sh_size);
	if(!shstrtab){
		kfree(shtab);
		return ERR_PTR(-ENOMEM);
	}
	
	// Read the section header string table
	file_seek(file, shtab[ehdr.e_shstrndx].sh_offset, SEEK_SET);
	file_read(file, shstrtab, shtab[ehdr.e_shstrndx].sh_size);
	
	// Find the size of the module in memory by finding largest
	// address. The addresses have a base of zero, so the largest
	// addresses will also be the size needed in memory.
	// 
	// This loop also looks for the module_info section which must
	// be present for a correct module load
	for(size_t i = 0; i < ehdr.e_shnum; ++i)
	{
		if( !(shtab[i].sh_flags & SHF_ALLOC) ) continue;
		
		// Check if this is the module info section
		if( strcmp(&shstrtab[shtab[i].sh_name], ".module_info") == 0 ){
			module_shdr = &shtab[i];
		}
		
		// Check if this is the farthest section
		if( (shtab[i].sh_offset+shtab[i].sh_size-sizeof(Elf32_Ehdr)) > module_length ){
			module_length = shtab[i].sh_offset+shtab[i].sh_size-sizeof(Elf32_Ehdr);
		}
	}
	
	// There must be a module info section
	if( !module_shdr ){
		printk("No module info section!\n");
		kfree(shstrtab);
		kfree(shtab);
		return ERR_PTR(-EINVAL);
	} else {
		module_info = (module_t*)(module_shdr->sh_offset-sizeof(Elf32_Ehdr));
	}
	
	// Allocate a place for the module in memory
	module_base = kmalloc(module_length);
	if( !module_base ){
		kfree(shtab);
		return ERR_PTR(-ENOMEM);
	}
	
	// Correct the module info pointer for the new module base address
	module_info = (module_t*)( (Elf32_Addr)module_info + (Elf32_Addr)module_base );
	
	// Read in all sections with the SHF_ALLOC flag
	// and clear all SHT_NOBITS sections
	for(size_t i = 0; i < ehdr.e_shnum; ++i)
	{
		if( !(shtab[i].sh_flags & SHF_ALLOC) ) continue;
		
		if( shtab[i].sh_type == SHT_NOBITS ){
			memset((void*)(shtab[i].sh_offset + (u32)module_base - sizeof(Elf32_Ehdr)), 0, shtab[i].sh_size);
		} else {
			file_seek(file, shtab[i].sh_offset, SEEK_SET);
			file_read(file, (void*)(shtab[i].sh_offset + (Elf32_Addr)module_base - sizeof(Elf32_Ehdr)), shtab[i].sh_size);
		}
	}
	
	// Now that all the sections are loaded, we can do relocations
	for(size_t i = 0; i < ehdr.e_shnum; ++i)
	{
		//if( !(shtab[i].sh_flags & SHF_ALLOC) ) continue;
		if( shtab[i].sh_type == SHT_REL || shtab[i].sh_type == SHT_RELA )
		{
			// Allocate space for the relocation table
			Elf32_Rel* rtab = (Elf32_Rel*)kmalloc(shtab[i].sh_size);
			if(!rtab){
				kfree(shtab);
				kfree(shstrtab);
				kfree(module_base);
				return ERR_PTR(-ENOMEM);
			}
			// Locate the target section address
//			char* target = (char*)module_base + shtab[shtab[i].sh_info].sh_addr;
			// Allocate space for the symbol table
			Elf32_Shdr* symhdr = &shtab[shtab[i].sh_link];
			Elf32_Sym* stab = (Elf32_Sym*)kmalloc(symhdr->sh_size);
			if( !stab ){
				kfree(rtab);
				kfree(shstrtab);
				kfree(shtab);
				kfree(module_base);
				return ERR_PTR(-ENOMEM);
			}
			// Allocate space for the symbol string table
			char* symstr = (char*)kmalloc(shtab[symhdr->sh_link].sh_size);
			if( !stab ){
				kfree(rtab);
				kfree(shstrtab);
				kfree(shtab);
				kfree(module_base);
				kfree(stab);
				return ERR_PTR(-ENOMEM);
			}
			
			// Read in the relocation table
			file_seek(file, shtab[i].sh_offset, SEEK_SET);
			file_read(file, rtab, shtab[i].sh_size);
			// Read in the symbol table
			file_seek(file, symhdr->sh_offset, SEEK_SET);
			file_read(file, stab, symhdr->sh_size);
			// Read in the symbol string table
			file_seek(file, shtab[symhdr->sh_link].sh_offset, SEEK_SET);
			file_read(file, symstr, shtab[symhdr->sh_link].sh_size);
			
			
			
			// Apply The Relocations
			if( shtab[i].sh_type == SHT_REL ){
				for(size_t r = 0; r < (shtab[i].sh_size/sizeof(Elf32_Rel)); ++r){
					elf_relocate(file, &ehdr, shtab, &rtab[r], (Elf32_Addr)module_base, shtab[shtab[i].sh_info].sh_offset - sizeof(Elf32_Ehdr), stab, symstr, shtab[symhdr->sh_link].sh_size, NULL);
				}
			} else {
				for(size_t r = 0; r < (shtab[i].sh_size/sizeof(Elf32_Rela)); ++r){
					elf_relocate(file, &ehdr, shtab, (Elf32_Rel*)&((Elf32_Rela*)rtab)[r], (Elf32_Addr)module_base, shtab[shtab[i].sh_info].sh_offset - sizeof(Elf32_Ehdr), stab, symstr,shtab[symhdr->sh_link].sh_size, &((Elf32_Rela*)rtab)[r].r_addend);
				}
			}
			
			kfree(rtab);
			kfree(stab);
			kfree(symstr);
			
		}
	}
	
	// Free unneeded tables
	kfree(shtab);
	kfree(shstrtab);
	
	module_info->m_loadaddr = module_base;
	
	return module_info;
}

int elf_resolve(struct file* file, Elf32_Ehdr* ehdr __attribute__((unused)), Elf32_Sym* symbol, char* strtab, size_t strtablen)
{
	// Undefined symbol
	if( symbol->st_shndx == SHN_UNDEF )
	{
		for(size_t i = 0; i < (g_symhdr->sh_size/sizeof(Elf32_Sym)); ++i)
		{
			if( symbol->st_name >= strtablen ){
				continue;
			}
			if( strcmp(&g_strtab[g_symtab[i].st_name], &strtab[symbol->st_name]) == 0 ){
				symbol->st_value = g_symtab[i].st_value;
				symbol->st_shndx = SHN_ABS;
				return 0;
			}
		}
		printk("elf_resolve: %s: unresolved symbol %s\n", file_dentry(file)->d_name, &strtab[symbol->st_name]);
		return -EINVAL;
	}
	
	// Common symbol
	if( symbol->st_shndx == SHN_COMMON )
	{
		printk("elf_resolve: %s: common symbols are unsupported in modules!\n", file_dentry(file)->d_name);
		return -EINVAL;
	}
	
	
	
	// Absolute symbol
	return 0;
}

int elf_relocate(struct file* file, Elf32_Ehdr* ehdr, Elf32_Shdr* shtab __attribute__((unused)), Elf32_Rel* rel, Elf32_Addr B, Elf32_Addr target, Elf32_Sym* stab, char* symstr, size_t symstrlen, Elf32_Sword* pA)
{
	int error = 0;
	Elf32_Sword* P = NULL;
	Elf32_Addr S = 0;
	
	// r_offset has different interpretations for ET_REL vs. ET_EXEC
	if( ehdr->e_type == ET_REL ){
		P = (Elf32_Sword*)(target + B + rel->r_offset);
	} else {
		P = (Elf32_Sword*)rel->r_offset;
	}
	
	// Implicit Addend
	if( pA == NULL ) {
		pA = P;
	}
	
	// Get the value of the symbol
	if( (error = elf_resolve(file, ehdr, &stab[ELF32_R_SYM(rel->r_info)], symstr, symstrlen)) != 0 ){
		return error;
	}
	S = stab[ELF32_R_SYM(rel->r_info)].st_value;
	// Not an absolute symbol, so we need to calculate its address
	if( stab[ELF32_R_SYM(rel->r_info)].st_shndx != SHN_ABS ){
		S += shtab[stab[ELF32_R_SYM(rel->r_info)].st_shndx].sh_offset - sizeof(Elf32_Ehdr) + B;
	}
	
	switch( ELF32_R_TYPE(rel->r_info) )
	{
		case R_386_COPY:
		case R_386_NONE:
			break;
		case R_386_32:
			*P = S + *pA;
			break;
		case R_386_PC32:
			*P = S + *pA - (Elf32_Sword)P;
			break;
		case R_386_GLOB_DAT:
		case R_386_JMP_SLOT:
			*P = S;
			break;
		default:
			printk("elf_relocate: unsupported/invalid relocation type: 0x%X\n", ELF32_R_TYPE(rel->r_info));
			return -EINVAL;
	}
	
	return 0;
	
}

int elf_check_ident(Elf32_Ehdr* elf, Elf32_Half type)
{
	if( ( (elf)->e_ident[EI_MAG0] == ELFMAG0 && \
				(elf)->e_ident[EI_MAG1] == ELFMAG1 && \
				(elf)->e_ident[EI_MAG2] == ELFMAG2 && \
				(elf)->e_ident[EI_MAG3] == ELFMAG3 && \
				(elf)->e_ident[EI_CLASS] == ELFCLASS32 && \
				(elf)->e_ident[EI_DATA] == ELFDATALSB && \
	  			(elf)->e_type == type ) )
	{
		return 0;
	} else {
		return -ENOEXEC;
	}
}
