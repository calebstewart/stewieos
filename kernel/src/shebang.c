#include "stewieos/shebang.h"
#include "stewieos/fs.h"
#include "stewieos/kmem.h"
#include <fcntl.h>
#include <sys/stat.h>
#include "stewieos/error.h"
#include "unistd.h"

exec_type_t shebang_script_type = {
	.name = "shebang",
	.descr = "Shebang Script - Handles scripts starting with #! (Shebang)",
	.load_exec = shebang_load,
	.check_exec = shebang_check,
};

int shebang_load(exec_t* exec)
{
	return 0;
}

int shebang_check(const char* filename, exec_t* exec)
{
	
	if( exec->buffer[0] != '#' || exec->buffer[1] != '!' ){
		return EXEC_INVALID;
	}
	
	char* interp = &exec->buffer[2];
	size_t nargs = 0;
	
 	exec->buffer[1] = ' '; // so we'll see the first parameter
	while( *interp == ' ' ) interp++;
	
	for(size_t i = 0; interp[i] != '\n' && interp[i] != '\0'; i++){
		// this is okay because &interp[0] == &exec->buffer[2]
		if( interp[i-1] == ' ' && interp[i] != ' '){
			nargs++;
		}
	}
	
	// they didn't give an interpreter
	if( nargs == 0 ){
		return -ENOENT;
	}
	
	char** argv = (char**)kmalloc(sizeof(char*)*(nargs+exec->argc+1));
	
	for(size_t i = 0; interp[i] != 0; ++i){
		if( interp[i] == '\n' ){
			interp[i] = 0;
			break;
		}
	}
	
	// Allocate space for the new arguments
	size_t i = 0;
	char* token = strtok(interp, " ");
	do
	{
		argv[i] = (char*)kmalloc(strlen(token)+1);
		strcpy(argv[i++], token);
	} while( (token = strtok(NULL, " ")) != NULL );
	
	argv[i] = (char*)kmalloc(strlen(filename)+1);
	strcpy(argv[i++], filename);
	
	// copy all the other arguments (includign the null terminating one)
	for(; i <= (nargs+exec->argc); ++i){
		argv[i] = exec->argv[i-nargs];
	}
	
	// Free the old arguments array
	kfree(exec->argv[0]);
	kfree(exec->argv);
	
	// Save the new arguments
	exec->argv = argv;
	exec->argc = nargs + exec->argc;
	
	struct path path;
	int error = path_lookup(argv[0], WP_DEFAULT, &path);
	if( error != 0 ){
		return -ENOENT;
	}
	
	struct file* old_file = exec->file; // save the old file (the script)
	exec->file = file_open(&path, O_RDONLY);
	path_put(&path);
	
	if( IS_ERR(exec->file) ){
		exec->file = old_file; // reset the old file
		return -ENOENT;
	}
	
	// close the script
	file_close(old_file);
	
	file_seek(exec->file, 0, SEEK_SET);
	file_read(exec->file, exec->buffer, 256);
	
	return EXEC_INTERP;
}