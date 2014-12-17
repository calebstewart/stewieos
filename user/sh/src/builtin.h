#ifndef _SH_BUILTIN_H_
#define _SH_BUILTIN_H_

typedef int (*builtin_command_func_t)(int argc, char** argv);

typedef struct _command
{
	builtin_command_func_t func;
	const char* name;
} builtin_command_t;

builtin_command_func_t find_command(const char* name);

int builtin_chdir(int argc, char** argv);
int builtin_exit(int argc, char** argv);

#endif