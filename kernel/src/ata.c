#include "stewieos/exec.h" // For the module definitions
#include "stewieos/kernel.h" // For printk
#include "stewieos/pci.h"
#include "stewieos/error.h"
#include "stewieos/ata.h"
#include "stewieos/timer.h"
#include "stewieos/fs.h"
#include "stewieos/block.h"

int ide_block_open(struct block_device* device, dev_t devid);
int ide_block_close(struct block_device* device, dev_t devid);
int ide_block_read(struct block_device* device, dev_t devid, off_t lba, size_t count, char* buffer);
int ide_block_write(struct block_device* device, dev_t devid, off_t lba, size_t count, const char* buffer);
int ide_block_exist(struct block_device* device, dev_t devid);

struct block_operations ide_block_operations = {
	.read = ide_block_read,
	.write = ide_block_write,
	.open = ide_block_open,
	.close = ide_block_close,
	.exist = ide_block_exist,
};

// Forward Declarations
static inline void ata_wait( u8 channel );
void ata_write_block(u8 channel, u8 reg, u8* buffer);

static const char* ATA_DEVICE_TYPE_NAME[2] = { "ATA", "ATAPI" };

static ide_channel_t ide_channel[2];
static ide_device_t ide_device[4];
static u8 ide_buffer[2048];

// This tells the operating system the module name, and the
// load and remove functions to be called at respective times.
//MODULE_INFO("ide", ide_load, ide_remove);

// This function will be called during module insertion
// The return value should be zero for success. All other
// values are passed back from insmod.
int ide_load( void )
{
	int result = ide_initialize();
	
	if( result < 0 ){
		return result;
	}
	
	int major = register_block_device(0x00, 0xff, &ide_block_operations);
	if( major < 0 ){
		syslog(KERN_ERR, "ide: unable to register block device: error %d\n", major);
	} else {
		syslog(KERN_NOTIFY, "ide: registered block device under major number %d.\n", major);
	}
	
	return 0;
}

int ide_block_exist(struct block_device* device, dev_t devid)
{
	UNUSED(device);
	UNUSED(devid);
	
	u8 min = (u8)minor(devid);
	u8 disk = (min & 0x0f);
	u8 part = (min & 0xf0) >> 4;

	if( disk >= 4 ){
		return 0;
	}
	
	if( part > 4 ){
		return 0;
	}
	
	if( ide_device[disk].exist == 0 ){
		return 0;
	}
	
	if( ide_device[disk].part[part].valid == 0 ){
		return 0;
	} 
	
	return 1;
}

int ide_block_open(struct block_device* device, dev_t devid)
{
	return ide_block_exist(device, devid) ? 0 : -ENXIO;
}

int ide_block_close(struct block_device* device, dev_t devid)
{
	UNUSED(device);
	UNUSED(devid);
	return 0;
}

int ide_block_read(struct block_device* device, dev_t devid, off_t lba, size_t count, char* buffer)
{
	UNUSED(device);
	u8 min = (u8)minor(devid);
	u8 d = (min & 0x0f);
	u8 p = ((min & 0xf0) >> 4);
	
	if( (lba+count) >= ide_device[d].part[p].lba_end ){
		return -EFAULT;
	}
	
	return ata_pio_transfer(ide_device[d].channel, ide_device[d].drive, ATA_PIO_READ, lba + ide_device[d].part[p].lba_start, (u8)count, buffer);
}

int ide_block_write(struct block_device* device, dev_t devid, off_t lba, size_t count, const char* buffer)
{
	UNUSED(device);
	u8 min = (u8)minor(devid);
	u8 d = (min & 0x0f);
	u8 p = ((min & 0xf0) >> 4);
	
	if( (lba+count) >= ide_device[d].part[p].lba_end ){
		return -EFAULT;
	}
	
	return ata_pio_transfer(ide_device[d].channel, ide_device[d].drive, ATA_PIO_WRITE, lba + ide_device[d].part[p].lba_start, (u8)count, (void*)buffer);
}

static inline void ata_wait( u8 channel )
{
	for(int i = 0; i < 4; ++i){
		ata_read_reg(channel, ATA_REG_ALTSTATUS);
		//inb(ide_channel[ATA_PRIMARY].ctrl);
	}
// 	u32 eflags = enablei();
// 	tick_t start = timer_get_ticks();
// 	while( (timer_get_ticks() - start) == 0 ){
// 		asm volatile ("hlt");
// 	}
// 	restore(eflags);
}

