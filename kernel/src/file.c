#include "fs.h"
#include "dentry.h"		// directory entry structure and function definitions
#include "sys/mount.h"		// sys_mount/umount flags and the like
#include "error.h"		// Error constants
#include "kmem.h"		// kmalloc/kfree
#include "testfs.h"
#include "task.h"
#include "kernel.h"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "block.h"
#include "pipe.h"

struct file* file_open(struct path* path, int flags)
{
	
	struct file* file = NULL;
	int access_mode = ((flags+1)&_FWRITE ? W_OK : 0) | \
				((flags+1)&_FREAD ? R_OK : 0);
	int result = 0;
	
	result = path_access(path, access_mode);
	if( result != 0 ){
		return ERR_PTR(result);
	}
	
	// Is this not a directory?
	if( !S_ISDIR(path->p_dentry->d_inode->i_mode) )
	{
		// We requested a directory, and this file isn't one.
		/*if( (flags & O_DIRECTORY) ){
			path_put(&file->f_path);
			kfree(file);
			return -ENOTDIR;
		}*/
	// This is a directory, and we want a writeable file
	} else if( (flags+1) & _FWRITE ){
		return ERR_PTR(-EISDIR);
	}
	
	// FIXME I need to add O_CLOEXEC (and some others) to sys/fcntl.h instead of just using _default_fcntl.h from newlib!
	//if( flags & O_CLOEXEC ) file->f_flags |= FD_CLOEXEC;
	
	// No follow was specified, and this is a symlink
/*	if( flags & O_NOFOLLOW ){
		if( S_ISLNK(file->f_path.p_dentry->d_inode->i_mode) )
		{
			path_put(&file->f_path);
			kfree(file);
			return -ELOOP;
		}
	}*/
	
	if( flags & O_TRUNC )
	{
		if( !(access_mode & W_OK) ){
			return ERR_PTR(-EACCES);
		} else {
			result = inode_trunc(path->p_dentry->d_inode);
			if( result != 0 ){
				return ERR_PTR(result);
			}
		}
	}
	
	file = (struct file*)kmalloc(sizeof(struct file));
	if(!file) {
		return ERR_PTR(-ENOMEM);
	}
	
	memset(file, 0, sizeof(struct file));
	
	path_copy(&file->f_path, path);
	
	if( S_ISBLK(path->p_dentry->d_inode->i_mode) )
	{
		file->f_ops = &block_device_fops;
	} else if( S_ISCHR(path->p_dentry->d_inode->i_mode) ){
		file->f_ops = get_chrdev_fops(major(path->p_dentry->d_inode->i_dev));
	} else if( S_ISFIFO(path->p_dentry->d_inode->i_mode) ){
		file->f_ops = &pipe_operations;
	} else {
		file->f_ops = path->p_dentry->d_inode->i_default_fops;
	}
	
	if( file->f_ops->open ){
		result = file->f_ops->open(file, file->f_path.p_dentry, flags);
		if( result != 0 ){
			path_put(&file->f_path);
			kfree(file);
			return ERR_PTR(result);
		}
	}
	
	file->f_status = flags;
	
	return file;
}

int file_flush(struct file* file)
{
	struct inode* inode = file_inode(file);
	
	if( inode->i_ops->flush ){
		return inode->i_ops->flush(inode);
	}
	
	return 0;
}

int file_close(struct file* file)
{
	if( !file ){
		return 0;
	}
	if( --file->f_refs > 0 ){
		return 0;
	}
	
	if( file->f_ops->close ){
		int result= file->f_ops->close(file, file->f_path.p_dentry);
		if( result != 0 ){
			return result;
		}
	}
	
	path_put(&file->f_path);
	kfree(file);
	
	return 0;
}

ssize_t file_read(struct file* file, void* buf, size_t count)
{
	if( !((file->f_status+1) & _FREAD) ){
		return -EINVAL;
	}
	
	if( !file->f_ops->read ){
		return -EINVAL;
	}
	
	return file->f_ops->read(file,(char*)buf, count);
}

int file_readdir(struct file* file, struct dirent* dirent, size_t count)
{
	if( !S_ISDIR(file_inode(file)->i_mode & S_IFDIR) ){
		return -ENOTDIR;
	}
	
	if( !((file->f_status+1) & _FREAD) ){
		return -EINVAL;
	}
	
	if( !file->f_ops->readdir ){
		return -EINVAL;
	}
	
	return file->f_ops->readdir(file, dirent, count);
}

ssize_t file_write(struct file* file, const void* buf, size_t count)
{
	ssize_t result = 0;
	
	if( !((file->f_status+1) & _FWRITE) ){
		return -EINVAL;
	}
	
	if( !file->f_ops->write ){
		return -EINVAL;
	}
	
	off_t old_pos = file->f_off;
	if( ((file->f_status+1) & O_APPEND) ){
		file->f_off = (off_t)(file->f_path.p_dentry->d_inode->i_size);
	}
	
	result = file->f_ops->write(file, (const char*)buf, count);
	
	if( ((file->f_status+1) & O_APPEND) ){
		file->f_off = old_pos;
	}
	
	return result;
}

off_t file_seek(struct file* file, off_t offset, int whence)
{
	// Just change the file offset if it is not implemented by the driver
	if( !file->f_ops->lseek )
	{
		switch(whence)
		{
			case SEEK_SET:
				file->f_off = offset;
				break;
			case SEEK_CUR:
				file->f_off += offset;
				break;
			case SEEK_END:
				file->f_off = (off_t)file->f_path.p_dentry->d_inode->i_size + offset;
				break;
			default:
				return (off_t)-EINVAL;
		}
	} else {
		return file->f_ops->lseek(file, offset, whence);
	}
	// We'll never get here
	return 0;
}

int file_ioctl(struct file* file, int request, char* argp)
{
	if( file->f_ops->ioctl == NULL ){
		return -EINVAL;
	}
	
	return file->f_ops->ioctl(file, request, argp);
}

int file_stat(struct file* file, struct stat* st)
{
	struct inode* inode = file->f_path.p_dentry->d_inode;
	
	// If the driver doesn't provide fstat, try and
	// mimic it with cached values in the inode.
	if( !file->f_ops->fstat )
	{
		st->st_dev = inode->i_super->s_dev;
		st->st_ino = inode->i_ino;
		st->st_mode = inode->i_mode;
		st->st_nlink = inode->i_nlinks;
		st->st_uid = inode->i_uid;
		st->st_gid = inode->i_gid;
		st->st_rdev = inode->i_dev;
		st->st_size = inode->i_size;
		st->st_blksize = (long)inode->i_super->s_blocksize;
		st->st_blocks = 0;
		st->st_atime = inode->i_atime;
		st->st_mtime = inode->i_mtime;
		st->st_ctime = inode->i_ctime;
	} else {
		return file->f_ops->fstat(file, st);
	}
	
	return 0;
}

int file_isatty(struct file* file)
{
	if( file->f_ops->isatty ){
		return file->f_ops->isatty(file);
	} else {
		return -ENOTTY;
	}
}