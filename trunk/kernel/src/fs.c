#include "fs.h"			// Basic filesystem definitions
#include "dentry.h"		// directory entry structure and function definitions
#include "sys/mount.h"		// sys_mount/umount flags and the like
#include "error.h"		// Error constants
#include "kmem.h"		// kmalloc/kfree

// A list of mounts to check a device against currently
// mounted devices
static list_t vfs_mount_list = LIST_INIT(vfs_mount_list);

// internal function prototypes
void i_free(struct inode* inode); // free an inode with no more references

/*
 * function: sys_mount
 * parameters:
 * 		source_name: name of the source file (what to mount at target, if needed)
 * 		target_name: name of the target directory (where to mount the source)
 * 		filesystemtype: name of the filesystem registered with the kernel (e.g. ext2)
 * 		mountflags: MF_* values OR'd together, see sys/mount.h
 * 		data: filesystem specific data, interpreted by the driver (typically a string containing KEY=VALUE options)
 * return value:
 * 		integer value indicating success. A return value of zero indicates success
 * 		while a negative value indicates an error and represents one of the error
 * 		values from errno.h (negated).
 * notes:
 * 		See `man 2 mount' for more details
 */
int sys_mount(const char* source_name, const char* target_name, const char* filesystemtype,
	      unsigned long mountflags, const void* data)
{
	struct dentry		*source = NULL,		// pointer to the source file (should be a device file)
				*target = NULL,		// pointer to the target file (should be a directory)
				*root = NULL;		// What we are mounting to target
	struct filesystem	*filesystem = NULL;	// pointer to the file system type
	int			 error = 0;		// error codes returned by various functions
	dev_t			 device = 0;		// the device to send to the driver
	list_t			*iter;			// iterator for search the mount list
	
	// Shut the compiler up until we implement the resolve_path function
	UNUSED(target_name);
	UNUSED(source_name);
	UNUSED(filesystemtype);
	// get the directory entries for the source and target
	//target = resolve_path(target_name);
	//source = resolve_path(source_name);
	// find the filesystem pointer
	//filesystem = get_filesystem(filesystemtype);
	
	// is the target valid?
	if( IS_ERR(target) ){
		if( !IS_ERR(source) ) d_put(source);
		return PTR_ERR(target);
	}
	// is the filesystem valid?
	if( IS_ERR(filesystem) ){
		d_put(target);
		if( !IS_ERR(source) ) d_put(source);
		return PTR_ERR(filesystem);
	}
	// is the source valid (only if we require a source)
	if( IS_ERR(source) && !(filesystem->fs_flags & FS_NODEV) ){
		d_put(target);
		return PTR_ERR(source);
	}
	if( !(filesystem->fs_flags & FS_NODEV) ){
		device = source->d_inode->i_dev;
		d_put(source);
	} else if( !IS_ERR(source) ){
		d_put(source);
	}
	
	// make sure source isn't already mounted (or this filesystem isn't already mounted, if device is unneeded)
	list_for_each(iter, &vfs_mount_list){
		struct mount* item = list_entry(iter, struct mount, m_globlink); // grab the entry from the list
		// Is there a device to look for?
		if( device == 0 )
		{
			// does this mount also not require a device and use the same file system? BUSY!
			if( item->m_super->s_dev == 0 && item->m_super->s_fs == filesystem ){
				d_put(target); // release the target
				return -EBUSY; // tell the caller that this filesystem is busy
			}
		// We are using a device. Does this device use the same device? BUSY!
		} else if( item->m_super->s_dev == device ){
			d_put(target); // release the target
			return -EBUSY; // tell the caller that this device is already used
		}
	}
	
	// don't allow mounting a read only filesystem as read/write
	if( !(mountflags & MF_RDONLY) && (filesystem->fs_flags & FS_RDONLY) ){
		d_put(target);
		return -EACCES;
	}
	
	// allocate the super block
	struct superblock* super = (struct superblock*)kmalloc(sizeof(struct superblock));
	if(!super){
		d_put(target);
		return -ENOMEM;
	}
	memset(super, 0, sizeof(struct superblock));
	
	// read the superblock
	super->s_fs = filesystem;
	error = filesystem->fs_ops->read_super(filesystem, super, device, mountflags);
	
	// bad source/unable to load! :(
	if( error < 0 ){
		d_put(target);
		kfree(super);
		return error;
	}
	
	root = super->s_root;
	
	// allocate a new mointpoint structure if needed
	if( !target->d_mountpoint )
	{
		target->d_mountpoint = (struct mountpoint*)(kmalloc(sizeof(struct mountpoint)));
		if(!target->d_mountpoint){
			d_put(target);
			filesystem_put_super(filesystem, super);
			kfree(super);
			return -ENOMEM;
		}
		memset(target->d_mountpoint, 0, sizeof(struct mountpoint));
		target->d_mountpoint->mp_point = target;
		INIT_LIST(&target->d_mountpoint->mp_mounts);
	}
	
	// link the root dentry to the mountpoint structure
	root->d_mountpoint = target->d_mountpoint;
	
	// create a new mount
	struct mount* mount = (struct mount*)(kmalloc(sizeof(struct mount)));
	if( !mount ){
		d_put(target);
		filesystem_put_super(filesystem, super);
		kfree(super);
		return -ENOMEM;
	}
	mount->m_super = super;
	mount->m_flags = mountflags;
	mount->m_data  = data;
	mount->m_point = root->d_mountpoint;
	INIT_LIST(&mount->m_mplink);
	INIT_LIST(&mount->m_globlink);
	
	// add the mount to the mount point
	list_add(&mount->m_mplink, &root->d_mountpoint->mp_mounts);
	// add the mount to the global list
	list_add(&mount->m_globlink, &vfs_mount_list);
	
	return 0; 
}

