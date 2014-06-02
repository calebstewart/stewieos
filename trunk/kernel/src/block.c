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

static block_data_t* block_file_data(struct file* file);
static int  block_file_data_sync(struct file* file);
static void block_data_lock(block_data_t* data);
static void block_data_unlock(block_data_t* data);

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
	int result = block_open(dentry->d_inode->i_dev);
	if( result != 0 ){
		return result;
	}
	
	struct block_device* device = get_block_device(dentry->d_inode->i_dev);
	if( device == NULL ){
		return -ENODEV;
	}
	
	file->f_private = (char*)kmalloc(sizeof(block_data_t));
	file->f_off = 0;
	block_data_t* data = block_file_data(file);
	data->block = (char*)kmalloc(device->blksz);
	data->dirty = 0;
	data->dirty_block = 0;
	spin_init(&data->lock);
	
	block_file_data_sync(file);
	
	return 0;
}

int block_file_close(struct file* file, struct dentry* dentry)
{
	UNUSED(dentry);
	kfree(file->f_private);
	return block_close(file->f_path.p_dentry->d_inode->i_dev);
}

block_data_t* block_file_data(struct file* file)
{
	return (block_data_t*)file->f_private;
}

void block_data_lock(block_data_t* data)
{
	spin_lock(&data->lock);
}

void block_data_unlock(block_data_t* data)
{
	spin_unlock(&data->lock);	
}

int block_file_data_sync(struct file* file)
{
	block_data_t* data = block_file_data(file);
	dev_t devid = file->f_path.p_dentry->d_inode->i_dev;
	struct block_device* device = get_block_device(devid);
	int result = 0;
	
	if( (file->f_off%device->blksz) == 0 )
	{
		// if data was written to the buffer, flush it to the device
		if( data->dirty ){
			block_write(devid, data->block, data->dirty_block);
			data->dirty = 0;
		}
		if( (result = block_read(devid, data->block, file->f_off%device->blksz)) != 0 ){
			return result;
		}
		data->dirty_block = file->f_off%device->blksz;
	}
	return 0;
}

ssize_t block_file_read(struct file* file, char* buffer, size_t count)
{
	dev_t devid = file->f_path.p_dentry->d_inode->i_dev;
	struct block_device* device = get_block_device(devid);
	size_t nread = 0;
	int result = 0;
	block_data_t* data = block_file_data(file);

	// invalid device
	if( !device ){
		return (ssize_t)-ENODEV;
	}	
	// we don't need to read for that...
	if( count == 0 ) {
		return 0;
	}
	
	block_data_lock(data);
	
	// read all the requested bytes
	while( count > 0 )
	{
		// this will synchronize the buffered data->block with the current file->f_off
		// If this is the end of the disk or there was a read error, it will return an
		// error code
		if( (result = block_file_data_sync(file)) != 0 ){
			break;
		}
		*buffer = data->block[file->f_off%device->blksz]; // read the next byte
		file->f_off++;
		nread++;
		buffer++;
	}
	
	block_data_unlock(data);
	
	if( result < 0 ){
		nread = (ssize_t)result;
	}
	
	return nread;
}

ssize_t block_file_write(struct file* file, const char* buffer, size_t count)
{
	dev_t devid = file->f_path.p_dentry->d_inode->i_dev;
	struct block_device* device = get_block_device(devid);
	size_t nwritten = 0;
	int result = 0;
	block_data_t* data = block_file_data(file);

	// invalid device
	if( !device ){
		return (ssize_t)-ENODEV;
	}	
	// we don't need to read for that...
	if( count == 0 ) {
		return 0;
	}
	
	block_data_lock(data);
	
	// read all the requested bytes
	while( count > 0 )
	{
		// this will synchronize the buffered data->block with the current file->f_off
		// If this is the end of the disk or there was a read error, it will return an
		// error code
		if( (result = block_file_data_sync(file)) != 0 ){
			break;
		}
		// when we fill this buffered block or close the file it will sync to the device
		data->block[file->f_off%device->blksz] = *buffer; // write the next byte
		data->dirty = 1;
		file->f_off++;
		nwritten++;
		buffer++;
	}
	
	block_data_unlock(data);
	
	if( result < 0 ){
		nwritten = (ssize_t)result;
	}
	
	return nwritten;
}