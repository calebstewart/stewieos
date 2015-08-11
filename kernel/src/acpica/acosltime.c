#include "kernel.h"
#include "task.h"
#include "acpi/acpi.h"
#include "timer.h"

/* Sleep the current task for the given milliseconds */
void AcpiOsSleep(UINT64 Milliseconds)
{
	task_sleep(current, (u32)Milliseconds);
}

/* Stall the task for the given MICROseconds */
void AcpiOsStall(UINT32 Microseconds)
{
	tick_t ticks = Microseconds/1000;
	tick_t finish = timer_get_ticks() + ticks;
	
	while( timer_get_ticks() < finish ){
		ticks = 0;
	}
}

/* Get the current timer ticks */
UINT64 AcpiOsGetTimer( void )
{
	return (UINT64)(timer_get_ticks() * 10);
}