/*
 * function: sys_umount
 * parameters:
 * 		target_name: name of the directory where something is mounted
 * 		flags: MF_* values OR'd together; see sys/mount.h or the manpage
 * return value:
 * 		An integer indicating success or failure. A zero return value
 * 		indicates success, while a negative number indicates error and
 * 		represents a negated errno.h error value.
 * notes:
 * 		See `man 2 umount' for more information
 */
int sys_umount(const char* target_name, int flags)
{
	struct dentry		*target = NULL; // the target directory entry
	struct mount		*mount = NULL; // the mount for this target
	struct filesystem	*filesystem = NULL; // the filesystem type that is mounted
	struct superblock	*super = NULL; // the superblock that is mounted
	UNUSED(flags);
	
	UNUSED(target_name);
	//target = resolve_path(target_name);
	
	if( IS_ERR(target) ){
		return PTR_ERR(target);
	}
	
	if( !target->d_mountpoint )
	{
		d_put(target);
		return -EINVAL;
	}
	
	// that's peculiar, there shouldn't be a mount point structure allocated
	if( list_empty(&target->d_mountpoint->mp_mounts) ){
		kfree(target->d_mountpoint);
		target->d_mountpoint = NULL;
		d_put(target);
		return -EINVAL;
	}
	
	// Grab the top-most mount from the mount point mount list
	mount = list_entry(&target->d_mountpoint->mp_mounts.next, struct mount, m_mplink);
	super = mount->m_super;
	filesystem = super->s_fs;
	
	
	filesystem_put_super(filesystem, super);
	list_rem(&mount->m_mplink);
	
	kfree(mount);
	
	return 0;
}

/*
 * function: i_get
 * parameters:
 * 		super: the superblock to retrieve the inode from
 * 		ino: the inode number of the inode inside the superblock
 * return value:
 * 		The requested inode, or an error value inidicating the problem
 * 		PTR_ERR(return value) will return a negative errno.h error
 * 		constant.
 * notes:
 * 
 */
struct inode* i_get(struct superblock* super, ino_t ino)
{	
	struct inode		*inode = NULL;			// the inode structure
	int			 error = 0;			// the return error value
	
	// allocate the inode structure and clear its contents
	inode = (struct inode*)kmalloc(sizeof(struct inode));
	memset(inode, 0, sizeof(struct inode));
	
	// fill in the information we know
	inode->i_ino = ino;
	inode->i_super = super;
	inode->i_ref = 1;
	INIT_LIST(&inode->i_sblink);
	INIT_LIST(&inode->i_dentries);
	
	// ask the filesystem for the inode
	error = super->s_ops->read_inode(super, inode);
	
	// check if the filesystem had an error
	if( error != 0 )
	{
		kfree(inode);
		return ERR_PTR(error);
	}
	
	// add it to the superblock list
	list_add(&inode->i_sblink, &super->s_inode_list);
	
	return NULL;
}

/*
 * function: i_getref
 * description:
 * 		increments the reference count of the inode object.
 * parameters:
 * 		inode: the inode to get a reference of.
 * return values:
 * 		the inode which was passed in.
 * notes:
 * 		This is just for convenience in order that you can say
 * 		struct inode* myinoderef = i_getref(someinode);
 */
struct inode* i_getref(struct inode* inode)
{
	inode->i_ref++;
	return inode;
}

/*
 * function: i_free
 * description:
 * 		free the memory taken by a unreferenced inode
 * parameters:
 * 		inode: the inode to be free'd
 * return values:
 * 		none.
 * notes:
 * 		
 */
void i_free(struct inode* inode)
{
	if( inode->i_super->s_ops->put_inode ){
		inode->i_super->s_ops->put_inode(inode->i_super, inode);
	}
	list_rem(&inode->i_sblink);
	kfree(inode);
}

/*
 * function: i_put
 * description:
 * 		decrements the reference count of the inode.
 * 		if the inode is at a reference count of 1
 * 		upon entering the function, i_put will also
 * 		free the inode (using i_free).
 * parameters:
 * 		inode: pointer to the inode you no longer need
 * return value:
 * 		none.
 * notes:
 *
 */
void i_put(struct inode* inode)
{
	if( inode->i_ref == 1 ){
		i_free(inode);
		return;
	}
	inode->i_ref--;
}