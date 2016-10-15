#include <stdio.h>
#include <signal.h>

void signal_handler(int sig)
{
	printf("CAUGHT SIGNAL: %s (%d)\n", strsignal(sig), sig);
	//signal_return();
	//while(1);
}

int main(int argc, char** argv)
{
	int sig = SIGUSR1;
	signal(sig, signal_handler);
	printf("Raising %s (%d)...\n", strsignal(sig), sig);
	//kill(getpid(), SIGUSR1);
	raise(sig);
	printf("Finished raising signal. Exiting normally.\n");
	return 0;
}
