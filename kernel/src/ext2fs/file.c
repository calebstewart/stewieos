#include <errno.h>
#include "ext2.h"
#include "kmem.h"
#include "block.h"
#include "dentry.h"
#include "task.h"

int e2_file_open(struct file* file ATTR((unused)), struct dentry* dentry ATTR((unused)), int mode ATTR((unused)))
{
	 return 0;
}

int e2_file_close(struct file* file ATTR((unused)), struct dentry* dentry ATTR((unused)))
{
	file_flush(file);
	return 0;
}

ssize_t e2_file_read(struct file* file, char* buffer, size_t count)
{
	struct inode* inode = file_inode(file); // get the inode
	
	// Do the operation
	ssize_t result = e2_inode_io(inode, EXT2_READ, file->f_off, count, buffer);
	if( result < 0 ){
		return result;
	}
	
	// Increment the offset
	file->f_off += result;
	
	file_flush(file);
	
	// return byte count
	return result;
}

ssize_t e2_file_write(struct file* file, const char* buffer, size_t count)
{
	struct inode* inode = file_inode(file); // get the inode
	
	// Do the operation
	ssize_t result = e2_inode_io(inode, EXT2_WRITE, file->f_off, count, (char*)buffer);
	if( result < 0 ){
		return result;
	}
	
	// Increment the offset
	file->f_off += result;
	
	file_flush(file);
	
	// return byte count
	return result;
}

int e2_file_readdir(struct file* file, struct dirent* dirent, size_t count)
{
	struct inode* inode = file_inode(file);
	char tmp[264] = {0};
	e2_dirent_t* e2ent = (e2_dirent_t*)tmp;
	
	for(size_t i = 0; i < count; ++i)
	{
		ssize_t nread = e2_inode_io(inode, EXT2_READ, file->f_off, 264, (char*)e2ent);
		if( nread == 0 ){
			file_flush(file);
			return (int)i;
		} else if( nread < 0 ){
			file_flush(file);
			return (int)nread;
		} else if( e2ent->inode != 0 ) {
			dirent[i].d_ino = (ino_t)e2ent->inode;
			dirent[i].d_type = e2ent->file_type;
			strncpy(dirent[i].d_name, e2ent->name, e2ent->name_len);
			dirent[i].d_name[e2ent->name_len] = 0;
		} else {
			i--;
		}
		file->f_off += e2ent->rec_len;
	}
	
	file_flush(file);
	
	return count;
}