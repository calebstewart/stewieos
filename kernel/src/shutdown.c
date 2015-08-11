#include "kernel.h"
#include "acpi/acpi.h"

shutdown_handler_t shutdown_handler[256] = {0};

int sys_shutdown( void )
{
	ACPI_STATUS status = AE_OK;
	
	for(int i = 0; i < 256; ++i){
		if( shutdown_handler[i] == NULL ) break;
		shutdown_handler[i]();
	}
	
	syslog(KERN_WARN, "shutdown: halting system...");
	
	// Interrupts must be on for EnterSleepStatePrep
	u32 flags = enablei();
	//status = AcpiEnterSleepStatePrep(ACPI_STATE_S5);
	
	if( status != AE_OK ){
		syslog(KERN_ERR, "shutdown: AcpiEnterSleepStatePrep(_S5_) failed with code %s.", AcpiFormatException(status));
		syslog(KERN_ERR, AcpiFormatException(status));
		restore(flags);
		return -1;
	}
	
	// Interrupts must be disabled for EnterSleepState
	disablei();
	status = AcpiEnterSleepState(ACPI_STATE_S5);
	
	if( status != AE_OK ){
		syslog(KERN_ERR, "shutdown: AcpiEnterSleepState(_S5_) failed with code %s.", AcpiFormatException(status));
		restore(flags);
		return -1;
	}
	
	restore(flags);
	
	// Execute the _S5_ state
	//AcpiEvaluateObject(NULL, (char*)"_S5_", NULL, NULL);
	
	// HALT!
	//asm volatile("cli;hlt");
	
	return 0;
}


void register_shutdown_handler(shutdown_handler_t handler)
{
	int i = 0;
	for(; i < 256; ++i){
		if( shutdown_handler[i] == NULL ){
			shutdown_handler[i] = handler;
			return;
		}
	}
}