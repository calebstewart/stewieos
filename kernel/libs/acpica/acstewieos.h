/*
 * Acess-specific ACPI header.
 *
 * A modified version of the ForgeOS acforge.h, copied and modified with permision.
 */
#ifndef __ACSTEWIEOS_H__
#define __ACSTEWIEOS_H__

#include <kernel.h>
#include <stdint.h>
#include <stdarg.h>

// typedef unsigned char					BOOLEAN;
// typedef unsigned char					UINT8;
// typedef unsigned short					UINT16;
// typedef short							INT16;
// typedef unsigned long long				UINT64;
// typedef long long						INT64;

//#define ACPI_USE_SYSTEM_INTTYPES
#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_32BIT_PHYSICAL_ADDRESS

#define ACPI_USE_DO_WHILE_0
#define ACPI_MUTEX_TYPE             ACPI_BINARY_SEMAPHORE

#define ACPI_USE_NATIVE_DIVIDE

#define ACPI_CACHE_T                ACPI_MEMORY_LIST
#define ACPI_USE_LOCAL_CACHE        1

#define ACPI_MACHINE_WIDTH          32

#define ACPI_UINTPTR_T              uintptr_t

#define ACPI_FLUSH_CPU_CACHE() __asm__ __volatile__("wbinvd");

int acpi_init( void );

#include "acgcc.h"

#endif /* __ACSTEWIEOS_H__ */
