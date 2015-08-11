#include "kernel.h"
#include "acpi/acpi.h"

/* OS Specific Initialization Routines */
ACPI_STATUS AcpiOsInitialize()
{
	
	
	return AE_OK;
}

/* OS Specific Termination Routines */
ACPI_STATUS AcpiOsTerminate()
{
	
	return AE_OK;
}

/* Find the RSDP Pointer */
ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer()
{
	ACPI_PHYSICAL_ADDRESS ret;
	AcpiFindRootPointer(&ret);
	return ret;
}

/* Override predefined objects in the namespace */
ACPI_STATUS AcpiOsPredefinedOverride(
						const ACPI_PREDEFINED_NAMES *PredefinedObject,
						ACPI_STRING *NewValue)
{
	*NewValue = NULL;
	return AE_OK;
}

/* Override predefined tables */
ACPI_STATUS AcpiOsTableOverride(
						ACPI_TABLE_HEADER *ExistingTable,
						ACPI_TABLE_HEADER **NewTable)
{
	*NewTable = NULL;
	return AE_OK;
}

/* Override the physical table */
ACPI_STATUS AcpiOsPhysicalTableOverride(
	ACPI_TABLE_HEADER *ExistingTable ATTR((unused)),
	ACPI_PHYSICAL_ADDRESS *NewAddress ATTR((unused)),
	UINT32 *NewTableLength)
{
	*NewAddress = 0;
	return AE_OK;
}

/* Validate an interface */
ACPI_STATUS AcpiOsValidateInterface(char* Interface ATTR((unused)));
ACPI_STATUS AcpiOsValidateInterface(char* Interface ATTR((unused)))
{
	return AE_SUPPORT;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void* Info)
{
	syslog(KERN_WARN, "acpi: %s: method not implemented!\n", __func__);
	return AE_OK;
}

/* Wait for all asynchronous events to complete */
void AcpiOsWaitEventsComplete( void )
{
	return;
}
