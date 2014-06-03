#ifndef _EXEC_H_
#define _EXEC_H_

#include "fs.h"
#include "linkedlist.h"

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
	void* private;		// Private loader data
} exec_t;

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
	int(*load_exec)(exec_t*);
	int(*load_module)(exec_t*);
	
	exec_type_t* next;
};

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

// Call this function to close the executable and free all memory associated
// You should not use or dereference the pointer after calling this function.
void close_exec(exec_t* exec);

void register_exec_type(exec_type_t* type);

#endif
