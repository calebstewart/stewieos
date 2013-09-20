#include "kernel.h"

size_t memset(void* ptr, size_t count, int v)
{
	size_t tmp = count;
	while(count--){
		*((char*)ptr) = (char)v;
		ptr = (void*)( (char*)ptr + 1 );
	}
	return tmp;
}

size_t memcpy(void* d, void* s, size_t c)
{
	size_t tmp = c;
	while(c--){
		*((char*)d) = *((char*)s);
		d = (void*)( (char*)d + 1 );
		s = (void*)( (char*)s + 1 );
	}
	return tmp;
}