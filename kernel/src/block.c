#include "kernel.h"
#include "block.h"
#include "fs.h"
#include "dentry.h"
#include "kmem.h"
#include "sys/types.h"
#include <errno.h>
#include "spinlock.h"

typedef struct {
	char*  block;
	int dirty;
	off_t dirty_block;
	spinlock_t lock;
} block_data_t;

#define GET_BLOCK_DATA(file) ((block_data_t*)((file)->f_private))

// Private declarations
static int block_attach(dev_t device);
static int block_detach(dev_t device);
static int block_file_open(struct file*,struct dentry*, int);
static int block_file_close(struct file*,struct dentry*);
static int block_file_ioctl(struct file*, int, char*);
static int block_file_fstat(struct file*, struct stat*);
static ssize_t block_file_read(struct file*, char*, size_t);
static ssize_t block_file_write(struct file*, const char*, size_t);

struct block_device* vfs_dev[256];
struct file_operations block_device_fops = {
	.open = block_file_open,
	.close = block_file_close,
	.read = block_file_read,
	.write = block_file_write,
	.ioctl = block_file_ioctl,
	.fstat = block_file_fstat
};

int register_block_device(unsigned int major, unsigned int nminors, struct block_operations* block_ops)
{
	
	if( major >= 256 )
	{
		for(major = 0; major < 256; ++major)
		{
			if( vfs_dev[major] == NULL ){
				break;
			}
		}
	}
	if( major >= 256 ) return -EINVAL;
	if( vfs_dev[major] != NULL ) return -EBUSY;
	
	struct block_device* dev = (struct block_device*)kmalloc(sizeof(struct block_device));
	if(!dev){
		return -ENOMEM;
	}
	memset(dev, 0, sizeof(struct block_device));
	
	dev->ops = block_ops;
	dev->nminors = nminors;
	dev->blksz = 512;
	dev->block = kmalloc(dev->blksz);
	spin_init(&dev->lock);
	
	vfs_dev[major] = dev;
	
	int result = block_attach(makedev(major,0));
	if( result != 0 ){
		vfs_dev[major] = NULL;
		kfree(dev);
		return result;
	}
	
	return 0;
}

int unregister_block_device(unsigned int major)
{
	if( major >= 256 || vfs_dev[major] == NULL ){
		return 0;
	}
	if( vfs_dev[major]->refs != 0 ){
		return -EBUSY;
	}
	
	struct block_device* dev = vfs_dev[major];
	vfs_dev[major] = NULL;
	
	int result = block_detach(makedev(major,0));
	if( result != 0 ){
		vfs_dev[major] = dev;
		return result;
	}
	
	kfree(dev);
	return 0;
}

struct block_device* get_block_device(dev_t devid)
{
	struct block_device* device = vfs_dev[major(devid)];
	if( !device ){
		return NULL;
	}
	
	if( minor(devid) >= device->nminors ){
		return NULL;
	}
	
	if( device->ops->exist ){
		if( !device->ops->exist(device, devid) ){
			return NULL;
		}
	}
	
	return device;
}

int block_attach(dev_t devid)
{
	struct block_device* device = get_block_device(devid);
	if( !device ) return -ENODEV;
	if( device->ops->attach ){
		int result = device->ops->attach(device);
		return result;
	}
	return 0;
}

int block_detach(dev_t devid)
{
	struct block_device* device = get_block_device(devid);
	if( !device ) return -1;
	if( device->ops->detach ){
		int result = device->ops->detach(device);
		return result;
	}
	return 0;
}

ssize_t block_read(dev_t devid, off_t off, size_t count, char* buffer)
{
	struct block_device* device = get_block_device(devid);
	int error = 0;
	off_t lba, end_lba;
	size_t requested_count = count; // count is decremented throughout the process
	
	if( !device ) return -ENODEV;
	
	if( !device->ops->read ){
		return -ENOSYS;
	}
	
	// Aqcuire the data lock
	spin_lock(&device->lock);
	
	// calculate the LBA values
	lba = (off_t)(off / device->blksz);
	end_lba = lba + (count / device->blksz);
	if( (count % device->blksz) != 0 ){
		end_lba++;
	}
	
	if( (off % device->blksz) != 0 )
	{
		error = device->ops->read(device, devid, lba, 1, device->block);
		if( error != 0 ){
			spin_unlock(&device->lock);
			return (ssize_t)error;
		}
		if( count > (device->blksz - (off % device->blksz)) )
		{
			memcpy(buffer, &device->block[off%device->blksz], device->blksz - (off%device->blksz));
			buffer = buffer + (device->blksz - (off%device->blksz));
			count -= (device->blksz - (off%device->blksz));
		} else {
			memcpy(buffer, &device->block[off%device->blksz], count);
			spin_unlock(&device->lock);
			return (ssize_t)count;
		}
		lba++;
		off = lba*device->blksz;
	}
	
	if( count >= device->blksz )
	{
		error = device->ops->read(device, devid, lba, count / device->blksz, buffer);
		if( error != 0 ){
			spin_unlock(&device->lock);
			return (ssize_t)error;
		}
		lba += count / device->blksz;
		buffer = buffer + ((size_t)(count / device->blksz))*device->blksz;
		count -= ((size_t)(count / device->blksz))*device->blksz;
	}
	
	if( count != 0 )
	{
		error = device->ops->read(device, devid, lba, 1, device->block);
		if( error != 0 ){
			spin_unlock(&device->lock);
			return (ssize_t)error;
		}
		memcpy(buffer, device->block, count);
	}
	
	spin_unlock(&device->lock);
	
	return (ssize_t)requested_count;
}

