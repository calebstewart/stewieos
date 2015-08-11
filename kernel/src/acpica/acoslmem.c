#include "kernel.h"
#include "kmem.h"
#include "paging.h"
#include "task.h"
#include "acpi/acpi.h"

/* Map a physical address to a virtual one */
void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress,
					  ACPI_SIZE Length)
{
	// The first one megabyte is already mapped to 0xC0000000
	if( PhysicalAddress < 0x100000 && (PhysicalAddress+Length) < 0x100000 ){
		return (void*)(PhysicalAddress+0xC0000000);
	}
	
	// Local Variables
	u32 vaddr = 0xF0000000; // virtual address
	page_t* page = NULL; // page for the virtual address
	u32 physical_page ATTR((unused)) = PhysicalAddress & 0xFFFFF000; // physical page address
	u32 page_count = Length + (PhysicalAddress & 0xFFF); // page page
	
	// Round to a page boundary
	if( (page_count & 0xFFF) != 0 )
		page_count = (page_count & 0xFFFFF000) + 0x1000;
	// Calculate the number of pages required
	page_count /= 0x1000;
	
	// Look for an open virtual address
	for(; vaddr != 0; vaddr += 0x1000)
	{
		// Check for an empty page
		page = get_page((void*)vaddr, 1, current->t_dir);
		if( page == NULL ) continue;
		if( page->present == 1 ) continue;
		
		// Check for enough consecutive blocks!
		u32 count = 1;
		for(u32 incr = 0x1000;
			count < page_count && (vaddr+incr) != 0;
			count++, incr += 0x1000)
		{
			page = get_page((void*)(vaddr + incr), 1, current->t_dir);
			if( page == NULL || page->present == 1 ) break;
		}
		
		// we didn't find enough consecutive pages
		if( count != page_count ) continue;
		
		// We found an appropriate block! Allocate!
		break;
	}
	
	// No free memory!
	if( page == NULL || page->present == 1 ){
		syslog(KERN_ERR, "AcpiOsMapMemory: no more free virtual addresses!");
		return NULL;
	}
	
	// Map the pages
	for(u32 incr = 0; incr < (page_count*0x1000); incr += 0x1000)
	{
		map_page(current->t_dir, (void*)(vaddr+incr), PhysicalAddress+incr, 0, 1);
	}
	
	// return the virtual address
	return (void*)(vaddr + (PhysicalAddress & 0xFFF));
}

/* Unmap the virtual memory */
void AcpiOsUnmapMemory(void *Where, ACPI_SIZE Length)
{
	u32 page_count = Length + ((ACPI_PHYSICAL_ADDRESS)(Where) & 0xFFF); // page page
	
	// Round to a page boundary
	if( (page_count & 0xFFF) != 0 )
		page_count = (page_count & 0xFFFFF000) + 0x1000;
	// Calculate the number of pages required
	page_count /= 0x1000;
	
	for(u32 incr = 0; incr < (page_count*0x1000); incr += 0x1000)
	{
		unmap_page(current->t_dir, (void*)((u32)Where + incr));
	}
}

/* Get the physical address which the virtual one points to */
ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress,
									 ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
	u32 phys = 0;
	
	if( get_physical_addr(current->t_dir, LogicalAddress, &phys) != 0 ){
		*PhysicalAddress = 0;
		return AE_BAD_PARAMETER;
	}
	
	*PhysicalAddress = (ACPI_PHYSICAL_ADDRESS)phys;
	
	return AE_OK;
}

/* Allocate memory on the heap */
void* AcpiOsAllocate(ACPI_SIZE size)
{
	return kmalloc(size);
}

/* Free memory from the heap */
void AcpiOsFree(void* Memory)
{
	kfree(Memory);
}

/* Check if the address is readable */
BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length)
{
	for(ACPI_SIZE n = 0; n < Length; n += 0x1000){
		page_t* page = get_page((void*)((u32)Memory + n), 0, current->t_dir);
		if( page == NULL ) return FALSE;
		if( page->present == 0 ) return FALSE;
	}
	return TRUE;
}

/* Check if the address is writeable */
BOOLEAN AcpiOsWritable(void* Memory, ACPI_SIZE Length)
{
	for(ACPI_SIZE n = 0; n < Length; n += 0x1000){
		page_t* page = get_page((void*)((ACPI_SIZE)Memory + n),
								0,
								current->t_dir);
		if( page == NULL ) return FALSE;
		if( page->present == 0 ) return FALSE;
		if( page->rw == 0 ) return FALSE;
	}
	return TRUE;
}

/* Read physical memory */
ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address ATTR((unused)),
							 UINT64* Value ATTR((unused)),
							 UINT32 Width ATTR((unused)))
{
	*Value = 0;
	syslog(KERN_ERR, "acpi: %s: method not implemented!", __func__);
	return AE_OK;
}

/* Write physical memory */
ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address ATTR((unused)),
							 UINT64 Value ATTR((unused)),
							 UINT32 Width ATTR((unused)))
{
	syslog(KERN_ERR, "acpi: %s: method not implemented!", __func__);
	return AE_OK;
}