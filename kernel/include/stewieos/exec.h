#ifndef _EXEC_H_
#define _EXEC_H_

#include "stewieos/fs.h"
#include "stewieos/linkedlist.h"

#define EXEC_VALID 0
#define EXEC_INTERP 1
#define EXEC_INVALID 2

// Define the module info structure for a loadable module
#define MODULE_INFO(name, load, remove) module_t __moduel_info __attribute__((section(".module_info"))) = { .m_name=(name), .m_load=(load), .m_remove=(remove) }

/* type: exec_t
 * purpose: holds executable information while the executable is
 * 	being loaded.
 */
typedef struct _exec
{
	char buffer[256];	// First 256 bytes of the file
	struct file* file;	// The file this executable is read from
	char** argv;		// Argument List
	char** envp;		// Environment List
	size_t argc;
	size_t envc;
	void* entry;		// The Entry Address
	void* bssend;		// end of process data (for sbrk)
	void* private;		// Private loader data
} exec_t;

/* type: module_t
 * purpose: holds module data including loading, and unloading functions
 */
struct _module;
typedef struct _module module_t;
struct _module
{
	const char* m_name;		// Name of the module
	int(*m_load)(module_t*);	// Module load functions
	int(*m_remove)(module_t*);	// Module remove function
	void* m_loadaddr;		// Address to which the module was loaded (the address to kfree)
	int m_refs;			// Number of references currently held throughout the system
	
	list_t m_link;			// Link in the kernel module list
	void* m_private;		// Private module data
};

/* type: exec_type_t
 * purpose: holds functions for loading different types of
 * 	executables
 */
struct _exec_type;
typedef struct _exec_type exec_type_t;
struct _exec_type
{
	const char* name;
	const char* descr;
	
	/* function: load_exec
	 * parameters:
	 * 	exec_t* - The executable structure holding file and parameter information
	 * return value:
	 * 	Returns 1 on successfull loading of the executable
	 * 	Returns 0 if the executable was not of this type
	 * 	Returns negative error values if the file was of this type, but contained errors
	 */
	int(*load_exec)(exec_t*);
	/* function: read_page
	 * parameters:
	 * 	exec_t* - The executable structure currently being executed
	 * 	void* - the page to be read
	 * return values:
	 * 	Returns 1 if the page was read successfully
	 * 	Returns 0 if the page did not exist within the executable
	 * 	Returns negative error values if there was an error reading the file
	 * notes:
	 * 	This function is optional. You only need it if you are implementing "load on request"
	 * 	style program loading.
	 */
	int(*read_page)(exec_t*,void*);
	/* function: load_module
	 * parameters:
	 * 	struct file* - the file to load from
	 * return values:
	 * 	Returns NULL if the file was not of this type
	 * 	Returns a module_t structure on success
	 * 	Returns a negative error value (see IS_ERR/PTR_ERR) if the file was of this type but had problems
	 * notes:
	 * 	You should fully load and link the module in one kmalloc'd memory
	 * 	block, and return a module_t structure from within the block (it should
	 * 	be part of the memory image). The module_t structure should have all callbacks
	 * 	and module pointers filled in (reference counting and lists can be empty).
	 */
	module_t*(*load_module)(struct file*);
	/* function: check_exec
	 * parameters:
	 * 	const char* - the filepath used to execute this file
	 *	exec_t* - the executable structure currently being loaded
	 * return values:
	 * 	Returns 2 if the executable is of the correct format but requires an interpreter
	 * 	Returns 1 if the executable is of the correct format and can be loaded
	 *	Returns 0 if the executable is invalid for the current system.
	 */
	int(*check_exec)(const char*, exec_t*);
	
	exec_type_t* next;
};

// Load a new executable from the given file
int load_exec(const char* filename, char** argv, char** envp);
// Load and insert a new module into the system
int load_module(const char* filename);
// Remove a module from the system
int remove_module(const char* name);
// 

// Load an Elf32 Module into the kernel
int sys_insmod(const char* filename);
// Remove module from the kernel
int sys_rmmod(const char* module_name);
// Execute a file
int sys_execve(const char* filename, char** argv, char** envp);

// Call this function to close the executable and free all memory associated
// You should not use or dereference the pointer after calling this function.
void close_exec(exec_t* exec);

void register_exec_type(exec_type_t* type);

#endif
