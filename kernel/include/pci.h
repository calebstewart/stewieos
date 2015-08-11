#ifndef _PCI_H_
#define _PCI_H_

#include "kernel.h"
#include "linkedlist.h"

typedef u16 pci_word_t;
typedef u32 pci_dword_t;
typedef u8 pci_byte_t;

#define PCI_INVALID_WORD 0xFFFF
#define PCI_INVALID_DWORD 0xFFFFFFFF
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_IS_MULTIFUNCTION(ht)	(((ht) & 0x80) > 0)

typedef struct _pci_class
{
	pci_byte_t revid;
	pci_byte_t progif;
	pci_byte_t subclass;
	pci_byte_t class;
} __attribute__((packed)) pci_class_t;

struct _pci_address
{
	unsigned int zero : 2;
	unsigned int reg : 6;
	unsigned int func : 3;
	unsigned int dev : 5;
	unsigned int bus : 8;
	unsigned int reserved : 7;
	unsigned int enable : 1;
};
typedef struct _pci_address pci_addr_t;

/* type: pci_address_limit_pair_t
 * purpose:
 * 	This is simply a pair DWORDs containing an address and limit.
 */
typedef struct _pci_address_limit_pair
{
	pci_dword_t address;
	pci_dword_t limit;
} pci_address_limit_pair_t;


pci_dword_t pci_config_read_dword(pci_dword_t bus, pci_dword_t slot, pci_dword_t func, pci_dword_t offset);
pci_word_t pci_config_read_word(pci_dword_t bus, pci_dword_t slot, pci_dword_t func, pci_dword_t offset);

struct _pci_device;
typedef struct _pci_device pci_device_t;
struct _pci_bus;
typedef struct _pci_bus pci_bus_t;

/* type: pci_bridge_header_t
 * purpose:
 * 	Header (starting at offset 0x10) of a header_type == 0x02
 * 	This is a PCI-to-PCI bridge.
 */
struct _pci_bridge_header
{
	pci_dword_t bar[2];
	pci_byte_t primary_bus;
	pci_byte_t secondary_bus;
	pci_byte_t subordinate_bus;
	pci_byte_t secondary_latency_timer;
	pci_byte_t io_base;
	pci_byte_t io_limit;
	pci_word_t secondary_status;
	pci_word_t prefetch_memory_base;
	pci_word_t prefetch_memory_limit;
	pci_dword_t prefetch_base_upper;
	pci_dword_t prefetch_limit_upper;
	pci_word_t io_base_upper;
	pci_word_t io_limit_upper;
	pci_byte_t capability_pointer;
	pci_byte_t reserved0;
	pci_word_t reserved1;
	pci_dword_t expansion_rom_base;
	pci_byte_t interrupt_line;
	pci_byte_t interrupt_pin;
	pci_word_t bridge_control;
};
typedef struct _pci_bridge_header pci_bridge_header_t;

/* type: pci_cardbus_bridge_header_t
 * purpose:
 * 	Header (starting at offset 0x10) of a header_type == 0x01
 * 	This is a PCI-to-Cardbus bridge.
 */
struct _pci_cardbus_bridge_header
{
	pci_dword_t cardbus_base;
	pci_byte_t capability_offset;
	pci_byte_t reserved0;
	pci_word_t secondary_status;
	pci_byte_t pci_bus;
	pci_byte_t cardbus_bus;
	pci_byte_t subordinate_bus;
	pci_byte_t cardbus_latency_timer;
	pci_address_limit_pair_t memory[2];
	pci_address_limit_pair_t io[2];
	pci_byte_t interrupt_line;
	pci_byte_t interrupt_pin;
	pci_word_t bridge_control;
	pci_word_t subsystem_device_id;
	pci_word_t subsystem_vendor_id;
	pci_dword_t legacy_base_address;
};
typedef struct _pci_cardbus_bridge_header pci_cardbus_bridge_header_t;

/* type: pci_basic_header_t
 * purpose:
 * 	Header (starting at offset 0x10) of a header_type == 0x00
 * 	This is a basic PCI device (not a bridge).
 */
typedef struct _pci_basic_header
{
	pci_dword_t bar[6];
	pci_dword_t cardbus_cis_pointer;
	pci_word_t subsystem_vendor_id;
	pci_word_t subsystem_id;
	pci_dword_t expansion_base_address;
	pci_byte_t capability_pointer;
	pci_byte_t reserved0;
	pci_word_t reserved1;
	pci_dword_t reserved2;
	pci_byte_t interrupt_line;
	pci_byte_t interrupt_pin;
	pci_byte_t min_grant;
	pci_byte_t max_latency;
} pci_basic_header_t;

typedef union _pci_header
{
	pci_basic_header_t basic;
	pci_bridge_header_t bridge;
	pci_cardbus_bridge_header_t cardbus;
} pci_header_t;

struct _pci_device
{
	// PCI Address/Location
	pci_bus_t* bus;
	pci_byte_t dev_id;
	pci_byte_t func_id;
	
	// List links
	list_t bus_link;
	
	// Information common to all header types
	pci_word_t vendor;
	pci_word_t device;
	pci_word_t command;
	pci_word_t status;
	pci_class_t class;
	pci_byte_t cache_line_size;
	pci_byte_t latency_timer;
	pci_byte_t header_type;
	pci_byte_t bist;
	
	pci_header_t header;

} __attribute__((packed)); 

struct _pci_bus
{
	// Bus information
	pci_byte_t id;		// bus id (i.e. 0-7, the bus number)
	
	// If this bus is a PCI-to-PCI bridge, these point to the address of that bridge
	pci_byte_t bridge_bus;	// Bus number
	pci_byte_t bridge_dev;	// Device number
	pci_byte_t bridge_func;	// Function number
	
	// Lists
	list_t devices;		// List of devices on this bus
	list_t busses;		// List of busses attached to this bus
	list_t bus_link;	// Link in the parent bus busses list
};

// Enumerate the entire PCI bus recursively
void pci_enumerate(void);
// Enumerate a single PCI bus number
void pci_enumerate_bus(pci_bus_t* bus);
// Enumerate a single PCI Device on a specified bus
void pci_enumerate_device(pci_bus_t* bus, pci_byte_t device);
// Enumerate a single function of a specified device
void pci_enumerate_function(pci_bus_t* bus, pci_byte_t device, pci_byte_t function);

pci_word_t pci_get_vendor(pci_byte_t bus, pci_byte_t dev, pci_byte_t func);
pci_class_t pci_get_class(pci_byte_t bus, pci_byte_t dev, pci_byte_t func);
pci_byte_t pci_get_header_type(pci_byte_t bus, pci_byte_t dev, pci_byte_t func);
pci_byte_t pci_get_secondary_bus(pci_byte_t bus, pci_byte_t dev, pci_byte_t func);

pci_device_t* pci_get_device(pci_byte_t bus, pci_byte_t dev, pci_byte_t func);
pci_device_t** pci_search(pci_class_t class, pci_device_t** buffer, pci_dword_t buffer_len);


#endif