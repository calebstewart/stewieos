#include "kernel.h"
#include "acpi/acpi.h"

void AcpiOsRedirectOutput( void* Destination )
{
	syslog(KERN_ERR, "acpi: %s: feature not implemented!", __func__);
}

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(
	const char* Fmt,
	...)
{
#ifdef ACPI_PRINT_DEBUG
	va_list va;
	char buffer[512];
	
	va_start(va, Fmt);
	ee_vsprintf(buffer, Fmt, va);
	va_end(va);
	
	syslog_printf(buffer);
#endif	
}

void AcpiOsVprintf(
	const char* Fmt,
	va_list Args)
{
#ifdef ACPI_PRINT_DEBUG
	char buffer[512];
	ee_vsprintf(buffer, Fmt, Args);
	syslog_printf(buffer);
#endif
}

ACPI_STATUS AcpiOsGetLine(char* Buffer ATTR((unused)),
						  UINT32 BufferLength ATTR((unused)),
						  UINT32* BytesRead ATTR((unused)))
{
	syslog(KERN_ERR, "acpi: %s: feature not implemented!", __func__);
	return AE_OK;
}

ACPI_STATUS AcpiOsBreakpoint(char* Msg);
ACPI_STATUS AcpiOsBreakpoint(char* Msg)
{
	syslog(KERN_WARN, "acpi: %s", Msg);
	syslog(KERN_ERR, "acpi: debugging breakpoint not implemented.");
	return AE_OK;
}