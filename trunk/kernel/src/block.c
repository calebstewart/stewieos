#include "kernel.h"
#include "block.h"
#include "fs.h"
#include "dentry.h"
#include "kmem.h"
#include "sys/types.h"
#include <errno.h>

typedef struct {
	char*  block;
	int dirty;
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

static block_data_t* block_file_data(struct file* file);
static void block_file_data_sync(struct file* file);
static void block_data_lock(block_data_t* data);
static void block_data_unlock(block_data_t* data);

struct block_device* vfs_dev[256];
struct file_operations block_device_fops = {
	/*.open = block_file_open,
	.close = block_file_close,
	.read = block_file_read,
	.write = block_file_write,
	.ioctl = block_file_ioctl,
	.fstat = block_file_fstat*/
};

int register_major_device(unsigned int major, unsigned int nminors, struct block_operations* block_ops)
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
	if( major >= 256 ) return -1;
	if( vfs_dev[major] == NULL ) return -1;
	
	struct block_device* dev = (struct block_device*)kmalloc(sizeof(struct block_device));
	if(!dev){
		return -1;
	}
	memset(dev, 0, sizeof(struct block_device));
	
	dev->ops = block_ops;
	dev->nminors = nminors;
	
	vfs_dev[major] = dev;
	
	int result = block_attach(makedev(major,0));
	if( result != 0 ){
		vfs_dev[major] = NULL;
		kfree(dev);
		return result;
	}
	
	return 0;
}

int unregister_major_device(unsigned int major)
{
	if( major >= 256 || vfs_dev[major] == NULL ){
		return 0;
	}
	if( vfs_dev[major]->refs != 0 ){
		return -EBUSY;
	}
	
	struct block_device* dev = vfs_dev[major];
	vfs_dev[major] = NULL;
	
	if( dev->ops->detach ){
		int result = dev->ops->detach(dev);
		if( result != 0 ){
			vfs_dev[major] = dev;
			return result;
		}
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
	
	return device;
}

int block_attach(dev_t devid)
{
	struct block_device* device = get_block_device(devid);
	if( !device ) return -1;
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

int block_read(dev_t devid, char* buffer, off_t lba)
{
	struct block_device* device = get_block_device(devid);
	if( !device ) return -1;
	if( device->ops->read ){
		int result = device->ops->read(device, devid, buffer, lba);
		return result;
	}
	return -1;
}

int block_write(dev_t devid, const char* buffer, off_t lba)
{
	struct block_device* device = get_block_device(devid);
	if( !device ) return -1;
	if( device->ops->write ){
		int result = device->ops->write(device, devid, buffer, lba);
		return result;
	}
	return -1;
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

int block_file_open(struct file* file, struct dentry* dentry, int omode)
{
	UNUSED(omode);
	int result = block_open(dentry->d_inode->i_dev);
	if( result != 0 ){
		return result;
	}
	
	struct block_device* device = get_block_device(dentry->d_inode->i_dev);
	if( device == NULL ){
		return -ENODEV;
	}
	
	file->f_private = (char*)kmalloc(device->blksz);
	file->f_off = 0;
	
	return 0;
}

int block_file_close(struct file* file, struct dentry* dentry)
{
	UNUSED(dentry);
	kfree(file->f_private);
	return block_close(file->f_path.p_dentry->d_inode->i_dev);
}

int file_read_next_block(struct file* file)
{
	dev_t dev = file->f_path.p_dentry->d_inode->i_dev;
	struct block_device* device = get_block_device(dev);
	char* buffer = (char*)file->f_private;
	
	off_t lba = file->f_off / device->blksz;
	int result = block_read(dev, buffer, lba);
	if( result != 0 ){
		return result;
	}
	
	return 0;
}

block_data_t* block_file_data(struct file* file)
{
	return (block_data_t*)file->f_private;
}

ssize_t block_file_read(struct file* file, char* buffer, size_t count)
{
	dev_t devid = file->f_path.p_dentry->d_inode->i_dev;
	struct block_device* device = get_block_device(devid);
	size_t nread = 0;
	int result = 0;
	char* block = (char*)file->f_private;

	// invalid device
	if( !device ){
		return (ssize_t)-ENODEV;
	}	
	// we don't need to read for that...
	if( count == 0 ) {
		return 0;
	}
	// read all the requested bytes
	while( count > 0 )
	{
		// if we finished that block, read the next one
		if( (file->f_off%device->blksz) == 0 ){
			result = file_read_next_block(file);
			if( result != 0 ){
				return nread;
			}
		}
		*buffer = block[file->f_off%device->blksz];
		file->f_off++;
		nread++;
		buffer++;
	}
	
	return nread;
}
/*
ssize_t block_file_write(struct file* file, const char* buffer, size_t count)
{
	dev_t devid = file->f_path.p_dentry->d_inode->i_dev;
	struct block_device* device = get_block_device(devid);
	size_t nwritten = 0;
	int result = 0;
	char* block = NULL;
	
	if(!device){
		return (ssize_t)-ENODEV;
	}
	if( count == 0 ){
		return 0;
	}
	
	*/