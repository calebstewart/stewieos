#include "exec.h" // For the module definitions
#include "kernel.h" // For printk
#include "ext2fs/ext2.h"

// Forward Declarations
int ext2fs_load(module_t* module);
int ext2fs_remove(module_t* module);

// This tells the operating system the module name, and the
// load and remove functions to be called at respective times.
// MODULE_INFO(MODULE_NAME, ext2fs_load, ext2fs_remove);

// Filesystem operations structures
struct inode_operations e2_inode_operations = {
	.mknod = e2_inode_mknod, .lookup = e2_inode_lookup,
	.truncate = e2_inode_truncate, .flush = e2_inode_flush,
	.unlink = e2_inode_unlink,
};
struct file_operations e2_default_file_operations = {
	.open = e2_file_open, .close = e2_file_close, 
	.read = e2_file_read, .write = e2_file_write,
	.readdir = e2_file_readdir
};
struct superblock_operations e2_superblock_ops = {
	.read_inode = e2_read_inode, .put_inode = e2_put_inode,
};
struct filesystem_operations e2_filesystem_ops = {
	.read_super = e2_read_super, .put_super = e2_put_super,
};
struct filesystem e2_filesystem = {
	.fs_name = "ext2",
	.fs_flags = 0,
	.fs_ops = &e2_filesystem_ops,
};

// This function will be called during module insertion
// The return value should be zero for success. All other
// values are passed back from insmod.
int ext2fs_load(module_t* module ATTR((unused)))
{
	info_message("registering filesystem type...", 0);
	int result = register_filesystem(&e2_filesystem);
	if( result != 0 ){
		info_message("unable to register filesystem. error code %d", result);
		return result;
	}
	return 0;
}

// This function will be called during module removal.
// It is only called when module->m_refs == 0, therefore
// you don't need to worry about reference counting. Just
// free any data you allocated, and return. Any non-zero
// return value will be passed back from rmmod
int ext2fs_remove(module_t* module ATTR((unused)))
{
	info_message("unregistering filesystem type...", 0);
	int result = unregister_filesystem(&e2_filesystem);
	if( result != 0 ){
		info_message("unable to unregister filesystem. error code %d", result);
		return result;
	}
	return 0;
}

void e2_install_filesystem( void )
{
	ext2fs_load(NULL);
}