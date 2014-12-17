#include <stdio.h>

void shutdown( void );

int main(int argc, char** argv)
{
	printf("WARNING: System is going down for a halt NOW!\n");
	shutdown();
	return 0;
}
