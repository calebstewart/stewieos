#include "acpi/acpi.h"
#include "kernel.h"
#include "sem.h"
#include "kmem.h"

// /* Create a new Mutex (actually a semaphore) */
// ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle)
// {
// 	sem_t* sem = sem_alloc(1);
// 	*OutHandle = (ACPI_MUTEX)sem;
// 	
// 	if( sem == NULL ){
// 		return AE_NO_MEMORY;
// 	}
// 	return AE_OK;
// }
// 
// /* Delete a Mutex */
// void AcpiOsDeleteMutex(ACPI_MUTEX Handle)
// {
// 	sem_free((sem_t*)Handle);
// }
// 
// /* Lock the Mutex */
// ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout)
// {
// 	int result = sem_wait((sem_t*)Handle, Timeout == 0xFFFF ? 0xFFFFFFFF : Timeout);
// 	if( result == -ETIMEDOUT ){
// 		return AE_TIME;
// 	}
// 	
// 	return AE_OK;
// }
// 
// /* Release the mutex */
// void AcpiOsReleaseMutex(ACPI_MUTEX Handle)
// {
// 	sem_signal((sem_t*)Handle);
// }

/* Create a new semaphore */
ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits,
								  UINT32 InitialUnits,
								  ACPI_SEMAPHORE *OutHandle)
{
	sem_t* sem = sem_alloc(InitialUnits);
	*OutHandle = (ACPI_SEMAPHORE)sem;
	
	if( sem == NULL ){
		return AE_NO_MEMORY;
	}
	return AE_OK;
}

/* Free the semaphore */
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
	if( sem_free((sem_t*)Handle) != 0 ){
		return AE_ERROR;
	}
	return AE_OK;
}

/* Grab a unit from the semaphore */
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle,
								UINT32 Units ATTR((unused)),
								UINT16 Timeout)
{
	int result = sem_wait((sem_t*)Handle, Timeout == 0xFFFF ? 0xFFFFFFFF : Timeout);
	if( result != 0 ){
		return AE_TIME;
	}
	return AE_OK;
}

/* Release a unit from the semaphore */
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
	sem_signal((sem_t*)Handle);
	return AE_OK;
}

/* Create a spinlock */
ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
	spinlock_t* lock = (spinlock_t*)kmalloc(sizeof(spinlock_t));
	if( lock == NULL ){
		return AE_NO_MEMORY;
	}
	
	spin_init(lock);
	*OutHandle = (ACPI_SPINLOCK)lock;
	
	return AE_OK;
}

/* Delete a spinlock */
void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
	kfree((void*)Handle);
}

/* Lock the spinlock */
ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
	ACPI_CPU_FLAGS flags = (ACPI_CPU_FLAGS)disablei();
	spin_lock((spinlock_t*)Handle);
	return flags;
}

/* Unlock the spinlock */
void AcpiOsReleaseLock(ACPI_SPINLOCK Handle,
					   ACPI_CPU_FLAGS Flags)
{
	spin_unlock((spinlock_t*)Handle);
	restore(Flags);
}