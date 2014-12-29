#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/message.h>

int main(int argc, char** argv)
{
	pid_t parent = getpid();
	pid_t child = 0;
	char c = 'H';
	message_t* message = message_alloc();
	
	
	child = fork();
	if( child == 0 )
	{
		printf("Hello, I am the child.\n I'm going to let the parent say hello.\n");
		message_send(parent, 0, &c, 1);
		message_pop(message, MESG_ANY, MESG_POP_WAIT);
		printf("Well, that was fun. Goodbye!\n");
		exit(0);
	} else {
		message_pop(message, MESG_ANY, MESG_POP_WAIT);
		printf("I am the parent. I guess I can speak now...\n");
		int status = 0;
		waitpid(child, &status, 0);
		printf("My kid is done, so I'm going to go.\n");
	}
	
	return 0;
}
