#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>

builtin_command_t builtin_command[] = {
	{ .name = "cd",		.func = builtin_chdir	},
	{ .name = "chdir",	.func = builtin_chdir	},
	{ .name = "exit",	.func = builtin_exit	}
};

builtin_command_func_t find_command(const char* name)
{
	for(int i = 0; i < (sizeof(builtin_command)/sizeof(builtin_command[0])); ++i){
		if( strcmp(name, builtin_command[i].name) == 0 ){
			return builtin_command[i].func;
		}
	}
	return NULL;	
}

int builtin_chdir(int argc, char** argv )
{
	if( argc != 2 ){
		printf("%s: usage: %s [dir_name]\n", argv[0], argv[0]);
		return -1;
	}
	
	return chdir(argv[1]);
}

int builtin_exit(int argc, char** argv )
{
	exit(0);
	return 0;
}