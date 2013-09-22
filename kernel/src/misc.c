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

void cpuid(int code, u32* a, u32* d)
{
	asm volatile("cpuid":"=a"(*a),"=d"(*d):"a"((u32)code):"ecx","ebx");
}

u32 cpuid_string(int code, char* s)
{
	u32 max_code = 0;
	asm volatile("cpuid":"=a"(max_code), "=b"(*((u32*)(s+0))), "=d"(*((u32*)(s+4))), "=c"(*((u32*)(s+8))):"a"(code));
	return max_code;
}