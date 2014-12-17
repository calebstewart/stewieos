#include "fs.h"
#include "kernel.h"
#include "kmem.h"
#include "multiboot.h"
#include "sys/mount.h"
#include <sys/fcntl.h>
#include "error.h"
#include "dentry.h"

// inode operations
int initfs_inode_lookup(struct inode* inode, struct dentry* dentry);
int initfs_inode_mknod(struct inode* inode, const char* name, mode_t mode, dev_t device);
int initfs_inode_node_unlink(struct inode* inode);
// file operations
int initfs_file_open(struct file* file, struct dentry* dentry, int flags);
int initfs_file_close(struct file* file, struct dentry* dentry);
ssize_t initfs_file_read(struct file* file, char* buffer, size_t count);
//int initfs_file_readdir(struct file* file, struct dirent* dirent);
off_t initfs_file_lseek(struct file* file, off_t offset, int whence);
int initfs_file_ioctl(struct file* file, int cmd, char* argp);
int initfs_file_fstat(struct file* file, struct stat* stat);
// filesystem operations
int initfs_read_super(struct filesystem* filesystem, struct superblock* super, dev_t devid, unsigned long flags, void* data);
int initfs_put_super(struct filesystem* filesystem, struct superblock* super);
// superblock operations
int initfs_super_read_inode(struct superblock* super, struct inode* inode);


struct filesystem_operations initfs_ops = {
	.read_super = initfs_read_super,
	.put_super = initfs_put_super
};

// The root inode can lookup children
struct inode_operations initfs_root_inode_ops = {
	.lookup = initfs_inode_lookup,
	.mknod = initfs_inode_mknod,
};

// Regular files do nothing with respect to inodes...
struct inode_operations initfs_inode_ops = {
	.lookup = NULL,
};

// Device node operations
struct inode_operations initfs_inode_node_ops = {
	.unlink = initfs_inode_node_unlink
};

struct file_operations initfs_file_ops = {
	.open = initfs_file_open,
	.close = initfs_file_close,
	.read = initfs_file_read,
//	.lseek = initfs_file_lseek,
//	.fstat = initfs_file_lstat,
//	.ioctl = initfs_file_ioctl
};

struct file_operations initfs_file_dir_ops = {
	.open = initfs_file_open,
	.close = initfs_file_close,
//	.readdir = initfs_file_readdir
};

struct superblock_operations initfs_super_ops = {
	.read_inode = initfs_super_read_inode,
};

struct filesystem initfs = {
	.fs_name = "initfs",
	.fs_flags = FS_NODEV,
	.fs_ops = &initfs_ops
};

struct initfs_node
{
	char name[128];
	dev_t devid;
	mode_t mode;
};

struct initfs_private
{
	u32 nfiles;
	multiboot_module_t* list;
	struct initfs_node* node[32];
};

int initfs_install(multiboot_info_t* mb);
int initfs_install(multiboot_info_t* mb)
{
	
	int result = register_filesystem(&initfs);
	if( result != 0 )
	{
		return result;
	} else { 
		result = sys_mount(NULL, "/", "initfs", 0, ((char*)mb));
		if( result != 0 )
		{
			syslog(KERN_ERR, "initfs: unable to mount initfs! error code %d.", -result);
			return result;
		}
	}
	return 0;
}

int initfs_read_super(struct filesystem* fs, struct superblock* super, dev_t devid, unsigned long flags, void* data)
{
	UNUSED(fs); UNUSED(devid); UNUSED(flags);
	multiboot_info_t* mb = (multiboot_info_t*)data;
	multiboot_module_t* modlist = NULL;
	
	// was the bootloader configured to give us modules?
	if( !(mb->flags & MULTIBOOT_INFO_MODS) ){
		syslog(KERN_WARN, "initfs: no modules found (flags: 0x%X).\n", mb->flags);
		//return -ENODEV;
		mb->mods_count = 0;
	} else {
		modlist = (multiboot_module_t*)(mb->mods_addr);
		
		// The kernel shifts the main multiboot structure pointers for
		// the higher half, but does not recurse into sub-structures.
		// We need to fix our own structures.
		for(multiboot_uint32_t i = 0; i < mb->mods_count; ++i){
			modlist[i].mod_start += KERNEL_VIRTUAL_BASE;
			modlist[i].mod_end += KERNEL_VIRTUAL_BASE;
			modlist[i].cmdline += KERNEL_VIRTUAL_BASE;
		}
	}
	
	struct initfs_private* priv = (struct initfs_private*)kmalloc(sizeof(struct initfs_private));
	if(!priv){
		return -ENOMEM;
	}
	
	priv->nfiles = mb->mods_count;
	priv->list = modlist;
	for(int i = 0; i < 32; ++i){
		priv->node[i] = NULL;
	}
	
	super->s_private = priv;
	super->s_ops = &initfs_super_ops; // initialize the superblock operations
	struct inode* root_inode = i_get(super, 0);
	super->s_root = d_alloc_root(root_inode); // initialize the root dentry
	i_put(root_inode);
	return 0; // that was easy!
}

int initfs_put_super(struct filesystem* filesystem, struct superblock* super)
{
	UNUSED(filesystem);
	kfree(super->s_private);
	return 0;
}

