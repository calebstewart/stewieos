#include "kernel.h"
#include "exec.h"
#include "fs.h"
#include <fcntl.h>
#include <sys/stat.h>
#include "error.h"
#include "kmem.h"
#include "unistd.h"
#include "elf/elf32.h"
#include "paging.h"
#include "task.h"
#include "ksignal.h"

exec_type_t* g_exec_type = NULL;
list_t g_module_list = LIST_INIT(g_module_list);

int add_module(module_t* module);
int rem_module(module_t* module);
module_t* get_module(const char* name);

// add a newly loaded module to the module list
int add_module(module_t* module)
{
	int error = 0;
	
	INIT_LIST(&module->m_link);
	list_add(&module->m_link, &g_module_list);
	
	if( module->m_load ){
		error = module->m_load(module);
		if( error != 0 ){
			list_rem(&module->m_link);
			return error;
		}
	}
	
	return 0;
}

// remove a loaded module from the module list
// also free it's memory. module is no longer a
// valid pointer after this function
int rem_module(module_t* module)
{
	int error = 0;
	
	if( module->m_remove ){
		error = module->m_remove(module);
		if( error != 0 ){
			return error;
		}
	}
	
	list_rem(&module->m_link);
	
	/*! NOTE I need to finish removing the allocated data and 
		associated things, but I need to write the loading
		function so I know what the removing looks like :P
	*/
	kfree(module->m_loadaddr);
	
	return 0;
}

int sys_execve(const char* filename, char** argv, char** envp)
{
	struct path path;
	exec_t* exec = NULL;
	int error = 0;
	int check = 0;
	
	// Lookup the path from the filename
	error = path_lookup(filename, WP_DEFAULT, &path);
	// Did the file exist?
	if( error != 0 ){
		return error;
	}
	
	if( path_access(&path, X_OK) != 0 ){
		return -EPERM;
	}
	
	// Open the file
	struct file* filp = file_open(&path, O_RDONLY);
	// We don't need the path reference anymore
	path_put(&path);
	
	// Could the file be opened?
	if( IS_ERR(filp) ){
		return PTR_ERR(filp);
	}
	
	// Allocate the executable structure
	exec = (exec_t*)kmalloc(sizeof(exec_t));
	memset(exec, 0, sizeof(exec_t));
	
	// Initialize the structure
	exec->file = filp;
	
	// Count the arguments
	u32 argc = 0;
	while( argv[argc] ) ++argc;
	u32 envc = 0;
	while( envp[envc] ) ++envc;
	
	char** nargv = (char**)kmalloc(sizeof(char*)*(argc+1));
	char** nenvp = (char**)kmalloc(sizeof(char*)*(envc+1));
	
	for(u32 i = 0; i < argc; ++i){
		nargv[i] = (char*)kmalloc(strlen(argv[i]));
		strcpy(nargv[i], argv[i]);
	}
	for(u32 i = 0; i < envc; ++i){
		nenvp[i] = (char*)kmalloc(strlen(envp[i]));
		strcpy(nenvp[i], envp[i]);
	}
	
	nargv[argc] = NULL;
	nenvp[envc] = NULL;
	
	// Load the executable structure with the kernel side argument lists
	exec->argv = nargv;
	exec->envp = nenvp;
	exec->argc = argc;
	exec->envc = envc;
	
	// Fill the read buffer
	file_seek(filp, 0, SEEK_SET);
	file_read(filp, exec->buffer, 256);
	
	// Check all types. Use the first acceptable type
	exec_type_t* type = g_exec_type;
	while( type != NULL )
	{
		// a type can implement load_exec, load_module, or both.
		if( type->load_exec )
		{
			check = type->check_exec(filename, exec);
			if( check == EXEC_VALID ) break; // valid
			else if( check == EXEC_INTERP ) { // restart the search
				type = g_exec_type;
				continue;
			} else if( check != EXEC_INVALID ) {
				// right type, but had some other error
				return check;
			}
		}
		type = type->next;
	}
	
	if( type == NULL ){
		file_close(filp);
		kfree(exec);
		return -ENOEXEC;
	}
	
	// Calculate total stack size for the argument strings
	u32 argsz = 0;
	for(u32 i = 0; i < exec->argc; ++i) argsz += strlen(exec->argv[i])+1;
	u32 envsz = 0;
	for(u32 i = 0; i < exec->envc; ++i) envsz += strlen(exec->envp[i])+1;
		
	// Enough room for the arrays and the strings themselves
	size_t total_argsz = argsz + envsz;
	total_argsz += sizeof(char*)*(exec->envc+1) + sizeof(char*)*(exec->argc+1);
	// Check if we are requesting too much space
	if( total_argsz > TASK_MAX_ARG_SIZE ){
		return file_close(filp);
		kfree(exec);
		return -E2BIG;
	}
	
	// Create an empty page directory and free the old one (there's no going back from here...)
	strip_page_dir(curdir);
	signal_init(current);

	// Allocate the signal stack
	for( u32 addr = TASK_SIGNAL_STACK-TASK_SIGNAL_STACK_SIZE;
			addr < TASK_SIGNAL_STACK; addr += 0x1000 ){
		alloc_page(curdir, (void*)addr, 1, 1);
	}
	
	// Allocate the new tasks user stack space
	for(u32 addr = TASK_STACK_INIT_BASE; addr < TASK_STACK_START; addr += 0x1000)
	{
		alloc_page(curdir, (void*)addr, 1, 1);
	}
	
	// Calculate argument and environment pointers within the users stack
	argv = (char**)(TASK_STACK_START-total_argsz);
	envp = (char**)( (char*)argv + sizeof(char*)*(exec->argc+1) );
	char* str = (char*)( (char*)envp + sizeof(char*)*(exec->envc+1) );
	
	// Copy the argument/environment arrays/strings into the users stack
	for(u32 i = 0; i < exec->argc; ++i){
		argv[i] = str;
		strcpy(str, exec->argv[i]);
		str += strlen(str)+1;
	}
	argv[exec->argc] = NULL;
	for(u32 i = 0; i < exec->envc; ++i){
		envp[i] = str;
		strcpy(str, exec->envp[i]);
		str += strlen(str)+1;
	}
	envp[exec->envc] = NULL;
	
	// Load the executable
	error = type->load_exec(exec);
	// Hmmm... whoops...
	if( error != 0 ){
		sys_exit(error);
		while(1);
	}
	
	// These are the actual arguments to main (argv and envp)
	*(char***)( (u32)argv - 8 ) = argv;
	*(char***)( (u32)argv - 4 ) = envp;
	*(int*)( (u32)argv - 12 ) = exec->argc;
	
	// Free the kernel argument pointers
	for(u32 i = 0; i < exec->argc; ++i) kfree(exec->argv[i]);
	kfree(exec->argv);
	for(u32 i = 0; i < exec->envc; ++i) kfree(exec->envp[i]);
	kfree(exec->envp);
	
	// These are only valid within this page directory though...
	exec->argv = argv;
	exec->envp = envp;
	
	// Fill the Registers structure so we start at the correct place
	// and in user mode
	memset(&current->t_regs, 0, sizeof(current->t_regs));
	current->t_regs.eip = (u32)exec->entry;
	current->t_regs.useresp = (u32)argv - 12;
	current->t_regs.eflags = 0x200200;
	current->t_regs.cs = 0x1B;
	current->t_regs.ss = 0x23;
	current->t_regs.ds = 0x23;
	// Tell the scheduler that we switched 
	// and also give up our time slice so we
	// finish switching as soon as possible.
	current->t_flags |= TF_EXECVE;
	current->t_ticks_left = 0;
	current->t_dataend = (u32)exec->bssend;

	// Close the file and free the memory
	close_exec(exec);
	
	// Interrupts may have been disabled, just enabled them just in case
	// It shouldn't matter at this point anyway.
	asm volatile("sti");
	// Loop until a timer interrupt fires (in which case the scheduler takes
	// over).
	while(1) asm volatile ("hlt;");
}

