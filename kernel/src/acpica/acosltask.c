#include "acpi/acpi.h"
#include "task.h"
#include "error.h"

/* Returns the process id of the current process */
ACPI_THREAD_ID AcpiOsGetThreadId()
{
	return (ACPI_THREAD_ID)current->t_pid+1;
}

/* Create a worker thread for ACPICA */
ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type,
						  ACPI_OSD_EXEC_CALLBACK Function,
						  void *Context)
{
	pid_t pid = worker_spawn(Function, Context);
	if( IS_ERR(pid) ){
		if( pid == -ENOMEM ) return AE_NO_MEMORY;
		return AE_ERROR;
	}
	return AE_OK;
}
