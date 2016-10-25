#ifndef _STEWIEOS_BYTESWAP_H_
#define _STEWIEOS_BYTESWAP_H_

#include <stdint.h>

static uint16_t bswap_16(uint16_t x)
{
	return ((x<<8)&0xff00) | ((x>>8)&0x00ff);
}

static uint32_t bswap_32(uint32_t x)
{
	return ((x<<24)&0xff000000) | \
			((x<<8)&0x00ff0000) | \
			((x>>8)&0x0000ff00) | \
			((x>>24)&0x000000ff);
}

static uint64_t bswap_64(uint64_t x)
{
	return ((x<<56)&0xff00000000000000LL) | \
			((x<<40)&0x00ff000000000000LL) | \
			((x<<24)&0x0000ff0000000000LL) | \
			((x<<8 )&0x000000ff00000000LL) | \
			((x>>56)&0x00000000000000ffLL) | \
			((x>>40)&0x000000000000ff00LL) | \
			((x>>24)&0x0000000000ff0000LL) | \
			((x>>8 )&0x00000000ff000000LL);
}

#endif