int block_write(dev_t devid, off_t off, size_t count, const char* buffer)
{
	struct block_device* device = get_block_device(devid);
	int error = 0;
	off_t lba, end_lba;
	size_t requested_count = count; // count is decremented throughout the process
	
	if( !device ) return -ENODEV;
	
	if( !device->ops->write ){
		return -ENOSYS;
	}
	
	// Aqcuire the data lock
	spin_lock(&device->lock);
	
	if( count == 0 ){
		spin_unlock(&device->lock);
		return 0;
	}
	
	lba = (off_t)(off / device->blksz);
	end_lba = lba + (count / device->blksz);
	if( (count % device->blksz) != 0 ){
		end_lba++;
	}
	
	// We need to modify a piece of a block
	if( (off % device->blksz) != 0 )
	{
		// read in the entire block
		error = device->ops->read(device, devid, lba, 1, device->block);
		if( error != 0 ){
			spin_unlock(&device->lock);
			return (ssize_t)error;
		}
		// Copy the data into the block
		if( count > (device->blksz - (off % device->blksz)) )
		{
			// we are writing more than just this block
			memcpy(&device->block[off%device->blksz], buffer, device->blksz - (off%device->blksz));
			buffer = buffer + (device->blksz - (off%device->blksz));
			count -= (device->blksz - (off%device->blksz));
		} else {
			// we are only writing within this block
			memcpy(&device->block[off%device->blksz], buffer, count);
			count = 0;
		}
		// write the whole back to disk
		error = device->ops->write(device, devid, lba, 1, device->block);
		if( error != 0 ){
			spin_unlock(&device->lock);
			return (ssize_t)error;
		}
		lba++;
		off = lba*device->blksz;
	}
	
	// do we have block aligned writes to do? (that's prefered...)
	if( count >= device->blksz )
	{
		error = device->ops->write(device, devid, lba, count / device->blksz, buffer);
		if( error != 0 ){
			spin_unlock(&device->lock);
			return (ssize_t)error;
		}
		lba += count / device->blksz;
		buffer = buffer + ((size_t)(count / device->blksz))*device->blksz;
		count -= ((size_t)(count / device->blksz))*device->blksz;
	}
	
	if( count != 0 )
	{
		error = device->ops->read(device, devid, lba, 1, device->block);
		if( error != 0 ){
			spin_unlock(&device->lock);
			return (ssize_t)error;
		}
		memcpy(device->block, buffer, count);
		error = device->ops->write(device, devid, lba, 1, device->block);
		if( error != 0 ){
			spin_unlock(&device->lock);
			return (ssize_t)error;
		}
	}
	
	spin_unlock(&device->lock);
	
	return (ssize_t)requested_count;
}


int block_ioctl(dev_t devid, int cmd, char* argp)
{
	struct block_device* device = get_block_device(devid);
	if( !device ) return -1;
	if( device->ops->ioctl ){
		int result = device->ops->ioctl(device, devid, cmd, argp);
		return result;
	}
	return -1;
}

int block_open(dev_t devid)
{
	struct block_device* device = get_block_device(devid);
	if(!device) return -ENODEV;
	if( device->ops->open ){
		return device->ops->open(device, devid);
	}
	return 0;
}

int block_close(dev_t devid)
{
	struct block_device* device = get_block_device(devid);
	if(!device) return -ENODEV;
	if( device->ops->close ){
		return device->ops->close(device, devid);
	}
	return 0;
}

int block_file_ioctl(struct file* file, int cmd, char* parm)
{
	return block_ioctl(file->f_path.p_dentry->d_inode->i_dev, cmd, parm);	
}
int block_file_fstat(struct file* file, struct stat* stat)
{
	memset(stat, 0, sizeof(struct stat));
	UNUSED(file);
	return 0;
}

int block_file_open(struct file* file, struct dentry* dentry, int omode)
{
	UNUSED(omode);
	UNUSED(file);
	
	int result = block_open(dentry->d_inode->i_dev);
	if( result != 0 ){
		return result;
	}
	
	return 0;
}

int block_file_close(struct file* file, struct dentry* dentry)
{
	UNUSED(dentry);
	return block_close(file->f_path.p_dentry->d_inode->i_dev);
}

/* function: block_file_read
 * purpose: read a block device as a file on the byte level
 * 		This can be tricky due to block alignments and
 * 		funny shifts you have to do. The function reads
 * 		from file->f_off and modifies it directly.
 * parameters:
 * 	file - the file we are reading from
 * 	buffer - the buffer to read into
 * 	count - the number of bytes to read
 * returns:
 * 	The number of bytes successfully read or an error value.
 */
ssize_t block_file_read(struct file* file, char* buffer, size_t count)
{
	dev_t devid = file->f_path.p_dentry->d_inode->i_dev;
	ssize_t result = block_read(devid, file->f_off, count, buffer);
	
	if( result > 0 ){
		file->f_off += result;
	}
	
	return result;
}

/* function: block_file_write
 * purpose: write to a block device as a file on the byte level
 * 		This can be tricky due to block alignments and
 * 		funny shifts you have to do. The function writes
 * 		from file->f_off and modifies it directly.
 * parameters:
 * 	file - the file we are writing to
 * 	buffer - the data to write
 * 	count - the number of bytes to write (same as length of buffer)
 * returns:
 * 	The number of bytes successfully written or an error value.
 */
ssize_t block_file_write(struct file* file, const char* buffer, size_t count)
{
	dev_t devid = file->f_path.p_dentry->d_inode->i_dev;
	ssize_t result = block_write(devid, file->f_off, count, buffer);
	
	if( result > 0 ){
		file->f_off += result;
	}
	
	return result;
}