void close_exec(exec_t* exec)
{
	file_close(exec->file);
	kfree(exec);
}

// Nothing fancy, just add the type to the list
void register_exec_type(exec_type_t* type)
{
	if(!type) return;
	type->next = g_exec_type;
	g_exec_type = type;
}

int sys_insmod(const char* filename)
{
	struct path path;
	int error = 0;
	module_t* module = NULL;
	
	// Lookup the path from the filename
	error = path_lookup(filename, WP_DEFAULT, &path);
	// Did the file exist?
	if( error != 0 ){
		return error;
	}
	
	// Open the file
	struct file* filp = file_open(&path, O_RDONLY);
	// We don't need the path reference anymore
	path_put(&path);
	
	// Could the file be opened?
	if( IS_ERR(filp) ){
		return PTR_ERR(filp);
	}
	
	exec_type_t* etype = g_exec_type;
	while( etype )
	{
		if( etype->load_module )
		{
			module = etype->load_module(filp);
			if( !IS_ERR(module) ){
				break;
			} else if( module != NULL ){ // it was the right type, something else went wrong
				return PTR_ERR(module);
			}
		}
		etype = etype->next;
	}
	
	if( module == NULL ){
		file_close(filp);
		return -ENOEXEC;
	}
	
	INIT_LIST(&module->m_link);
	list_add(&module->m_link, &g_module_list);
	
	if( module->m_load ){
		error = module->m_load(module);
		if( error != 0 ){
			list_rem(&module->m_link);
			kfree(module->m_loadaddr);
			return error;
		}
	}
	
	syslog(KERN_NOTIFY, "loaded %s at address 0x%x", filename, module->m_loadaddr);
	
	return 0;
}

int sys_rmmod(const char* name)
{
	list_t* item = NULL;
	list_for_each(item, &g_module_list)
	{
		module_t* module = list_entry(item, module_t, m_link);
		
		if( strcmp(module->m_name, name) == 0 ){
			if( module->m_refs != 0 ){
				return -EBUSY;
			}
			if( module->m_remove ){
				int error = module->m_remove(module);
				if( error != 0 ){
					return error;
				}
			}
			list_rem(&module->m_link);
			kfree(module->m_loadaddr);
			return 0;
		}
	}
	return -ENOENT;
}