int initfs_super_read_inode(struct superblock* super, struct inode* inode)
{
	struct initfs_private* priv = (struct initfs_private*)super->s_private;
	
	if( inode->i_ino == 0 )
	{
		inode->i_mode = S_IFDIR | 0777;
		inode->i_default_fops = &initfs_file_dir_ops;
		inode->i_ops = &initfs_root_inode_ops;
		return 0;
	}
	
	if( inode->i_ino > priv->nfiles ){
		if( (inode->i_ino-priv->nfiles) > 32 ){
			return -ENOENT;
		} else if( priv->node[inode->i_ino-priv->nfiles-1] == NULL ){
			return -ENOENT;
		} else {
			inode->i_mode = priv->node[inode->i_ino-priv->nfiles-1]->mode;
			inode->i_default_fops = &initfs_file_ops;
			inode->i_ops = &initfs_inode_node_ops;
			inode->i_size = 0;
			inode->i_dev = priv->node[inode->i_ino-priv->nfiles-1]->devid;
			return 0;
		}
	}
	
	inode->i_mode = S_IFREG | 0777;
	inode->i_default_fops = &initfs_file_ops;
	inode->i_ops = &initfs_root_inode_ops;
	inode->i_size = priv->list[inode->i_ino-1].mod_end - priv->list[inode->i_ino-1].mod_start;
	
	return 0;
}

int initfs_inode_lookup(struct inode* inode, struct dentry* dentry)
{
	struct initfs_private* priv = (struct initfs_private*)inode->i_super->s_private;
	
	for(ino_t i = 0; i < (ino_t)priv->nfiles; i++)
	{
		if( strcmp((char*)priv->list[i].cmdline, dentry->d_name) == 0 ){
			struct inode* child = i_get(inode->i_super, (ino_t)(i+1));
			dentry->d_inode = child;
			dentry->d_ino = (ino_t)(i+1);
			return 0;
		}
	}
	
	for(int i = 0; i < 32; ++i){
		if( priv->node[i] ){
			if( strcmp(priv->node[i]->name, dentry->d_name) == 0 ){
				struct inode* child = i_get(inode->i_super, (ino_t)(i + priv->nfiles + 1));
				dentry->d_inode = child;
				dentry->d_ino = (ino_t)(i + priv->nfiles + 1);
				return 0;
			}
		}
	}
	
	return -ENOENT;
}

int initfs_inode_node_unlink(struct inode* inode)
{
	struct initfs_private* priv = (struct initfs_private*)inode->i_super->s_private;
	int i = inode->i_ino - priv->nfiles - 1;
	struct initfs_node* node = priv->node[i];
	priv->node[i] = NULL;
	
	kfree(node);
	
	return 0;
}

int initfs_inode_mknod(struct inode* inode, const char* name, mode_t mode, dev_t devid)
{
	struct initfs_private* priv = (struct initfs_private*)inode->i_super->s_private;
	
	// Check that this the root (the only directory)
	if( inode->i_ino != 0 ){
		return -ENOTDIR;
	}
	
	// Check the name length
	if( strlen(name) >= 128 ){
		return -ENAMETOOLONG;
	}
	
	// Allocate the node structure
	struct initfs_node* node = (struct initfs_node*)(kmalloc(sizeof(struct initfs_node)));
	if(!node){
		return -ENOMEM;
	}
	
	// Setup the node structure
	memset(node, 0, sizeof(struct initfs_node));
	strncpy(node->name, name, 127);
	node->mode = mode;
	node->devid = devid;
	
	// Look for an open device node
	for(int i = 0; i < 32; ++i)
	{
		if( priv->node[i] == NULL ){
			priv->node[i] = node;
			return 0;
		}
	}
	
	// No more device nodes available
	kfree(node);
	
	return -ENOSPC;
}

int initfs_file_open(struct file* file, struct dentry* dentry, int flags)
{
	UNUSED(file); UNUSED(flags);
	struct initfs_private* priv = (struct initfs_private*)dentry->d_inode->i_super->s_private;
	file->f_private = &priv->list[dentry->d_ino-1];
	return 0;	
}

int initfs_file_close(struct file* file, struct dentry* dentry)
{
	UNUSED(file); UNUSED(dentry);
	return 0;
}

ssize_t initfs_file_read(struct file* file, char* buffer, size_t count)
{
	multiboot_module_t* module = (multiboot_module_t*)(file->f_private);
	u32 remaining = (module->mod_end - module->mod_start) - file->f_off;
	
	// read to end of file
	if( remaining < count ){
		count = remaining;
	}
	// either the user asked for 0 bytes or this is the end of the file
	if( count == 0 ){
		return 0;
	}
	
	// copy the data 
	memcpy(buffer, (void*)( module->mod_start + file->f_off ), count);
	
	// increment the file offset
	file->f_off += count;
	
	// return the number of bytes read
	return count;
}

//int initfs_file_readdir(struct file* file, struct dirent* dirent);
//off_t initfs_file_lseek(struct file* file, off_t offset, int whence);
//int initfs_file_ioctl(struct file* file, int cmd, char* argp);
//int initfs_file_fstat(struct file* file, struct stat* stat);