int ata_check_partition(u8* partition)
{
	for(int i = 0; i < 16; ++i){
		if( *partition != 0 ){
			return 1;
		}
	}
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
		syslog(KERN_PANIC, "ide: no ide controller found on PCI bus!\n");
		while(1);
		return -ENXIO;
	}
	
	
	pci_dword_t* bar = device->header.basic.bar;
	
	// Calculate I/O Ports reported by the BARs
	ide_channel[ATA_PRIMARY].base = (u16)((bar[0] & 0xFFFFFFFC) + 0x1F0 * (!bar[0]));
	ide_channel[ATA_PRIMARY].ctrl = (u16)((bar[1] & 0xFFFFFFFC) + 0x3F6 * (!bar[1]));
	ide_channel[ATA_PRIMARY].bmide = (u16)((bar[4] & 0xFFFFFFFC) + 0);
	ide_channel[ATA_SECONDARY].base = (u16)((bar[2] & 0xFFFFFFFC) + 0x170 * (!bar[2]));
	ide_channel[ATA_SECONDARY].ctrl = (u16)((bar[3] & 0xFFFFFFFC) + 0x376 * (!bar[3]));
	ide_channel[ATA_SECONDARY].bmide = (u16)((bar[4] & 0xFFFFFFFC) + 8);
	
	// Initialize the control register
	ata_write_reg(ATA_PRIMARY, ATA_REG_CONTROL, 0);
	ata_write_reg(ATA_SECONDARY, ATA_REG_CONTROL, 0);
	
	// Disable IRQs
	ata_write_reg(ATA_PRIMARY, ATA_REG_CONTROL, 2);
	ata_write_reg(ATA_SECONDARY, ATA_REG_CONTROL, 2);
	
	for(int i = 0; i < 4; ++i)
	{
		ide_device[i].exist = 0;
	}
	
	int ndevs = 0;
	
	// Check for the appearance of the four possible drives
	for(u8 channel = 0; channel < 2; ++channel)
	{
		for(u8 slave = 0; slave < 2; ++slave)
		{
			int type = ATA_TYPE_ATA;
			ide_device[ndevs].exist = 0;
			
			// Select the drive
			ata_select_drive(channel, slave);
			
			// Read the identification space into ide_buffer
			type = ata_identify_device(channel);
			
			// Identify told us that the device doesn't exist
			if( type == -1 ){
				continue;
			}
			
			// Read device parameters
			ide_device[ndevs].exist = 1;
			ide_device[ndevs].type = (unsigned char)type;
			ide_device[ndevs].channel = channel;
			ide_device[ndevs].drive = slave;
			ide_device[ndevs].sig = *((u16*)(ide_buffer + ATA_IDENT_DEVICETYPE));
			ide_device[ndevs].cap = *((u16*)(ide_buffer + ATA_IDENT_CAPABILITIES));
			ide_device[ndevs].command_set = *((u32*)(ide_buffer + ATA_IDENT_COMMANDSETS));
			
			// Get the size
			if( ide_device[ndevs].command_set & (1<<26) ){
				ide_device[ndevs].size = *((u32*)(ide_buffer + ATA_IDENT_MAX_LBA_EXT));
			} else {
				ide_device[ndevs].size = *((u32*)(ide_buffer + ATA_IDENT_MAX_LBA));
			}

			// Get the model string (for some reason the bytes are flipped)
			for(int i = 0; i < 39; i += 2){
				ide_device[ndevs].model[i] = ide_buffer[ATA_IDENT_MODEL + i + 1];
				ide_device[ndevs].model[i+1] = ide_buffer[ATA_IDENT_MODEL + i];
			}
			for(int i = 40; i > 0 && ide_device[ndevs].model[i] == ' '; --i){
				ide_device[ndevs].model[i] = 0;
			}
			ide_device[ndevs].model[41] = 0;
			
			// "partition 0" is the entire device
			ide_device[ndevs].part[0].valid = 1;
			ide_device[ndevs].part[0].lba_start = 0;
			ide_device[ndevs].part[0].lba_end = ide_device[ndevs].size;
			ide_device[ndevs].part[0].sysid = 0;
			
			printk("ide: found %s drive %dMB - %s\n", ATA_DEVICE_TYPE_NAME[ide_device[ndevs].type], ATA_SECTOR_TO_BYTE(ide_device[ndevs].size) / 1024 / 1024, ide_device[ndevs].model);
			
			for(int i = 1; i < 5; ++i){
				ide_device[ndevs].part[i].valid = 0;
			}
			
			if( ide_device[ndevs].type == ATA_TYPE_ATA )
			{
				int error = ata_pio_transfer(ide_device[ndevs].channel, ide_device[ndevs].drive, ATA_PIO_READ, 0, 1, ide_buffer);
				if( error != 0 ){
					syslog(KERN_ERR, "ide: unable to read MBR on device %d: %d", ndevs, error);
				} else {
					if( ide_buffer[0x1fe] != 0x55 || ide_buffer[0x1ff] != 0xAA ){
						printk("ide: device%d: bootsector signature is invalid. Assuming no partition table.\n", ndevs);
						for(int i = 0; i <= 4; ++i){
							ide_device[ndevs].part[i].valid = 0;
						}
					} else {
						printk("ide: device%d: bootsector signature validated.\n", ndevs);
						for(int p = 0; p < 4; ++p){
							struct _real_part {
								u8 unused0;
								u16 unused1;
								u8 sysid;
								u8 unused2;
								u16 unused3;
								u32 lba_start;
								u32 lba_end;
							}* part = (struct _real_part*)(&ide_buffer[0x1BE + p*16]);
							//printk("ide: device%d: partition%d: lba_start=0x%X, lba_end=0x%X, system-id=0x%X\n", ndevs, p+1, part->lba_start, part->lba_end, part->sysid);
							if( part->unused0 == 0 && part->unused1 == 0 && part->sysid == 0 \
								&& part->unused2 == 0 && part->unused3 == 0 && part->lba_start == 0 \
								&& part->lba_end == 0 ){
								ide_device[ndevs].part[p+1].valid = 0;
								continue;
							}
// 							if( !ata_check_partition(&ide_buffer[0x1BE + p*16]) ){
// 								ide_device[ndevs].part[p+1].valid = 0;
// 								continue;
// 							}
							ide_device[ndevs].part[p+1].valid = 1;
							ide_device[ndevs].part[p+1].lba_start = part->lba_start; //*((u32*)( &ide_buffer[0x1BE + p*16 + 8] ));
							ide_device[ndevs].part[p+1].lba_end = part->lba_start + part->lba_end; //ide_device[ndevs].part[p+1].lba_start + *((u32*)( &ide_buffer[0x1BE + p*16 + 12] ));
							ide_device[ndevs].part[p+1].sysid = part->sysid; //ide_buffer[0x1BE + p*16 + 4];
							printk("ide: device%d: partition%d: lba_start=0x%X, lba_end=0x%X, system-id=0x%X\n", ndevs, p+1, ide_device[ndevs].part[p+1].lba_start, ide_device[ndevs].part[p+1].lba_end, ide_device[ndevs].part[p+1].sysid);
						}	
					}
				}
			}
			
			ndevs++;
		}
	}
	
	return 0; 
}

