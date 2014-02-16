#ifndef _BLOCK_H_
#define _BLOCK_H_

#define BLOCK_SIZE 512

#define ASSIGN_MAJOR 256

struct block_device;
struct file_operations;

extern struct file_operations block_device_fops;

/* NOTE
 * 	I'm not sure if I should just pass the minor number
 * 	as an integer after the block device structure or 
 * 	if I should pass an inode structure. I don't think
 * 	the block devices will be used without an inode but
 * 	i'm not sure...
 * 
 * 	I'm leaning toward instead passing the full device
 * 	id after the block device structure. At least keeping
 * 	the original data intact that much...
 */
struct block_operations
{
	int(*read)(struct block_device*,dev_t,char*,off_t);
	int(*write)(struct block_device*,dev_t,const char*, off_t);
	int(*ioctl)(struct block_device*,dev_t,int,char*);
	int(*open)(struct block_device*, dev_t);
	int(*close)(struct block_device*, dev_t);
	
	int(*attach)(struct block_device*);
	int(*detach)(struct block_device*);
};

struct block_device
{
	struct block_operations* ops; 	// block device operations
	size_t blksz;			// block size
	unsigned int nminors;		// number of allowed minor devices
	void* priv;			// private driver data
	u32 refs;
};

/* function: register_major_device
 * purpose:
 * 	register a new major device number,
 * 	if major<0, then a free major device
 * 	is chosen. The major device number is
 * 	always returned.
 */
int register_major_device(unsigned int major, unsigned int nminors, struct block_operations* block_ops);
/* function: unregister_major_device
 * purpose:
 * 	removes the major device from memory.
 */
int unregister_major_device(unsigned int major);
/* function: get_block_device
 * purpose
 * 	retrieve the block device structure
 * 	from a device id type. If minor(device)
 * 	is greater than nminors, then NULL
 * 	is returned.
 */
struct block_device* get_block_device(dev_t device);

int block_open(dev_t device);
int block_close(dev_t device);
int block_read(dev_t device, char* buffer, off_t lba_addr);
int block_write(dev_t device, const char* buffer, off_t lba_addr);
int block_ioctl(dev_t device, int cmd, char* argp);


#endif