#include "tty.h"
#include "chrdev.h"
#include "error.h"
#include "dentry.h"
#include <termios.h>
#include <ctype.h>

tty_driver_t* tty_driver[256]; // a driver for each major number.. I should probably use a list...
struct termios default_termios = {
	.c_iflag = ISTRIP | IGNCR | ICRNL,
	.c_oflag = 0,
	.c_cflag = CS8,
	.c_lflag = ICANON | ECHO | ECHOE,
	.c_cc = {
		[VEOL] = '\n',
		[VERASE] = '\b',
		[VTIME] = 3,
	}
};

int tty_file_open(struct file* file, struct dentry* dentry, int mode);
int tty_file_close(struct file* file, struct dentry* dentry);
ssize_t tty_file_read(struct file* file, char* buffer, size_t count);
ssize_t tty_file_write(struct file* file, const char* buffer, size_t count);
int tty_file_ioctl(struct file* file, int cmd, char* parm);
int tty_file_isatty(struct file* file);

struct file_operations tty_file_ops = {
	.open = tty_file_open,
	.close = tty_file_close,
	.read = tty_file_read,
	.write = tty_file_write,
	.ioctl = tty_file_ioctl,
	.isatty = tty_file_isatty,
};

tty_driver_t* tty_find_driver(unsigned int major)
{
	if( major >= 256 ){
		return NULL;
	}
	return tty_driver[major];
}

tty_driver_t* tty_alloc_driver(const char* name, unsigned int major, unsigned int nminors, struct tty_operations* ops)
{
	if( major >= 256 ){
		return ERR_PTR(-EINVAL);
	}
	
	// Allocate the driver structure
	tty_driver_t* driver = (tty_driver_t*)kmalloc(sizeof(tty_driver_t)+nminors*sizeof(tty_device_t));
	if( !driver ){
		unregister_chrdev(major);
		return ERR_PTR(-ENOMEM);
	}
	
	// Fill in the fields
	driver->major = major;
	driver->nminors = nminors;
	driver->private = NULL;
	driver->open = NULL;
	driver->close = NULL;
	memset(driver->device, 0, sizeof(tty_device_t)*nminors);
	
	// Clear out all the devices
	// and setup their queues
	for(unsigned int i = 0; i < nminors; ++i){
		driver->device[i].devid = makedev(major, i);
		driver->device[i].ops = ops;
		driver->device[i].task = NULL;
		driver->device[i].queue.read = driver->device[i].queue.queue;
		driver->device[i].queue.write = driver->device[i].queue.queue;
//		driver->device[i].flags = 0;
		driver->device[i].refs = 0;
		memcpy(&driver->device[i].termios, &default_termios, sizeof(default_termios));
		spin_init(&driver->device[i].lock);
	}

	tty_driver[driver->major] = driver;
	
	// Register the character device major number
	int result = register_chrdev(major, name, &tty_file_ops);
	if( result != 0 ){
		kfree(driver);
		return NULL;
	}
	
	return driver;
}

int tty_free_driver(unsigned int major)
{
	if( major >= 256 ){
		return -EINVAL;
	}
	
	tty_driver_t* driver = tty_driver[major];
	
	for(unsigned int i =0; i < driver->nminors; ++i){
		spin_lock(&driver->device[i].lock);
		if( driver->device[i].refs != 0 ){
			spin_unlock(&driver->device[i].lock);
			return -EBUSY;
		}
		spin_unlock(&driver->device[i].lock);
	}
	
	int result = unregister_chrdev(major);
	if( result != 0 ){
		return result;
	}
	
	tty_driver[major] = NULL;
	kfree(driver);
	
	return 0;
}

int tty_file_isatty(struct file* file __attribute__((unused)))
{
	return 1;
}

int tty_file_close(struct file* file, struct dentry* dentry __attribute__((unused)))
{
	tty_device_t* device = (tty_device_t*)file->f_private;
	
	spin_lock(&device->lock);
	
	// Check if there is a close function and this is the last reference
	if( device->ops->close && device->refs == 1 ){
		int error = device->ops->close(device);
		if( error != 0 ){
			spin_unlock(&device->lock);
			return error;
		}
	}
	
	device->refs--;
	
	// Remove the task reference if this is the last reference
	if( device->refs == 0 ){
		device->task = NULL;
	}
	
	file->f_private = NULL;
	
	spin_unlock(&device->lock);
	
	return 0;
}

