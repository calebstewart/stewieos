#include "kernel.h"
#include "pci.h"
#include "acpi/acpi.h"

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID* PciId,
									   UINT32 Register,
									   UINT64* Value,
									   UINT32 Width)
{
	switch(Width)
	{
		case 8:
			*((u8*)Value) = (u8)pci_config_read_word(PciId->Bus,
										PciId->Device,
										PciId->Function,
										Register);
			break;
		case 16:
			*((u16*)Value) = pci_config_read_word(PciId->Bus,
										PciId->Device,
										PciId->Function,
										Register);
			break;
		case 32:
			*((u32*)Value) = pci_config_read_dword(PciId->Bus,
										PciId->Device,
										PciId->Function,
										Register);
			break;
		case 64:
			((u32*)Value)[0] = pci_config_read_dword(PciId->Bus,
										PciId->Device,
										PciId->Function,
										Register);
			((u32*)Value)[1] = pci_config_read_dword(PciId->Bus,
										PciId->Device,
										PciId->Function,
										Register+4);
			break;
		default:
			syslog(KERN_ERR, "acpi: %s: unsupported width %d!", __func__, Width);
			return AE_ERROR;
	}
	
	return AE_OK;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID* PciId,
									   UINT32 Register,
									   ACPI_INTEGER Value,
									   UINT32 Width)
{
	syslog(KERN_ERR, "acpi: %s: method not implemented!", __func__);
	return AE_OK;
}

/* Derive PCI Id (we leave it the same) */
void AcpiOsDerivePciId(ACPI_HANDLE rhandle ATTR((unused)),
					   ACPI_HANDLE chandle ATTR((unused)),
					   ACPI_PCI_ID **PciId ATTR((unused)));
void AcpiOsDerivePciId(ACPI_HANDLE rhandle ATTR((unused)),
					   ACPI_HANDLE chandle ATTR((unused)),
					   ACPI_PCI_ID **PciId ATTR((unused)))
{
	syslog(KERN_WARN, "acpi: %s: method not implemented!\n", __func__);
}