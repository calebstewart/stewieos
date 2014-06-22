#include "exec.h" // For the module definitions
#include "kernel.h" // For printk
#include "pci.h"
#include "error.h"

// Forward Declarations
int ide_load(module_t* module);
int ide_remove(module_t* module);
int ide_initialize( void );

// This tells the operating system the module name, and the
// load and remove functions to be called at respective times.
MODULE_INFO("ide", ide_load, ide_remove);

// This function will be called during module insertion
// The return value should be zero for success. All other
// values are passed back from insmod.
int ide_load(module_t* module)
{
	UNUSED(module);
	return ide_initialize();
}

// This function will be called during module removal.
// It is only called when module->m_refs == 0, therefore
// you don't need to worry about reference counting. Just
// free any data you allocated, and return. Any non-zero
// return value will be passed back from rmmod
int ide_remove(module_t* module)
{
	UNUSED(module);
	printk("ide: module removed successfully.\n");
	return 0;
}

int ide_initialize( void )
{
	pci_device_t* device = NULL;
	pci_class_t ide_class = {
		.class = 0x01,
		.subclass = 0x01,
		.progif = 0xFF,
		.revid = 0xFF
	};
	
	// Search for ide device class
	if( pci_search(ide_class, &device, 1) == NULL ){
		printk("ide: no ide controller found on PCI bus!\n");
		return -ENXIO;
	}
	
	printk("ide: found ide device at address %02X:%02X:%02X\n", device->bus->id, device->dev_id, device->func_id);
	return 0;
}