int tty_file_open(struct file* file, struct dentry* dentry __attribute__((unused)), int mode)
{
	
	unsigned int major = major(file->f_path.p_dentry->d_inode->i_dev);
	if( tty_driver[major] == NULL ){
		return -ENXIO; // No such device driver
	}
	unsigned int minor = minor(file->f_path.p_dentry->d_inode->i_dev);
	if( minor >= tty_driver[major]->nminors ){
		return -ENXIO; // No such device within the driver
	}
	// Lock the device to check the owning process
	spin_lock(&tty_driver[major]->device[minor].lock);
	// Check the owning process
// 	if( tty_driver[major]->device[minor].task != NULL && tty_driver[major]->device[minor].task != current ){
// 		spin_unlock(&tty_driver[major]->device[minor].lock);
// 		return -EIO; // The device is already in use by another process
// 	} else {
// 		tty_driver[major]->device[minor].task = current;
// 		tty_driver[major]->device[minor].refs++;
// 	}
	tty_driver[major]->device[minor].refs++;
	
	// Grab the device for easy access
	tty_device_t* device = &tty_driver[major]->device[minor];
	
	// Newly opened and the open function exists?
	if( device->ops->open && device->refs == 1 )
	{
		int result = device->ops->open(device, mode);
		if( result != 0 ){
			device->refs--;
			if( device->refs == 0 ){
				device->task = NULL;
			}
			spin_unlock(&device->lock);
			return result;
		}
	}
	
	// Set the private file data for later
	file->f_private = device;
	
	// Unlock the device
	spin_unlock(&device->lock);
	
	return 0;
}

/* function: tty_file_read
 * parameters:
 * 	file - the file to read from
 * 	buffer - where to put the data
 * 	count - the number of bytes to read
 * return value:
 * 	the number of bytes actually read
 * purpose:
 */
ssize_t tty_file_read(struct file* file, char* buffer, size_t count)
{
	tty_device_t* device = (tty_device_t*)file->f_private;
	ssize_t n = 0;
	
	// Lock the device so we don't get a partial stream
	spin_lock(&device->lock);
	
	// if( task_getfg() != sys_getpid() ){
	// 	spin_unlock(&device->lock);
	// 	return 0;
	// }
	
	if( device->termios.c_lflag & ICANON )
	{
		if( !tty_end_of_line(device) ){
			device->task = current;
			spin_unlock(&device->lock);
			task_waitio(current);
			spin_lock(&device->lock);
			device->task = NULL;
		}
		n = tty_queue_read(device, buffer, count);
		spin_unlock(&device->lock);
		return n;
	}
	
	// Read in at most 'count' characters of characters
	while( n < (ssize_t)count )
	{
		if( tty_queue_empty(device) ){
			device->task = current;
			spin_unlock(&device->lock);
			task_waitio(current);
			spin_lock(&device->lock);
			device->task = NULL;
		}
		n += tty_queue_read(device, &buffer[n], count-n);	
	}
	
	// Unlock the device for others to use
	spin_unlock(&device->lock);
	
	// return the number of characters read
	return n;
}

void tty_putchar(tty_device_t* device, char c)
{
	if( device->ops->putchar ){
		device->ops->putchar(device, c);
	} else {
		device->ops->write(device, &c, 1);
	}
}

ssize_t tty_file_write(struct file* file, const char* buffer, size_t count)
{
	tty_device_t* device = (tty_device_t*)file->f_private;
	size_t result = 0;
	
	spin_lock(&device->lock);
	
	if( device->ops->write ){
		result = (ssize_t)device->ops->write(device, buffer, count);
	}
	
	spin_unlock(&device->lock);
	
	return result;
}

int tty_file_ioctl(struct file* file, int cmd, char* arg)
{
	tty_device_t* device = (tty_device_t*)file->f_private;
	size_t result = 0;
	
	spin_lock(&device->lock);
	
	if( device->ops->ioctl ){
		result = (ssize_t)device->ops->ioctl(device, cmd, arg);
	}
	
	spin_unlock(&device->lock);
	
	return result;
}

