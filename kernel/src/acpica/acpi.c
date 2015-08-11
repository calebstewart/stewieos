#include "kernel.h"
#include "acpi/acpi.h"
#include "error.h"

int acpi_init(void)
{
	ACPI_STATUS res = 0;
	
	syslog(KERN_NOTIFY, "acpi: initializing subsystem.");
	res = AcpiInitializeSubsystem();
	if( ACPI_FAILURE(res) ){
		syslog(KERN_ERR, "acpi: AcpiInitializeSubsytem failed with result %s.", AcpiFormatException(res));
		return -1;
	}
	
	syslog(KERN_NOTIFY, "acpi: initializing tables.");
	res = AcpiInitializeTables(NULL, 16, FALSE);
	if( ACPI_FAILURE(res) )
	{
		syslog(KERN_ERR, "acpi: AcpiInitializeTables failed with result %s.", AcpiFormatException(res));
		return -1;
	}
	
	syslog(KERN_NOTIFY, "acpi: loading tables.");
	res = AcpiLoadTables();
	if( ACPI_FAILURE(res) )
	{
		syslog(KERN_ERR, "acpi: AcpiLoadTables failed with result %s.", AcpiFormatException(res));
		return -1;
	}
	
	syslog(KERN_NOTIFY, "acpi: enabling subsystem.");
	res = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if( ACPI_FAILURE(res) )
	{
		syslog(KERN_ERR, "acpi: AcpiEnableSubsystem failed with result %s.", AcpiFormatException(res));
		return -1;
	}
	
	return 0;
}
