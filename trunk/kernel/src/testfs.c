#include "testfs.h"
#include "linkedlist.h"
#include "kmem.h"
#include "kernel.h"
#include <errno.h>

int testfs_read_super(struct filesystem* fs, struct superblock* super, dev_t device, unsigned long flags, void*);
int testfs_put_super(struct filesystem* fs, struct superblock* super);
int testfs_read_inode(struct superblock* super, struct inode* inode);
int testfs_inode_lookup(struct inode* inode, struct dentry* dentry);
int testfs_file_open(struct file* file, struct dentry* dentry, int omode);
int testfs_file_close(struct file* file, struct dentry* dentry);

struct filesystem_operations testfs_ops = {
	.read_super = testfs_read_super,
	.put_super = testfs_put_super,
};

struct superblock_operations testfs_superblock_ops = {
	.read_inode = testfs_read_inode
};

struct file_operations testfs_file_ops = {
	.open = testfs_file_open,
	.close = testfs_file_close
};

struct inode_operations testfs_inode_ops = {
	.lookup = testfs_inode_lookup
};

struct filesystem testfs_type = {
	.fs_name = "testfs",
	.fs_flags = FS_NODEV,
	.fs_ops = &testfs_ops
};

int testfs_read_super(struct filesystem* fs, struct superblock* super, dev_t device, unsigned long flags, void* data)
{
	// Shut up the compiler...
	UNUSED(fs);
	UNUSED(device);
	UNUSED(flags);
	UNUSED(data);
	
	super->s_magic = 0x2BADB002;
	super->s_ops = &testfs_superblock_ops;
	
	// Lookup the root inode, and create the root directory entry
	struct inode* inode = i_get(super, 0);
	super->s_root = d_alloc_root(inode);
	// The dentry has its own reference, we can release this one.
	i_put(inode);
	return 0;
}

int testfs_put_super(struct filesystem* fs, struct superblock* super)
{
	UNUSED(fs);
	d_put(super->s_root);
	return 0;
}

int testfs_read_inode(struct superblock* super, struct inode* inode)
{
	UNUSED(super);
	inode->i_ops = &testfs_inode_ops;
	inode->i_size = 42;
	inode->i_default_fops = &testfs_file_ops;
	return 0;
}

// this is really simply... just for testing (any entry we look for will be the parent... odd, but whatever)
int testfs_inode_lookup(struct inode* inode, struct dentry* dentry)
{
	dentry->d_inode = i_getref(inode);
	dentry->d_ino = inode->i_ino;
	dentry->d_uid = inode->i_uid;
	dentry->d_gid = inode->i_gid;
	dentry->d_size = inode->i_size;
	dentry->d_mode = inode->i_mode;
	return 0;
}

int testfs_file_open(struct file* file, struct dentry* dentry, int omode)
{
	UNUSED(file); UNUSED(dentry); UNUSED(omode);
	printk("testfs: opened file %s\n", dentry->d_name);
	return 0;
}

int testfs_file_close(struct file* file, struct dentry* dentry)
{
	UNUSED(file); UNUSED(dentry);
	printk("testfs: closed file %s\n", dentry->d_name);
	return 0;
}