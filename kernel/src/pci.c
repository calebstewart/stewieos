#include "stewieos/pci.h"
#include "stewieos/kmem.h"

// Bus table
pci_bus_t* g_pci_bus[255];

pci_word_t pci_config_read_word(pci_dword_t bus, pci_dword_t slot, pci_dword_t func, pci_dword_t offset)
{
	// This puts the address together to send to the PCI bus
	// we mask offset with 0xFC because that removes the first two
	// bits, and forces it to be dword aligned. We account for word
	// alignment later, and ignore byte alignment (auto-align to the
	// nearest word).
	u32 address = (offset & 0xFC)| 			// Register number/offset
			((func & 0x7) << 8) | 		// Function
			((slot & 0x1f) << 11) | 	// Device
			((bus & 0xFF) << 16) | 		// Bus
			0x80000000;			// Enable Bit
	
	outl(PCI_CONFIG_ADDRESS, address);

	return (pci_word_t)( (inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF );
}

pci_dword_t pci_config_read_dword(pci_dword_t bus, pci_dword_t slot, pci_dword_t func, pci_dword_t offset)
{
	// This puts the address together to send to the PCI bus
	// we mask offset with 0xFC because that removes the first two
	// bits, and forces it to be dword aligned. We truncate to the
	// nearest DWORD
	u32 address = (offset & 0xFC)| 			// Register number/offset
			((func & 0x7) << 8) | 		// Function
			((slot & 0x1f) << 11) | 	// Device
			((bus & 0xFF) << 16) | 		// Bus
			0x80000000;			// Enable Bit
	
	outl(PCI_CONFIG_ADDRESS, address);

	return (pci_dword_t)inl(PCI_CONFIG_DATA);
}

pci_word_t pci_get_vendor(pci_byte_t bus, pci_byte_t dev, pci_byte_t func)
{
	return pci_config_read_word(bus, dev, func, 0);
}

pci_class_t pci_get_class(pci_byte_t bus, pci_byte_t dev, pci_byte_t func)
{
	pci_dword_t class = pci_config_read_dword(bus, dev, func, 0x08);
	return *((pci_class_t*)(&class));
}

pci_byte_t pci_get_header_type(pci_byte_t bus, pci_byte_t dev, pci_byte_t func)
{
	pci_word_t word = pci_config_read_word(bus, dev, func, 0xE);
	return (pci_byte_t)(word & 0xFFFF);
}

pci_byte_t pci_get_secondary_bus(pci_byte_t bus, pci_byte_t dev, pci_byte_t func)
{
	union {
		pci_word_t bus_combined;
		struct{
			pci_byte_t primary;
			pci_byte_t secondary;
		} bus;
	} busses;
	
	busses.bus_combined = pci_config_read_word(bus, dev, func, 0x18);
	return busses.bus.secondary;
}

void pci_enumerate( void )
{
	if( !PCI_IS_MULTIFUNCTION(pci_get_header_type(0,0,0)) ) {
		pci_bus_t* bus = (pci_bus_t*)kmalloc(sizeof(pci_bus_t));
		if(!bus){
			printk("pci: error: unable to allocate pci child_bus structure: out of memory.\n\tpci bus 0x00 left unconfigured!\n");
			return;
		}
		// Set the child_bus number
		bus->id = 0;
		bus->bridge_bus = 0xFF;
		bus->bridge_dev = 0xFF;
		bus->bridge_func = 0xFF;
		INIT_LIST(&bus->devices);
		INIT_LIST(&bus->busses);
		INIT_LIST(&bus->bus_link);
		
		g_pci_bus[bus->id] = bus;
		
		pci_enumerate_bus(bus);
	} else {
		for(pci_byte_t func = 0; func < 8; ++func){
			if( pci_get_vendor(0, 0, func) != PCI_INVALID_WORD ){
				pci_bus_t* bus = (pci_bus_t*)kmalloc(sizeof(pci_bus_t));
				if(!bus){
					printk("pci: error: unable to allocate pci child_bus structure: out of memory.\n\tpci bus 0x00 left unconfigured!\n");
					return;
				}
				// Set the child_bus number
				bus->id = func;
				bus->bridge_bus = 0xFF;
				bus->bridge_dev = 0xFF;
				bus->bridge_func = 0xFF;
				INIT_LIST(&bus->devices);
				INIT_LIST(&bus->busses);
				INIT_LIST(&bus->bus_link);
				
				g_pci_bus[bus->id] = bus;
				pci_enumerate_bus(bus);
			}
		}
	}
}

void pci_enumerate_bus(pci_bus_t* bus)
{	
	for(pci_byte_t dev = 0; dev < 32; ++dev)
	{
		pci_enumerate_device(bus, dev);
	}
}

void pci_enumerate_device(pci_bus_t* bus, pci_byte_t dev)
{
	// Read the device vendor
	pci_word_t vendor = pci_get_vendor(bus->id, dev, 0);
	// Check for a missing device
	if( vendor == PCI_INVALID_WORD ){
		return;
	}
	
	// Enumerate the first device function ( we know that has to exist)
	pci_enumerate_function(bus, dev, 0);
	
	// Get this device's header type
	pci_byte_t header_type = pci_get_header_type(bus->id, dev, 0);
	
	// If this is a multifunction device, enumerate the other existing functions
	if( PCI_IS_MULTIFUNCTION(header_type) )
	{
		// Check every possible function
		for(pci_byte_t func = 1; func < 8; ++func)
		{
			if( pci_get_vendor(bus->id, dev, func) != PCI_INVALID_WORD )
			{
				pci_enumerate_function(bus, dev, func);
			}
		}
	}

}

void pci_enumerate_function(pci_bus_t* bus, pci_byte_t dev, pci_byte_t func)
{
	pci_class_t class = pci_get_class(bus->id, dev, func);
	

	pci_device_t* device = (pci_device_t*)kmalloc(sizeof(pci_device_t));
	
	device->bus = bus;
	device->dev_id = dev;
	device->func_id = func;
	INIT_LIST(&device->bus_link);
	
	list_add(&device->bus_link, &bus->devices);
	
	pci_dword_t* ptr = (pci_dword_t*)(&device->vendor);
	for(int i = 0, reg = 0; reg < 0x44; i++, reg += 4){
		ptr[i] = pci_config_read_dword(bus->id, dev, func, reg);
	}
	
	class = device->class;
	printk("pci: found device attached at %02X:%02X:%02X with base class 0x%02X and sub-class 0x%02X\n", bus->id, dev, func, class.class, class.subclass);
	
	// PCI-to-PCI Bridge Device
	if( class.class == 0x06 && class.subclass == 0x04 )
	{
		printk("pci: enumerating PCI-to-PCI bridge attached at %02X:%02X:%02X\n", bus->id, dev, func);
		// Allocate a new bus structure
		pci_bus_t* child_bus = (pci_bus_t*)kmalloc(sizeof(pci_bus_t));
		if( !child_bus ){
			printk("pci: error: unable to allocate pci child_bus structure: out of memory.\n\tpci bus 0x%02 left unconfigured!\n", (u32)pci_get_secondary_bus(bus->id, dev, func));
			return;
		}
		// Set the child_bus number
		child_bus->id = device->header.bridge.secondary_bus; //pci_get_secondary_bus(bus->id, dev, func);
		child_bus->bridge_bus = bus->id;
		child_bus->bridge_dev = dev;
		child_bus->bridge_func = func;
		INIT_LIST(&child_bus->devices);
		INIT_LIST(&child_bus->busses);
		INIT_LIST(&child_bus->bus_link);
		
		list_add(&child_bus->bus_link, &bus->busses);
		
		g_pci_bus[child_bus->id] = child_bus;
		
		pci_enumerate_bus(child_bus);
	}
}

pci_device_t* pci_get_device(pci_byte_t bus_id, pci_byte_t dev, pci_byte_t func)
{
	// Get the specified bus
	pci_bus_t* bus = g_pci_bus[bus_id];
	// Check if the bus exists
	if(!bus){
		return NULL;
	}
	
	// Search through every device for a matching device and function address
	// This is technically an address, so if it exists, there can only be one
	// matching device.
	list_t* iter = NULL;
	list_for_each(iter, &bus->devices){
		pci_device_t* device = list_entry(iter, pci_device_t, bus_link);
		if( device->dev_id == dev && device->func_id == func ){
			return device;
		}
	}
	
	return NULL;
}

/* function: pci_search
 * purpose: search a list of devices matching the specified vendor and device ids.
 * parameters:
 * 	pci_word_t vendor - the vendor of the device
 * 	pci_word_t deviceid - the device id (specific to the vendor)
 * 	pci_device_t** device - an array of device pointers to be returned
 * 	pci_dword_t length - the length of the array, and therefore the maximum number
 * 				of returned devices. This must be at least 1.
 * return value:
 * 	the array passed as 'device' or NULL on error. This function fails iff:
 * 		- Length < 1 Or,
 * 		- The specified vendor/device id is not attached to the system
 */
pci_device_t** pci_search(pci_class_t class, pci_device_t** array, pci_dword_t length)
{
	pci_dword_t arridx = 0;
	list_t* iter = NULL;
	
	// You requested zero items, we return zero items
	if( length == 0 ){
		return array;
	}
	
	// Search every bus for devices matching the given device class
	for(int b = 0; b < 255; ++b){
		if( g_pci_bus[b] )
		{
			list_for_each(iter, &g_pci_bus[b]->devices){
				pci_device_t* device = list_entry(iter, pci_device_t, bus_link);
				if( device->class.class == class.class && device->class.subclass )
				{
					if( class.progif == 0xFF ){
						array[arridx++] = device;
					} else if( device->class.progif == class.progif ){
						if( class.revid == 0xFF || device->class.revid == class.revid ){
							array[arridx++] = device;
						}
					}
					if( arridx == length ){
						return array;
					}
				}
			}
		}
	}
	
	// The specified device is not attached to the system
	if( arridx == 0 ){
		return NULL;
	}
	
	// Otherwise return the first item in the list
	return array;
}