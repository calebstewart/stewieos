#include "kernel.h"
#include "descriptor_tables.h"
#include "acpi/acpi.h"

ACPI_OSD_HANDLER AcpiHandler[256] = {NULL};

void AcpiInterrupt(struct regs* regs, void* context);
void AcpiInterrupt(struct regs* regs, void* context)
{
	AcpiHandler[regs->intno](context);
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel,
										  ACPI_OSD_HANDLER Handler,
										  void *Context)
{
	AcpiHandler[InterruptLevel] = Handler;
	register_interrupt_context((u8)InterruptLevel, Context, AcpiInterrupt);
	return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber,
										 ACPI_OSD_HANDLER Handler ATTR((unused)))
{
	unregister_interrupt((u8)InterruptNumber);
	return AE_OK;
}

