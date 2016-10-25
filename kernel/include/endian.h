#ifndef _STEWIEOS_ENDIAN_H_
#define _STEWIEOS_ENDIAN_H_

#include <endian.h>
#include <byteswap.h>

#define LITTLE_ENDIAN 1
#define BIG_ENDIAN 0
#define BYTE_ORDER LITTLE_ENDIAN

static uint16_t htobe16(uint16_t host_16bits) {
	return bswap_16(host_16bits);
}
static uint16_t htole16(uint16_t host_16bits){
	return host_16bits;
}
static uint16_t be16toh(uint16_t big_endian_16bits){
	return bswap_16(big_endian_16bits);
}
static uint16_t le16toh(uint16_t little_endian_16bits){
	return little_endian_16bits;
}

static uint32_t htobe32(uint32_t host_32bits){
	return bswap_32(host_32bits);
}
static uint32_t htole32(uint32_t host_32bits){
	return host_32bits;
}
static uint32_t be32toh(uint32_t big_endian_32bits){
       return bswap_32(big_endian_32bits);
}
static uint32_t le32toh(uint32_t little_endian_32bits){
       return little_endian_32bits;
}

static uint64_t htobe64(uint64_t host_64bits){
       return bswap_64(host_64bits);
}
static uint64_t htole64(uint64_t host_64bits){
       return host_64bits;
}
static uint64_t be64toh(uint64_t big_endian_64bits){
       return bswap_64(big_endian_64bits);
}
static uint64_t le64toh(uint64_t little_endian_64bits){
       return little_endian_64bits;
}

#endif