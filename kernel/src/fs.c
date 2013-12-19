#include "fs.h"			// Basic filesystem definitions
#include "dentry.h"		// directory entry structure and function definitions
#include "sys/mount.h"		// sys_mount/umount flags and the like
#include "error.h"		// Error constants
#include "kmem.h"		// kmalloc/kfree
#include "testfs.h"

// global vfs variables
static list_t			 vfs_filesystem_list = LIST_INIT(vfs_filesystem_list);		// A list of filesystem structures (filesystem types)
static list_t			 vfs_mount_list = LIST_INIT(vfs_mount_list);			// A list of mounts to check a device against currently
static struct dentry		*vfs_root = NULL;						// root directory entry for the entire filesystem

// internal function prototypes
void i_free(struct inode* inode); // free an inode with no more references
static struct dentry* walk_path_r(char* path, struct dentry** root, u32 flags);

/*
 * function: register_filesystem
 * purpose:
 * 		register a filesystem type with the virtual file system layer.
 * parameters:
 * 		fs: the filesystem structure to add to the system
 * return value:
 * 		the function returns zero on success or some negative error
 * 		value on failure.
 * notes:
 * 		Should there be a return value? Should there be a check
 * 		for multiple filesystem types with the same name?
 */
int register_filesystem(struct filesystem* fs)
{
	// make sure this is initialized, drivers don't usually bother with it
	INIT_LIST(&fs->fs_fslink);
	// add it to the list
	list_add(&fs->fs_fslink, &vfs_filesystem_list);
	// that's it...
	return 0;
}

/*
 * function: unregister_filesystem
 * purpose:
 * 		disconnects a filesystem driver with the virtual file system layer
 * parameters:
 * 		fs: the filesystem to disconnect
 * return value:
 * 		a negative integer error value or zero on success.
 * notes:
 * 
 */
int unregister_filesystem(struct filesystem* fs)
{
	// make sure it isn't being used!
	if( !list_empty(&fs->fs_slist) ){
		return -EBUSY;
	}
	// remove from the list
	list_rem(&fs->fs_fslink);
	// that's it...
	return 0;
}

/*
 * function: get_filesystem
 * purpose:
 * 		finds a filesystem structure by its name
 * parameters:
 * 		id: the name of the filesystem (according to its fs_name field)
 * return value:
 * 		the pointer to the registered filesystem type or you may check and
 * 		retrieve the error with IS_ERR and PTR_ERR respectively.
 * notes:
 * 
 */
struct filesystem* get_filesystem(const char* id)
{
	list_t* iter = NULL;
	list_for_each(iter, &vfs_filesystem_list){
		struct filesystem* entry = list_entry(iter, struct filesystem, fs_fslink);
		if( strcmp(entry->fs_name, id) == 0 ){
			return entry;
		}
	}
	return ERR_PTR(-ENODEV);
}

/*
 * function: initialize_filesystem
 * description:
 * 		sets up the initial vfs_root and registers a simple in memory filesystem type
 * parameters:
 * 		none.
 * return value:
 * 		none.
 * notes:
 * 
 */
void initialize_filesystem( void )
{
	vfs_root = d_alloc("/", NULL);
	if( IS_ERR(vfs_root) ){
		printk("%2Vvfs: error: unable to allocate root directory entry!\n");
	}
	
	register_filesystem(&testfs_type);
	
	// This block will test a REALLY simple, but I know it works,
	// and the output is just annoying now...
	/*
	printk("vfs: mounting testfs to \"/\"...\n");
	int result = sys_mount("", "/", "testfs", MF_RDONLY, NULL);
	printk("vfs: mounting result: %i\n", result);
	
	const char* file_path = "/stupid/somefile.txt";
	
	printk("vfs: attempting to walk the path \"%s\"...\n", file_path);
	struct dentry* dentry = walk_path(file_path, WP_DEFAULT);
	printk("vfs: walk_path result: %p\n", dentry);
	if( IS_ERR(dentry) ){
		printk("vfs: result is an error!\nvfs: error value: %i\n", PTR_ERR(dentry));
	} else {
		printk("vfs: releasing the dentry reference...\n");
		d_put(dentry);
	}
	*/
	
}