// Select a drive on a specified channel
void ata_select_drive(u8 channel, int drive)
{
	ata_write_reg(channel, ATA_REG_HDDEVSEL, (u8)(0xA0 | ((drive & 0x1) << 4)) );
	ata_wait(channel);
}

int ata_identify_device(u8 channel)
{
	u8 status = 0, err = 0, type = ATA_TYPE_ATA;
	
	// Send the ATA Identify Command
	ata_write_reg(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	ata_wait(channel);
	
	// If status is zero, the device doesn't exist
	status = ata_read_reg(channel, ATA_REG_STATUS);
	if( status == 0 ) return -1;
	
	tick_t start = timer_get_ticks();
	
	// Wait for command completion
	// the first read of status has already occured above and was non-zero
	do{
		if( (timer_get_ticks() - start) > (5000) ){
			syslog(KERN_WARN, "ide: command timeout on channel %d!\n", channel);
			return -1;
		}
		if( (status & ATA_SR_ERR) ){  err = 1; break; }; // Device is not ATA
		if( !(status & ATA_SR_BSY) && (status & ATA_SR_DRQ) ) break; // everything is fine
		status = ata_read_reg(channel, ATA_REG_STATUS);
		ata_wait(channel);
	} while ( 1 );
	
	// Check for ATAPI Type devices
	if( err != 0 ){
		u8 cl = ata_read_reg(channel, ATA_REG_LBA1);
		u8 ch = ata_read_reg(channel, ATA_REG_LBA2);
		
		if( cl == 0x14 && ch == 0xEB ){
			type = ATA_TYPE_ATAPI;
		} else if( cl == 0x69 && ch == 0x96 ){
			type = ATA_TYPE_ATAPI;
		} else {
			return -1;
		}
		
		ata_write_reg(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
		ata_wait(channel);
	}
	
	// Read the identification space
	ata_read_block(channel, ATA_REG_DATA, ide_buffer);
	
	return type;
}

u8 ata_read_reg(u8 channel, u8 reg)
{
	u8 result;
	if( reg > 0x07 && reg < 0x0C){
		ata_write_reg(channel, ATA_REG_CONTROL, ATA_DEVICE_CONTROL(1, 0, ide_channel[channel].interrupt));
	}
	if( reg < 0x08 ){
		result = inb((u16)(ide_channel[channel].base + reg - 0x00));
	} else if( reg < 0x0C ){
		result = inb((u16)(ide_channel[channel].base + reg - 0x06));
	} else if( reg < 0x0E ){
		result = inb((u16)(ide_channel[channel].ctrl + reg - 0x0A));
	} else if( reg < 0x16 ){
		result = inb((u16)(ide_channel[channel].base + reg - 0x0E));
	}
	if( reg > 0x07 && reg < 0x0C ){
		ata_write_reg(channel, ATA_REG_CONTROL, ide_channel[channel].interrupt);
	}
	return result;
}

void ata_write_reg(u8 channel, u8 reg, u8 data)
{
	if( reg > 0x07 && reg < 0x0C){
		ata_write_reg(channel, ATA_REG_CONTROL, 0x80 | ide_channel[channel].interrupt);
	}
	if( reg < 0x08 ){
		outb((u16)(ide_channel[channel].base + reg - 0x00), data);
	} else if( reg < 0x0C ){
		outb((u16)(ide_channel[channel].base + reg - 0x06), data);
	} else if( reg < 0x0E ){
		outb((u16)(ide_channel[channel].ctrl + reg - 0x0A), data);
	} else if( reg < 0x16 ){
		outb((u16)(ide_channel[channel].base + reg - 0x0E), data);
	}
	if( reg > 0x07 && reg < 0x0C ){
		ata_write_reg(channel, ATA_REG_CONTROL, ide_channel[channel].interrupt);
	}
}

void ata_read_block(u8 channel, u8 reg, u8* buffer)
{
	if( reg > 0x07 && reg < 0x0C){
		ata_write_reg(channel, ATA_REG_CONTROL, 0x80 | ide_channel[channel].interrupt);
	}
	for(int i = 0; i < 128; ++i){
		if( reg < 0x08 ){
			((u32*)(buffer))[i] = inl((u16)(ide_channel[channel].base + reg - 0x00));
		} else if( reg < 0x0C ){
			((u32*)(buffer))[i] = inl((u16)(ide_channel[channel].base + reg - 0x06));
		} else if( reg < 0x0E ){
			((u32*)(buffer))[i] = inl((u16)(ide_channel[channel].ctrl + reg - 0x0A));
		} else if( reg < 0x16 ){
			((u32*)(buffer))[i] = inl((u16)(ide_channel[channel].base + reg - 0x0E));
		}
	}
	if( reg > 0x07 && reg < 0x0C ){
		ata_write_reg(channel, ATA_REG_CONTROL, ide_channel[channel].interrupt);
	}
}

void ata_write_block(u8 channel, u8 reg, u8* buffer)
{
	if( reg > 0x07 && reg < 0x0C){
		ata_write_reg(channel, ATA_REG_CONTROL, 0x80 | ide_channel[channel].interrupt);
	}
	for(int i = 0; i < 128; ++i){
		if( reg < 0x08 ){
			outl((u16)(ide_channel[channel].base + reg - 0x00), ((u32*)(buffer))[i]);
		} else if( reg < 0x0C ){
			outl((u16)(ide_channel[channel].base + reg - 0x06), ((u32*)(buffer))[i]);
		} else if( reg < 0x0E ){
			outl((u16)(ide_channel[channel].ctrl + reg - 0x0A), ((u32*)(buffer))[i]);
		} else if( reg < 0x16 ){
			outl((u16)(ide_channel[channel].base + reg - 0x0E), ((u32*)(buffer))[i]);
		}
	}
	if( reg > 0x07 && reg < 0x0C ){
		ata_write_reg(channel, ATA_REG_CONTROL, ide_channel[channel].interrupt);
	}
}

void ata_enable_irq(u8 channel)
{
	UNUSED(channel);
}

void ata_disable_irq(u8 channel)
{
	UNUSED(channel);
}

int ata_error_handler(u8 channel ATTR((unused)), int drive ATTR((unused)), u8 status ATTR((unused)))
{
	u8 error = ata_read_reg(channel, ATA_REG_ERROR);
	u8 altstatus = ata_read_reg(channel, ATA_REG_ALTSTATUS);
	syslog(KERN_WARN, "ide: read error on channel %d and drive %d: status=0x%X, error=0x%X, altstatus=0x%X", channel, drive, status, error, altstatus);
	return -ENXIO;
}

// Read data from the hard disk using PIO mode
int ata_pio_transfer(u8 channel, u8 drive, u8 direction, u32 lba, u8 count, void* location)
{
	u8 status = 0;
	
	if( direction != ATA_PIO_READ && direction != ATA_PIO_WRITE ){
		return -EINVAL;
	}
	
	// Largest 28-bit number
	if( lba > 268435455 ){
		return -E2BIG;
	}
	
	//ata_select_drive(channel, drive);
	u8 device_register = 0xA0; // some hosts do this, and the device ignores it... we'll go for it.
	device_register |= (1 << 6); // we want LBA mode
	device_register |= (u8)((drive & 0x1) << 4); // slave/master device
	device_register |= (u8)(lba >> 27); // the last 4 bits of the LBA, according to the read sector(s) command
	ata_write_reg(channel, ATA_REG_HDDEVSEL, device_register);
	ata_wait(channel);
	
	ata_write_reg(channel, ATA_REG_SECCOUNT0, count);
	ata_write_reg(channel, ATA_REG_LBA0, (u8)(lba & 0xFF));
	ata_write_reg(channel, ATA_REG_LBA1, (u8)( (lba >> 8) & 0xFF ));
	ata_write_reg(channel, ATA_REG_LBA2, (u8)( (lba >> 16) & 0xFF ));
	if( direction == ATA_PIO_READ ){
		ata_write_reg(channel, ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
	} else {
		ata_write_reg(channel, ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
	}
	ata_wait(channel);
	
	while( 1 )
	{
		// HPIOI1: Check_Status State (ATA v6 Spec. pg. 333)
		while(1)
		{
			ata_wait(channel);
			status = ata_read_reg(channel, ATA_REG_STATUS);
			// Transition HPIOI1:HIO
			if( !(status & ATA_SR_BSY) && !(status & ATA_SR_DRQ) ){
				if( direction == ATA_PIO_READ ){
					ata_error_handler(channel, (int)drive, status);
					return -ENXIO;
				} else {
					return 0;
				}
				//return ata_error_handler(channel, (int)drive, status);
			}
			
			// Transition HPIOI1:HPIOI1
			if( (status & ATA_SR_BSY) ) continue;
			// Transition HPIOI1:HPIOI2
			if( !(status & ATA_SR_BSY) && (status & ATA_SR_DRQ) ){
				break;
			}
		}
		
		// HPIOI2: Transfer_Data State (ATA v6 Spec. pg. 334)
		if( direction == ATA_PIO_READ ){
			ata_read_block(channel, ATA_REG_DATA, location);
		} else {
			ata_write_block(channel, ATA_REG_DATA, location);
		}
		count = (u8)(count - 1);
		location = (void*)( (char*)location + 512 );
		
		if( count == 0 && direction == ATA_PIO_READ ){
			break;
		}
		
		// Transition HPIOI2:HPIOI1
	}
	
	
	
	return 0;
}
