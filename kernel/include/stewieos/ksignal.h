#ifndef _KSIGNAL_H_
#define _KSIGNAL_H_

#include <stdint.h>
#include <signal.h>
#include <sys/signal.h>

#define SIGNAL_RAISED(task, sig)	( (task)->t_signal.bitmap[(int)(sig)/32] & (1 << ((sig) % 32)) )
#define RAISE_SIGNAL(task, sig) do{ (task)->t_signal.bitmap[(int)(sig)/32] |= (1 << ((sig) % 32)); (task)->t_signal.nraised++; } while(0)
#define LOWER_SIGNAL(task, sig) do{ (task)->t_signal.bitmap[(int)(sig)/32] &= ~(1 << ((sig) % 32)); (task)->t_signal.nraised--; } while(0)

struct task;
typedef void(*sighandler_t)(int);

typedef struct _signal_state
{
	uint32_t bitmap[(NSIG/32)+1];
	int nraised;
	sighandler_t handler[NSIG];
	void* handler_return;
	char fpu[512];
	u32 eflags;
	u32 esp;
	u32 ebp;
	u32 eip;
} signal_state_t;

void signal_init(struct task* task);
void signal_copy(struct task* src, struct task* dst);
int signal_kill(struct task* task, int signal);
void signal_save(struct task* task);
int signal_check(struct task* task);
void signal_set_return(struct task* task, void* userfunc);
void signal_return(struct task* task);
sighandler_t signal_signal(struct task* task, int sig, sighandler_t handler);

#endif 
