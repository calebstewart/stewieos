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

// The kernel section header and accompanying information
Elf32_Word g_shdr_num = 0;
Elf32_Shdr* g_shdr = NULL;
Elf32_Shdr* g_symhdr = NULL;
Elf32_Shdr* g_shstrhdr = NULL;
Elf32_Shdr* g_strhdr = NULL;
char* g_shstrtab = NULL;
char* g_strtab = NULL;
Elf32_Sym* g_symtab = NULL;

int elf_relocate(struct file* file, Elf32_Ehdr* ehdr, Elf32_Shdr* shtab, Elf32_Rel* rel, Elf32_Addr B, Elf32_Addr target, Elf32_Sym* stab, char* symstr, Elf32_Sword* pA);
int elf_resolve(struct file* file, Elf32_Ehdr* ehdr, Elf32_Sym* symbol, char* strtab);

// Defines the elf executable loader interface
exec_type_t elf_exec_type = {
	.name = "ELF32",
	.descr = "Intel 32-bit Executable and Linkable Format",
	.load_exec = elf_load_exec,
	.load_module = elf_load_module,
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

int elf_load_exec(exec_t* exec)
{
	// Check if this is a valid elf file of type ET_EXEC
	if( elf_check_ident((Elf32_Ehdr*)exec->buffer, ET_EXEC) != 0 ){
		return 0;
	}
	
	// Grab the elf header from the buffer
	Elf32_Exec elf;
	memcpy(&elf.ehdr, exec->buffer, sizeof(Elf32_Ehdr));
	
	
	
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
					elf_relocate(file, &ehdr, shtab, &rtab[r], (Elf32_Addr)module_base, shtab[shtab[i].sh_info].sh_offset - sizeof(Elf32_Ehdr), stab, symstr, NULL);
				}
			} else {
				for(size_t r = 0; r < (shtab[i].sh_size/sizeof(Elf32_Rela)); ++r){
					elf_relocate(file, &ehdr, shtab, (Elf32_Rel*)&((Elf32_Rela*)rtab)[r], shtab[shtab[i].sh_info].sh_offset - sizeof(Elf32_Ehdr), (Elf32_Addr)module_base, stab, symstr, &((Elf32_Rela*)rtab)[r].r_addend);
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

/* function: elf_apply_relocation
 * purpose: apply relocation information to an ELF Relocatable file
 * 	It can either be SHT_REL or SHT_RELA, if it is RELA, just cast the 
 * 	Elf32_Rela pointer to a Elf32_Rel pointer. It's fine. I swear.
 * parameters:
 * 	Elf32_Ehdr* ehdr - the header for the executable (should also be the laod address)
 * 	Elf32_Shdr* relshn - the section which holds the relocation table
 * 	Elf32_Rel* rel - the relocation entry. if this is a relocation with addend, then cast it to the Elf32_Rel
 * 	Elf32_Shdr* target - the target section pulled from the relshn information
 * 	Elf32_Shdr* symshn - the symbol table section also pulled from relshn information
 * return value:
 * 	0 on success or a negative error value.
 */
int elf_apply_relocation(Elf32_Ehdr* ehdr, Elf32_Shdr* bss, Elf32_Shdr* relshn, Elf32_Rel* rel, Elf32_Shdr* target, Elf32_Shdr* symshn)
{
	Elf32_Sword S = 0, A = 0, P = 0;
	Elf32_Sym* symbol = NULL;
	Elf32_Sword* R = NULL;
	
	// Calculate the address of the storage unit to be modified
	R = ((Elf32_Sword*)( (Elf32_Addr)ehdr + target->sh_offset + rel->r_offset ));
	
	// Make sure the symbol is fully resolved
	symbol = elf_resolve_symbol(ehdr, symshn, bss, ELF32_R_SYM(rel->r_info));
	if( IS_ERR(symbol) ){
		return PTR_ERR(symbol);
	}
	
	// Get the symbol value
	if( symbol->st_shndx == SHN_ABS ){
		S = symbol->st_value;
	} else {
		S = ((Elf32_Sword)(symbol->st_value + ELF_SHDR(ehdr, symbol->st_shndx)->sh_offset + (Elf32_Addr)ehdr));
	}
	// Get the relocation addend
	if( relshn->sh_type == SHT_RELA  ){
		A = ((Elf32_Rela*)rel)->r_addend;
	} else {
		A = *R;
	}
	// Get the pointer to the storage unit
	P = (Elf32_Sword)R;
	
	switch( ELF32_R_TYPE(rel->r_info) )
	{
		case R_386_COPY:
		case R_386_NONE:
			break;
		case R_386_32:
			*R = S + A;
			break;
		case R_386_PC32:
			*R = S + A - P;
			break;
		case R_386_GLOB_DAT:
		case R_386_JMP_SLOT:
			*R = S;
		default:
			printk("elf_apply_relocation: invalid relocation type: 0x%X\n", ELF32_R_TYPE(rel->r_info));
			return -EINVAL;
	}
	
	return 0;
}

int elf_resolve(struct file* file, Elf32_Ehdr* ehdr __attribute__((unused)), Elf32_Sym* symbol, char* strtab)
{
	// Undefined symbol
	if( symbol->st_shndx == SHN_UNDEF )
	{
		for(size_t i = 0; i < (g_symhdr->sh_size/sizeof(Elf32_Sym)); ++i)
		{
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

int elf_relocate(struct file* file, Elf32_Ehdr* ehdr, Elf32_Shdr* shtab __attribute__((unused)), Elf32_Rel* rel, Elf32_Addr B, Elf32_Addr target, Elf32_Sym* stab, char* symstr, Elf32_Sword* pA)
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
	if( (error = elf_resolve(file, ehdr, &stab[ELF32_R_SYM(rel->r_info)], symstr)) != 0 ){
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

/* function: elf_init_module
 * purpose: initialize a module that has been loaded into memory
 * 	The module should be of ELF32 format and have the type
 * 	ET_REL (elf relocatable image, e.g. an object file).
 * 	The object must also have a section named ".module_info"
 * 	which contains a single module_t data structure with valid
 * 	pointers to module related funcitonality.
 * 	
 * 	See `module_t'
 * parameters:
 * 	void* file - a buffer containing the fully loaded module in memory
 * return value:
 * 	A negative error value on failure or a pointer to the module data structure.
 * 	
 * 	You can use IS_ERR to test for failure.
 */
module_t* elf_init_module(void* file, size_t length)
{
	Elf32_Ehdr* ehdr = (Elf32_Ehdr*)file;
	int error = 0;
	Elf32_Shdr* bss = NULL;
	
	if( elf_check_ident(ehdr, ET_REL) != 0 ){
		return ERR_PTR(-ENOEXEC);
	}
	
	// Get the section table
	Elf32_Shdr* shdr = (Elf32_Shdr*)( (Elf32_Addr)file + ehdr->e_shoff );
	// Get the section string table
	char* shstrtab = (char*)( (Elf32_Addr)file + shdr[ehdr->e_shstrndx].sh_offset );
	// A pointer to the module_info structure
	module_t* module = NULL;
	
	// we need to find the bss section first
	for(int i = 0; i < ehdr->e_shnum; i++)
	{
		if( strcmp(&shstrtab[shdr[i].sh_name], ".bss") == 0 )
		{
			bss = &shdr[i];
		}
	}
	
	// now we can do everything else
	for(int i = 0; i < ehdr->e_shnum; i++)
	{
		if( strcmp(&shstrtab[shdr[i].sh_name], ".module_info") == 0 ){
			module = (module_t*)( (Elf32_Addr)file + shdr[i].sh_offset );
		} else if( shdr[i].sh_type == SHT_REL )
		{
			Elf32_Rel* reltab = (Elf32_Rel*)( (Elf32_Addr)file + shdr[i].sh_offset );
			Elf32_Shdr* target = &shdr[shdr[i].sh_info];
			Elf32_Shdr* symtab = &shdr[shdr[i].sh_link];
			for(int r = 0; r < (int)( shdr[i].sh_size / shdr[i].sh_entsize ); r++){
				error = elf_apply_relocation(ehdr, bss, &shdr[i], &reltab[r], target, symtab);
				if( error != 0 ){
					return ERR_PTR(error);
				}
			}
		} else if( shdr[i].sh_type == SHT_RELA )
		{
			Elf32_Rela* reltab = (Elf32_Rela*)( (Elf32_Addr)file + shdr[i].sh_offset );
			Elf32_Shdr* target = &shdr[shdr[i].sh_info];
			Elf32_Shdr* symtab = &shdr[shdr[i].sh_link];
			for(int r = 0; r < (int)( shdr[i].sh_size / shdr[i].sh_entsize ); r++){
				error = elf_apply_relocation(ehdr, bss, &shdr[i], (Elf32_Rel*)&reltab[r], target, symtab);
				if( error != 0 ){
					return ERR_PTR(error);
				}
			}
		} else if( strcmp(&shstrtab[shdr[i].sh_name], ".bss") == 0 )
		{
			bss = &shdr[i];
		} else if( shdr[i].sh_type == SHT_NOBITS && (shdr[i].sh_flags & SHF_ALLOC) ){
			// We can't have a nobits section that needs allocated other than the .bss
			// section because the .bss section is specially accounted for. Who needs
			// that kind of section for anything other than the .bss anyway?
			printk("elf_init_module: error: section %d (idx: %d) has SHT_NOBITS and SHF_ALLOC set.\n", shdr[i].sh_name, i);
			return ERR_PTR(-EINVAL);
		}
	}
	
	if( module == NULL ){
		printk("elf_init_module: module does not include a module_info structure.\n");
		return ERR_PTR(-EINVAL);
	}
	
	// The bss section needs enough space, but we aren't messing with the file, which
	// presents a problem. If we insert space into the file, we have to do gross things
	// with file offsets within data structures :(, so instead we wait until the file is
	// completely loaded and use the fact that the BSS is the last section which is needed
	// at runtime and tack it on after the .bss section, regardless of sections that follow it.
	// This will probably also overwrite the section table so it has to be last.
	if( bss )
	{
		// this means we need to reallocate the data to fit the BSS
		// section, because there isn't enough room already... :(
		if( bss->sh_size > (length - bss->sh_offset) )
		{
			void* newfile = kmalloc(bss->sh_size + bss->sh_offset);
			if( !newfile ){
				return ERR_PTR(-ENOMEM);
			}
			memcpy(newfile, file, length);
			kfree(file);
			file = newfile;
		}
		Elf32_Addr ptr = (Elf32_Addr)file + bss->sh_offset;
		for(Elf32_Off i = 0; i < bss->sh_size; i++){
			*((char*)( ptr + i )) = 0;
		}
	}
	
	// We set this just in  case it was changed for the bss
	module->m_loadaddr = file;
	
	return module;
}

/* function: elf_resolve_symbol
 * purpose: resolve and retrieve a symbol pointer
 * 	If needed, the bss will be expanded in order to accomadate a 
 * 	Common symbol. Because of this, this function should be called
 * 	before the bss is allocated and initiated, otherwise common
 * 	symbols will not be allocated correctly.
 * parameters:
 * 	Elf32_Ehdr* ehdr - The elf header, and the base address of the entire object file in memory
 * 	Elf32_Shdr* symshn - The section containing the symbol data
 * 	Elf32_Shdr* bss - The section containing the bss data
 * 	Elf32_Word id - the symbol identification number
 * return value:
 * 	A pointer to the symbol or a negative error value.
 * 	
 * 	The Return value may be checked for error with IS_ERR and the
 * 	specific error code can be retrieved with PTR_ERR.
 */
Elf32_Sym* elf_resolve_symbol(Elf32_Ehdr* ehdr, Elf32_Shdr* symshn, Elf32_Shdr* bss, Elf32_Word id)
{
	Elf32_Sym* symtab = (Elf32_Sym*)( (Elf32_Addr)ehdr + symshn->sh_offset );
	Elf32_Shdr* shdr = (Elf32_Shdr*)( (Elf32_Addr)ehdr + ehdr->e_shoff );
	char* strtab = (char*)( (Elf32_Addr)ehdr + shdr[symshn->sh_link].sh_offset );
	
	// We need to load in the symbol table for the kernel
	// so we can link modules to the kernel, but for now
	// just make the executables invalid...
	if( symtab[id].st_shndx == SHN_UNDEF ){
		printk("%2Velf_resolve_symbol: unable to resolve symbol `%s'.\n", strtab[symtab[id].st_name]);
		return ERR_PTR(-EINVAL);
	}
	
	// This means that the object expects the linker (a.k.a. us) to allocate
	// some memory for it. We will use the BSS since we know it hasn't been
	// allocated yet anyway. We just change its' size.
	if( symtab[id].st_shndx == SHN_COMMON ){
		size_t size = symtab[id].st_size;
		Elf32_Addr next_bss_addr = (Elf32_Addr)( (Elf32_Addr)ehdr + bss->sh_offset );
		if( next_bss_addr % symtab[id].st_value ){
			// align to the requested bounds
			size += symtab[id].st_value - (next_bss_addr % symtab[id].st_value);
		}
		symtab[id].st_shndx = SHN_ABS;
		symtab[id].st_value = next_bss_addr + size - symtab[id].st_size;
		bss->sh_size += size;
	}
	
	// The only other option is SHN_ABS, therefore we can return the symbol
	// as is.
	return &symtab[id];
}

int elf_read_module(struct file* file);

int elf_read_header(struct file* file, Elf32_Ehdr* ehdr)
{
	//int result = 0; // store various results
	
	file_seek(file, 0, SEEK_SET);
	file_read(file, ehdr, sizeof(Elf32_Ehdr));
	
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

int elf_read_section_header_list(struct file* file, Elf32_Ehdr* elf, Elf32_Shdr* buffer)
{
	ssize_t result = 0;
	Elf32_Off offset = elf->e_shoff;
	Elf32_Half size = (Elf32_Half)(elf->e_shentsize * elf->e_shnum);
	
	file_seek(file, offset, SEEK_SET);
	result = file_read(file, buffer, size);
	
	if( result != size ){
		return -ENOEXEC;
	}
	
	return 0;
}
/*
int elf_load_section(struct file* file, Elf32_Ehdr* elf, Elf32_Shdr* shn, Elf32_Addr base_addr, Elf32_Addr load_addr, int user)
{
	// the section is not part of the process image
	if( !(shn->sh_flags & SHF_ALLOC) ){
		return 0;
	}
	// the section has no bits for the process image in the file
	if( shn->sh_type == SHT_NOBITS ){
		return 0;
	}
	
	// For allocating pages, should they be read or write;
	int write = (shn->sh_flags & SHF_WRITE) > 0;
	// Calculate the load address of this section
	void* addr = ((shn->sh_addr-base_addr) + load_addr) & 0xFFFFF000;
	
	// Position the file at the beginning of this section
	file_seek(file, (off_t)shn->sh_offset, SEEK_SET);
	
	while( (Elf32_Word)addr < (shn->sh_addr+shn->sh_size) )
	{
		// Allocate the page
		alloc_page(curdir, addr, user, write);
		// Invalidate the TLB entry
		invalidate_page((u32*)addr);
		// Read the data into memory
		file_read(file, addr, 0x1000);
		// Move to next block
		addr = addr + 0x1000;
	}
	
	return 0;
}

*/
