#include "kernel.h"

shutdown_handler_t shutdown_handler[256] = {0};

int sys_shutdown( void )
{
	for(int i = 0; i < 256; ++i){
		if( shutdown_handler[i] == NULL ) break;
		shutdown_handler[i]();
	}
	// HALT!
	asm volatile("cli;hlt");
	
	return 0;
}


void register_shutdown_handler(shutdown_handler_t handler)
{
	int i = 0;
	for(; i < 256; ++i){
		if( shutdown_handler[i] == NULL ){
			shutdown_handler[i] = handler;
			return;
		}
	}
}