/*
 * function: walk_path_r
 * description:
 * 		walks the path from parent through path and returns the dentry there
 * 		WP_NOLINK will prevent walk_path_r from following symbolic links.
 * parameters:
 * 		path: a string representing the path to the directory entry
 * 		root: a pointer to the pointer to a dentry to start the search from
 * 			a double pointer is used so that root can be reassigned from
 * 			mounts and the references still get counted correctly.
 * 		flags: WP_* flags OR'd together
 * return value:
 * 		a dentry representing the path passed into the function. If the path could
 * 		not be followed, then an error value is returned and can be retrieved with
 * 		the PTR_ERR macro.
 * notes:
 * 		the path must not start with a '/'. This is an error. "./" is fine, because
 * 		"." is interpreted as "parent", and the search will continue. In the same
 * 		way, "../" is also fine and is interpreted as parent->d_parent.
 * 
 * 		walk_path_r is called recursively in order to continue following the path.
 */
static struct dentry* walk_path_r(char* path, struct dentry** root, u32 flags)
{
	char			*slash = NULL;		// The pointer to the slash character from strchr
	struct dentry		*child = NULL;		// the next entry we found
	
	// this is a mountpoint or a mount (either way, grab to topmost mount point for this position)
	if( (*root)->d_mountpoint ){
		struct mount* mount = list_entry(list_first(&(*root)->d_mountpoint->mp_mounts), struct mount, m_mplink);
		d_put(*root);
		*root = d_get(mount->m_super->s_root);
	}
	
	// Check for ".", "..", "./*", "../*"
	if( *path == '.' ){
		if( *(path+1) == '/' ){
			path += 2;
		} else if( *(path+1) == '.' ){
			if( *(path+2) == '/' ){
				if( (*root)->d_parent == NULL ){
					return ERR_PTR(-ENOENT);
				}
				path += 3;
				struct dentry* newroot = (*root)->d_parent;
				d_put(*root);
				*root = newroot;
			} else if( *(path+2) == '\0' ){
				if( (*root)->d_parent == NULL ){
					return ERR_PTR(-ENOENT);
				}
				return d_get((*root)->d_parent); // return another reference to the parent
			}
		} else if( *(path+1) == 0 ){
			return d_get(*root);
		}
	}	

	if( *path == 0 ){
		return d_get(*root);
	}
	
	// find the path deliminator
	slash = strchr(path, '/');
	
	if( slash == NULL )
	{
		child = d_lookup(*root, path); // there's no slash, so just lookup the next item
		return child;
	}
	
	// lookup the new root of the search
	*slash = 0; // remove the slash
	child = d_lookup(*root, path); // path now points only to this entry
	*slash = '/'; // replace the slash
	if( IS_ERR(child) ){ // is this an error?
		return child; // return the error
	}
	struct dentry* temp = walk_path_r(slash+1, &child, flags); // grab the next..next..next, so on...
	d_put(child); // release our reference to the child entry
	return temp;
	
}

/*
 * function: walk_path
 * description:
 * 		decides whether to use the current path (held in the process structure) or
 * 		the root path (e.g. from vfs_root) then calls walk_path_r
 * parameters:
 * 		path: a string representing the path to the directory entry
 * 		flags: WP_* flags OR'd together
 * return value:
 * 		a dentry representing the path passed into the function. If the path could
 * 		not be followed, then an error value is returned and can be retrieved with
 * 		the PTR_ERR macro.
 * notes:
 * 		vfs_root is used as the current directory if '/' is the current character.
 * 		otherwise, current_task->cwd is used.
 */
struct dentry* walk_path(const char* upath, u32 flags)
{
	char			 path[512] = {0},		// the internal path array
				*pPath = path;			// the path pointer
	struct dentry		*root = NULL,			// where to start the search
				*result = NULL;			// result from the path walk
	
	// copy the user path to the internal path variable
	strncpy(path, upath, 512);
	
	if( *pPath == '/' ){
		root = d_get(vfs_root);
		pPath++;
	} else {
		//root = task_get_cwd(task_current());
		root = d_get(vfs_root);
	}
	
	result = walk_path_r(pPath, &root, flags);
	d_put(root);
	
	return result;
}
	


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
	
	// Shut the compiler up until we implement the walk_path function
	//UNUSED(target_name);
	//UNUSED(source_name);
	//UNUSED(filesystemtype);
	// get the directory entries for the source and target
	target = walk_path(target_name, WP_DEFAULT);
	source = walk_path(source_name, WP_DEFAULT);
	// find the filesystem pointer
	filesystem = get_filesystem(filesystemtype);
	
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
	
	//UNUSED(target_name);
	target = walk_path(target_name, WP_DEFAULT);
	
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
	
	return inode;
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