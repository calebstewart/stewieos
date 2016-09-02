#ifndef _STEWIEOS_WAIT_H_
#define _STEWIEOS_WAIT_H_

#include <unistd.h>

#define WNOHANG 0x00000001
#define WUNTRACED 0x00000002

#define WAIT_ANY ((pid_t)-1)

#define WIFEXITED(status)	(!WTERMSIG(status))
#define WEXITSTATUS(status)	(((status) >> 8) & 0xff)
#define WIFSIGNALED(status)	(!WIFSTOPPED(status) && !WIFEXITED(status))
#define WTERMSIG(status)	((status ) & 0x7f)
#define WIFSTOPPED(status)	(((status) & 0xff) == 0x7f)
#define WSTOPSIG(status)	WEXITSTATUS(status)

pid_t waitpid(pid_t pid, int* status, int options);
pid_t wait(int* status);

#endif