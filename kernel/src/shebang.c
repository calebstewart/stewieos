#include "shebang.h"
#include "fs.h"
#include "kmem.h"
#include <fcntl.h>
#include <sys/stat.h>
#include "error.h"
#include "unistd.h"

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
		return EXEC_INVALID;
	}
	
	char** argv = (char**)kmalloc(sizeof(char*)*(nargs+exec->argc+1));
	
	// Allocate space for the new arguments
	size_t i = 0;
	char* token = strtok(interp, " ");
	do
	{
		argv[i] = (char*)kmalloc(strlen(token)+1);
		strcpy(argv[i++], token);
	} while( (token = strtok(NULL, " ")) != NULL );
	
	argv[i] = (char*)kmalloc(strlen(filename)+1);
	strcpy(argv[i+1], filename);
	
	// copy all the other arguments (includign the null terminating one)
	for(; i <= (nargs+exec->argc); ++i){
		argv[i] = exec->argv[i-nargs+1];
	}
	
	// Free the old arguments array
	kfree(exec->argv[0]);
	kfree(exec->argv);
	
	// Save the new arguments
	exec->argv = argv;
	exec->argc = nargs + exec->argc - 1;
	
	file_close(exec->file);
	
	struct path path;
	int error = path_lookup(argv[0], WP_DEFAULT, &path);
	if( error != 0 ){
		return EXEC_INVALID;
	}
	
	exec->file = file_open(&path, O_RDONLY);
	path_put(&path);
	
	if( IS_ERR(exec->file) ){
		return EXEC_INVALID;
	}
	
	file_seek(exec->file, 0, SEEK_SET);
	file_read(exec->file, exec->buffer, 256);
	
	return EXEC_VALID;
}