int tty_queue_empty(tty_device_t* device)
{
	return (device->queue.read == device->queue.write);
}

void tty_queue_insert(tty_device_t* device, char c)
{
	if( device->termios.c_iflag & ISTRIP ){
		c &= 0x7f;
	}
	
	if( c == '\n' && (device->termios.c_iflag & INLCR) ){
		c = '\r';
	} else if( c == '\r' && (device->termios.c_iflag & ICRNL) ){
		c = '\n';
	}
	
	if( device->termios.c_iflag & IUCLC ){
		//c = (char)tolower((int)c);
		if( c >= 'A' && c <= 'Z' ){
			c = (char)((c-'A') + 'a');
		}
	}
	
	// Erase Character in ICANON and ECHOE set will erase the previous character
	if( c == device->termios.c_cc[VERASE] && (device->termios.c_lflag & (ICANON | ECHOE)) )
	{
		if( tty_queue_empty(device) ){
			c = 0;
			return;
		} else {
			if( device->queue.write == device->queue.queue ){
				device->queue.write = &device->queue.queue[TTY_QUEUELEN-1];
			} else {
				device->queue.write--;
			}
		}
	} else {
		*device->queue.write = c;
		
		if( device->queue.write == &device->queue.queue[TTY_QUEUELEN-1] ){
			device->queue.write = &device->queue.queue[0];
		} else {
			device->queue.write++;
		}
		
		if( ( (device->termios.c_lflag & ICANON) && c == device->termios.c_cc[VEOL] ) || !(device->termios.c_lflag & ICANON) ){
			if( device->task && T_WAITING(device->task) ){
				task_wakeup(device->task);
			}
		}
	}
	
	// Echo Character Input
	if( (device->termios.c_lflag & ECHO) || (device->termios.c_lflag & ECHONL && c == '\n') ){
		tty_putchar(device, c);
	}
	
}

char tty_queue_remove(tty_device_t* device)
{
	if( tty_queue_empty(device) ){
		return -1;
	}
	
	char result = *device->queue.write;
	
	if( device->queue.write == device->queue.queue ){
		device->queue.write = &device->queue.queue[TTY_QUEUELEN-1];
	} else {
		device->queue.write--;
	}
	
	return result;
}

char tty_queue_getchar(tty_device_t* device)
{
	if( tty_queue_empty(device) ){
		return -1;
	}
	
	char result = *device->queue.read;
	
	if( device->queue.read == &device->queue.queue[TTY_QUEUELEN-1] ){
		device->queue.read = &device->queue.queue[0];
	} else {
		device->queue.read++;
	}
	
	return result;
}

char tty_queue_peek(tty_device_t* device)
{
	if( tty_queue_empty(device) ) return 0;
	if( device->queue.write == device->queue.queue ){
		return device->queue.queue[TTY_QUEUELEN-1];
	} else {
		return *(device->queue.write-1);
	}
}

int tty_end_of_line(tty_device_t* device)
{
	char c = tty_queue_peek(device);
	if( c == '\n' || c == device->termios.c_cc[VEOL] ){
		return 1;
	}
	return 0;
}

ssize_t tty_queue_read(tty_device_t* device, char* buffer, size_t count)
{
	if( tty_queue_empty(device) ){
		return 0;
	}
	
	ssize_t n = 0;
	
	// if write has wrapped to the beginning
	if( device->queue.write < device->queue.read )
	{
		n = (ssize_t)(&device->queue.queue[TTY_QUEUELEN-1] - device->queue.read);
		if( n > (ssize_t)count ) n = (ssize_t)count;
		memcpy(buffer, device->queue.read, n);
		device->queue.read = device->queue.queue;
	}
	
	// was that enough to fill our buffer
	if( n == (ssize_t)count ) return n;
	
	// Nope, read in the rest or at most count total
	if( (ssize_t)(device->queue.write-device->queue.read) > (ssize_t)(count-n) ){
		memcpy(buffer, device->queue.read, (count-n));
		device->queue.read = device->queue.read + (count-n);
		return (ssize_t)count;
	} else {
		memcpy(buffer, device->queue.read, device->queue.write-device->queue.read);
		n += (ssize_t)(device->queue.write-device->queue.read);
		device->queue.read = device->queue.write;
		return n;
	}
	
}
