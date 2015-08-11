#include "kernel.h"
#include "acpi/acpi.h"

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address,
						   UINT32* Value,
						   UINT32 Width)
{
	*Value = 0;
	switch(Width)
	{
		case 8:
			*((u8*)Value) = inb(Address);
			break;
		case 16:
			*((u16*)Value) = inw(Address);
			break;
		case 32:
			*((u32*)Value) = inl(Address);
			break;
		default:
			syslog(KERN_ERR, "acpi: %s: unsupported width %d!", __func__, Width);
	}
	return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address,
							UINT32 Value,
							UINT32 Width)
{
	switch(Width)
	{
		case 8:
			outb(Address, (u8)Value);
			break;
		case 16:
			outw(Address, (u16)Value);
			break;
		case 32:
			outl(Address, (u32)Value);
			break;
		default:
			syslog(KERN_ERR, "acpi: %s: unsupported width %d!", __func__, Width);
	}
	return AE_OK;
}