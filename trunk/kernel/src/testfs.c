#include "testfs.h"
#include "linkedlist.h"
#include "kmem.h"
#include "kernel.h"

int testfs_read_super(struct filesystem* fs, struct superblock* super, dev_t device, unsigned long flags);
int testfs_read_inode(struct superblock* super, struct inode* inode);
int testfs_inode_lookup(struct inode* inode, struct dentry* dentry);

struct filesystem_operations testfs_ops = {
	.read_super = testfs_read_super
};

struct superblock_operations testfs_superblock_ops = {
	.read_inode = testfs_read_inode
};

struct inode_operations testfs_inode_ops = {
	.lookup = testfs_inode_lookup
};

struct filesystem testfs_type = {
	.fs_name = "testfs",
	.fs_flags = FS_NODEV,
	.fs_ops = &testfs_ops
};

int testfs_read_super(struct filesystem* fs, struct superblock* super, dev_t device, unsigned long flags)
{
	// Shut up the compiler...
	UNUSED(fs);
	UNUSED(device);
	UNUSED(flags);
	
	super->s_magic = 0x2BADB002;
	super->s_ops = &testfs_superblock_ops;
	
	struct inode* inode = i_get(super, 0);
	super->s_root = d_alloc_root(inode);
	
	return 0;
}

int testfs_read_inode(struct superblock* super, struct inode* inode)
{
	UNUSED(super);
	inode->i_ops = &testfs_inode_ops;
	inode->i_size = 42;
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