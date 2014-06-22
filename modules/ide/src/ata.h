#ifndef _ATA_H_
#define _ATA_H_


// Command/Status Port Flags
#define ATA_SR_BSY	0x80
#define ATA_SR_DRDY	0x40
#define ATA_SR_DF	0x20
#define ATA_SR_DSC	0x10
#define ATA_SR_DRQ	0x08
#define ATA_SR_CORR	0x04
#define ATA_SR_IDX	0x02
#define ATA_SR_ERR	0x01

// Features/Error Port Flags
#define ATA_ER_BBK	0x80
#define ATA_ER_UNC	0x40
#define ATA_ER_MC	0x20
#define ATA_ER_IDNF	0x10
#define ATA_ER_MCR	0x08
#define ATA_ER_ABRT	0x04
#define ATA_ER_TKONF	0x02
#define ATA_ER_AMNF	0x01

// ATA Command List
#define ATA_CMD_READ_PIO	0x20
#define ATA_CMD_READ_PIO_EXT	0x24
#define ATA_CMD_READ_DMA	0xC8
#define ATA_CMD_READ_DMA_EXT	0x25
#define ATA_CMD_WRITE_PIO	0x30
#define ATA_CMD_WRITE_PIO_EXT	0x34
#define ATA_CMD_WRITE_DMA	0xCA
#define ATA_CMD_WRITE_DMA_EXT	0x35
#define ATA_CMD_CACHE_FLUSH	0xE7
#define ATA_CMD_CACHE_FLUSH_EXT	0xEA
#define ATA_CMD_PACKET		0xA0
#define ATA_CMD_IDENTIFY_PACKET	0xA1
#define ATA_CMD_IDENTIFY	0xEC
// ATAPI Device Commands
#define ATAPI_CMD_READ		0xA8
#define ATAPI_CMD_EJECT		0x1B

// ATA Identification Space 
#define ATA_IDENT_DEVICETYPE	0
#define ATA_IDENT_CYLINDERS	2
#define ATA_IDENT_HEADS		6
#define ATA_IDENT_SECTORS	12
#define ATA_IDENT_SERIAL	20
#define ATA_IDENT_MODEL		54
#define ATA_IDENT_CAPABILITIES	98
#define ATA_IDENT_FIELDVALID	106
#define ATA_IDENT_MAX_LBA	120
#define ATA_IDENT_COMMANDSETS	164
#define ATA_IDENT_MAX_LBA_EXT	200

// Interface types
#define IDE_ATA			0x00
#define IDE_ATAPI		0x01

// Master/Slave
#define ATA_MASTER		0x00
#define ATA_SLAVE		0x01

// ATA Channel Registers
#define ATA_REG_DATA		0x00
#define ATA_REG_ERROR		0x01
#define ATA_REG_FEATURES	0x01
#define ATA_REG_SECCOUNT0	0x02
#define ATA_REG_LBA0		0x03
#define ATA_REG_LBA1		0x04
#define ATA_REG_LBA2		0x05
#define ATA_REG_HDDEVSEL	0x06
#define ATA_REG_COMMAND		0x07
#define ATA_REG_STATUS		0x07
#define ATA_REG_SECCOUNT1	0x08
#define ATA_REG_LBA3		0x09
#define ATA_REG_LBA4		0x0A
#define ATA_REG_LBA5		0x0B
#define ATA_REG_CONTROL		0x0C
#define ATA_REG_ALTSTATUS	0x0C
#define ATA_REG_DEVADDRESS	0x0D

// Wait for 400 nanoseconds
#define IDE_WAIT_400NS(channel)	for(int i = 0; i < 4; ++i){ ide_read(channel, ATA_REG_ALTSTATUS); } 

// Results of ide_polling
#define IDE_POLLING_ERROR	1
#define IDE_POLLING_FAULT	2
#define IDE_POLLING_DRQ		3

// Defines IDE ATA Channel Structure
typedef struct _ide_channel
{
	u16 base; // I/O Base Address
	u16 ctrl; // I/O Control Base Address
	u16 bmide; // Bus Master IDE
	u8  interrupt; // The interrupt vector for this channel
} ide_channel_t;

// Defines IDE ATA Device structure
typedef struct _ide_device
{
	u8 exist;			// Whether or not there is a device here
	u8 channel;			// Primary (0) or Secondary (1)
	u8 drive;			// ATA_MASTER/ATA_SLAVE
	u16 type;			// IDE_ATA/IDE_ATAPI
	u16 sig;			// Drive signature
	u16 cap;			// Features
	u32 command_set;		// Supported Command Sets
	u32 size;			// Size in sectors
	unsigned char model[41];	// Model String
} ide_device_t;

// Read an IDE register
u8 ide_read(u8 channel, u8 reg);
// Write an IDE register
void ide_write(u8 channel, u8 reg, u8 data);
// Read the identification space (128 dwords)
void ide_read_ident(u8 channel, u8 reg, u8* buffer);
// Wait for data to be ready and optionally return an error
u8 ide_polling(u8 channel, u32 check_error);
// Print the error information from a read/write attempt
void ide_print_error(u32 drive, u8 error);
// Initialize the IDE Controller
int ide_initialize( void );


#endif