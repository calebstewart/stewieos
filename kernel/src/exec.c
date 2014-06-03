#include "kernel.h"
#include "exec.h"
#include "fs.h"
#include <fcntl.h>
#include <sys/stat.h>
#include "error.h"
#include "kmem.h"
#include "unistd.h"
#include "elf/elf32.h"

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
	
	
	return 0;
}

int load_exec(const char* filename, char** argv, char** envp)
{
	struct path path;
	exec_t* exec = NULL;
	int error = 0;
	
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
	
	// Allocate the executable structure
	exec = (exec_t*)kmalloc(sizeof(exec_t));
	memset(exec, 0, sizeof(exec_t));
	
	// Initialize the structure
	exec->file = filp;
	exec->argv = argv;
	exec->envp = envp;
	
	// Fill the read buffer
	file_read(filp, exec->buffer, 256);
	
	// Check all types. Load the executable using the first acceptable type.
	exec_type_t* type = g_exec_type;
	while( type != NULL )
	{
		// a type can implement load_exec, load_module, or both.
		if( type->load_exec )
		{
			// Try and load the executable
			// If the executable doesn't match the type, 0 is returned
			// otherwise an error is returned.
			error = type->load_exec(exec);
			if( error != 0 ){
				close_exec(exec);
				return error;
			}
		}
		type = type->next;
	}
	
	return ENOEXEC;
}

int load_module(const char* filename)
{
	struct path path;
	exec_t* exec = NULL;
	int error = 0;
	
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
	
	// Allocate the executable structure and clear it
	exec = (exec_t*)kmalloc(sizeof(exec_t));
	memset(exec, 0, sizeof(exec_t));
	
	// put the file pointer in the exec structure
	exec->file = filp;
	
	// Fill the read buffer
	file_read(filp, exec->buffer, 256);
	
	// Check all types. Load the executable using the first acceptable type.
	exec_type_t* type = g_exec_type;
	while( type != NULL )
	{
		// a type can implement load_exec, load_module, or both.
		if( type->load_module )
		{
			// Try and load the executable
			error = type->load_module(exec);
			// If it was successfull, we wouldn't return.
			// If it isn't the right format, we return 0
			// If it is the right format but there was an
			// 	error, we return an error value.
			if( error != 0 ){
				close_exec(exec);
				return error;
			}
		}
		type = type->next;
	}
	
	return ENOEXEC;
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
	
	// Grab the file_info (for size)
	struct stat file_info;
	file_stat(filp, &file_info);
	
	// Allocate space for the file data
	void* file_data = kmalloc(file_info.st_size);
	if( !file_data ){
		file_close(filp);
		return -ENOMEM;
	}
	
	// Read in the file data
	file_seek(filp, 0, SEEK_SET);
	file_read(filp, file_data, file_info.st_size);
	
	// Close the file
	file_close(filp);
	
	module_t* module = elf_init_module(file_data, file_info.st_size);
	
	if( IS_ERR(module) ){
		kfree(file_data);
		return PTR_ERR(module);
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