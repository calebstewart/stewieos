#include "fs.h"			// Basic filesystem definitions
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

// global vfs variables
static list_t			 vfs_filesystem_list = LIST_INIT(vfs_filesystem_list);		// A list of filesystem structures (filesystem types)
static list_t			 vfs_mount_list = LIST_INIT(vfs_mount_list);			// A list of mounts to check a device against currently
static struct dentry		*vfs_root = NULL;						// root directory entry for the entire filesystem

// internal function prototypes
void i_free(struct inode* inode); // free an inode with no more references

/* function: path_put
 * purpose:
 * 	release references to the contained dentry and mount stuctures.
 * parameters:
 * 	path - the path to release
 * return value:
 * 	none.
 * notes:
 * 	There is no path_get, because there is no reference counting for
 * 	path structures themselves. You should copy the contents, not
 * 	the pointer to it. You shouldn't leave path pointers laying around
 * 	that you didn't create yourself.
 * 
 * 	There is a path_copy function to copy the contents with correct
 * 	reference counting.
 */
void path_put(struct path* path)
{
	d_put(path->p_dentry);
	mnt_put(path->p_mount);
}

/* function: path_copy
 * purpose:
 * 	copy the contents of a path structure, and keep reference counts.
 * parameters:
 * 	dst - the path structure to fill
 * 	src - the path structure to copy
 * return value:
 * 	none.
 */
void path_copy(struct path* dst, struct path* src)
{
	dst->p_dentry = d_get(src->p_dentry);
	dst->p_mount = mnt_get(src->p_mount);
}

/* function: mnt_get
 * purpose:
 * 	increase the reference count of the mount structure
 * 	You cannot unmount a mount while its mount strucutre
 * 	has outstanding references.
 * 
 * 	You should not use a mount structure until you have
 * 	a reference to it, and you should always call mnt_put
 * 	when you are done.
 * parameters:
 * 	mount - the mount to grab a reference of
 * return value:
 * 	A pointer to the newly retrieved mount structure.
 */
struct mount* mnt_get(struct mount* mount)
{
	if( !mount ) return NULL;
	mount->m_refs++;
	return mount;
}

/* function: mnt_put
 * purpose:
 * 	decrease the reference count of the mount structure.
 * 
 * 	See also 'mnt_get'
 * parameters:
 * 	mount - the mount structure to release
 * return value:
 * 	none.
 */
void mnt_put(struct mount* mount)
{
	if( !mount ) return;
	if( mount->m_refs == 0 ){
		syslog(KERN_WARN, "mnt_put: mount reference count is going negative...\n");
	}
	mount->m_refs--;
}

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
	vfs_root = d_alloc("", NULL);
	if( IS_ERR(vfs_root) ){
		printk("%2Vvfs: error: unable to allocate root directory entry!\n");
	}
	
	register_filesystem(&testfs_type);
	
	// This block will test a REALLY simple vfs, but I know it works,
	// and the output is just annoying now...
	
	/*
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

void copy_task_vfs(struct vfs* d, struct vfs* s)
{
	memset(d, 0, sizeof(struct vfs));
	path_copy(&d->v_cwd, &s->v_cwd);
// 	for(int i = 0; i < 1024; ++i){
// 		if( s->v_openvect[i].flags & FD_OCCUPIED ){
// 			d->v_openvect[i].flags = s->v_openvect[i].flags;
// 			d->v_openvect[i].file = s->v_openvect[i].file;
// 		}
// 	}
}

void init_task_vfs(struct vfs* vfs)
{
	memset(vfs, 0, sizeof(struct vfs));
	vfs->v_cwd.p_dentry = d_get(vfs_root);
	vfs->v_cwd.p_mount = NULL;
}

void free_task_vfs(struct vfs* vfs)
{
	path_put(&vfs->v_cwd);
	for(int i = 0; i < 1024; ++i){
		if( vfs->v_openvect[i].flags & FD_OCCUPIED ){
			file_close(vfs->v_openvect[i].file);
			vfs->v_openvect[i].flags = 0;
		}
	}
}

int path_lookup(const char* name, int flags, struct path* path)
{
	UNUSED(flags);
	char			query[512];		// the internal path array
	char			*iter = query;
	char			*slash;
	struct dentry		*child;
	int			error = 0;
	
	if( name == NULL ){
		return -ENOENT;
	}
	
	if( strlen(name) > 511 ){
		return -ENAMETOOLONG;
	}
	
	// copy the user path to the internal path variable
	strncpy(query, name, 512);
	query[511] = 0;
	
	iter = query;
	
	if( iter[0] == '/' ){
		path->p_dentry = d_get(vfs_root);
		path->p_mount = NULL;
		iter++;
	} else {
		path_copy(path, &current->t_vfs.v_cwd);
	}
	
	while( 1 )
	{
		// this is a mountpoint or a mount (either way, grab to topmost mount point for this position)
		if( path->p_dentry->d_mountpoint ){
			struct mount* mount = list_entry(list_first(&path->p_dentry->d_mountpoint->mp_mounts), struct mount, m_mplink);
			if( mount->m_super->s_root != path->p_dentry ) {
				path_put(path);
				path->p_dentry = d_get(mount->m_super->s_root);
				path->p_mount = mnt_get(mount);
			}
		}
		
		// Check for ".", "..", "./*", "../*"
		if( *iter == '.' ){
			// Current directory, that's redundant
			if( *(iter+1) == '/' ){
				iter += 2;
			} else if( *(iter+1) == '.' ){
				// This will make path reference its parent (event through mountpoints)
				error = path_get_parent(path);
				if( error != 0 ){
					path_put(path);
					return error;
				}
				// If that was the last character, we can just return.
				if( *(iter+2) == '\0' ){
					break;
				}
			} else if( *(iter+1) == '\0' ){
				// They just wanted the current directory entry
				break;
			}
		} else if( *iter == 0 ){
			// The query was empty: Same as "./"
			break;
		}
		
		// do we have search permissions?
		if( path_access(path, X_OK) != 0 ){
			path_put(path);
			return -EACCES;
		}
		
		// find the path deliminator
		slash = strchr(iter, '/');
		
		// If there is no slash, then just grab the entry
		// and return
		if( slash == NULL )
		{
			if( *iter == 0 ){
				break;
			}
			// We only need the parent anyway
			if( flags & WP_PARENT ) {
				return 0;
			}
			struct dentry* dentry = path->p_dentry;
			path->p_dentry = d_lookup(path->p_dentry, iter); // there's no slash, so just lookup the next item
			d_put(dentry);
			if( IS_ERR(path->p_dentry) ){
				mnt_put(path->p_mount);
				return PTR_ERR(path->p_dentry);
			} else { 
				break;
			}
		}
		
		// lookup the new root of the search
		*slash = 0; // remove the slash
		child = d_lookup(path->p_dentry, iter); // iter is now a c string containing the next element
		*slash = '/'; // replace the slash
		if( IS_ERR(child) ){ // is this an error?
			path_put(path);
			path->p_dentry = NULL;
			return PTR_ERR(child);
		}
		
		iter = slash+1;
		
		// Reuse the mount
		d_put(path->p_dentry);
		path->p_dentry = child;
	}
	
	if( flags & WP_PARENT ){
		error = path_get_parent(path);
		if( error != 0 ){
			path_put(path);
			return error;
		}
	}
	
	return 0;
}

int path_get_parent(struct path* path)
{
	if( path->p_dentry->d_parent ){
		struct dentry* parent = d_get(path->p_dentry->d_parent); // get the parent
		d_put(path->p_dentry); // free the current
		path->p_dentry = parent; // save the parent
		return 0;
	} else {
		struct path parent; // temporary path structure
		// grab a reference so it doens't get deleted on path_put(path)
		path_copy(&parent, &path->p_dentry->d_mountpoint->mp_point);
		// free this path
		path_put(path);
		// put it in the path pointer
		path_copy(path, &parent);
		// free the temporary path
		path_put(&parent);
		return 0;
	}
}

/* function: sys_dup
 * parameters:
 * 		newfd: the new file descriptor (or -1 to find an empty one)
 * 		oldfd: file descriptor to copy
 * 		flags: O_CLOEXEC flag if needed in new file descriptor
 * return value:
 * 		On success, this function returns the new descriptor.
 * 		On error, a negative error value is returned.
 */
// int sys_dup(int newfd, int oldfd, int flags)
// {
// 	if( !FD_VALID(oldfd) ){
// 		return -EBADF;
// 	}
// 	if( newfd == -1 )
// 	{
// 		for(newfd = 0; newfd < FS_MAX_OPEN_FILES; ++newfd){
// 			if( FD_INVALID(newfd) ){
// 				current->t_vfs.v_openvect[newfd].flags |= FD_OCCUPIED | FD_INVALID;
// 				break;
// 			}
// 		}
// 		if( newfd == FS_MAX_OPEN_FILES ){
// 			return -EMFILE;
// 		}
// 	} else {
// 		// outside the range, but not the special -1 value
// 		if( !FD_VALID_RANGE(newfd) ){
// 			return -EBADF;
// 		}
// 	}
// 	if( oldfd == newfd ){
// 		return -EINVAL;
// 	}
// 	
// 	if( flags != 0 && flags != O_CLOEXEC ){
// 		return -EINVAL;
// 	}
// 	
// 	
// 	return newfd;
// }

/*
 * function: sys_mount
 * parameters:
 * 		source_name: name of the source file (what to mount at target, if needed)
 * 		target_name: name of the target directory (where to mount the source)
 * 		filesystemtype: name of the filesystem registered with the kernel (e.g. ext2)
 * 		mountflags: MS_* values OR'd together, see sys/mount.h
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
	struct path		source,			// pointer to the source file (should be a device file)
				target;			// pointer to the target file (should be a directory)
	struct dentry		*root = NULL;		// What we are mounting to target
	struct filesystem	*filesystem = NULL;	// pointer to the file system type
	int			 error = 0;		// error codes returned by various functions
	dev_t			 device = 0;		// the device to send to the driver
	list_t			*iter;			// iterator for search the mount list
	
	// Shut the compiler up until we implement the walk_path function
	//UNUSED(target_name);
	//UNUSED(source_name);
	//UNUSED(filesystemtype);
	// get the directory entries for the source and target
	error = path_lookup(target_name, WP_DEFAULT, &target);
	if(error != 0){
		return error;
	}
	error = path_lookup(source_name, WP_DEFAULT, &source);
	// find the filesystem pointer
	filesystem = get_filesystem(filesystemtype);
	
	// is the filesystem valid?
	if( IS_ERR(filesystem) ){
		path_put(&target);
		if( error != 0 ) path_put(&source);
		return PTR_ERR(filesystem);
	}
	// is source invalid, and does the filesystem need a source?
	if( error != 0 && !(filesystem->fs_flags & FS_NODEV)){
		path_put(&target);
		return error;
	}
	// grab the device or release the source if needed
	if( !(filesystem->fs_flags & FS_NODEV) ){
		device = source.p_dentry->d_inode->i_dev;
		path_put(&source);
	} else if( error == 0 ){
		path_put(&source);
	}
	
	// make sure source isn't already mounted (or this filesystem isn't already mounted, if device is unneeded)
	list_for_each(iter, &vfs_mount_list){
		struct mount* item = list_entry(iter, struct mount, m_globlink); // grab the entry from the list
		// Is there a device to look for?
		if( device == 0 )
		{
			// does this mount also not require a device and use the same file system? BUSY!
			if( item->m_super->s_dev == 0 && item->m_super->s_fs == filesystem ){
				path_put(&target); // release the target
				return -EBUSY; // tell the caller that this filesystem is busy
			}
		// We are using a device. Does this mount use the same device? BUSY!
		} else if( item->m_super->s_dev == device ){
			path_put(&target); // release the target
			return -EBUSY; // tell the caller that this device is already used
		}
	}
	
	// don't allow mounting a read only filesystem as read/write
	if( !(mountflags & MS_RDONLY) && (filesystem->fs_flags & FS_RDONLY) ){
		path_put(&target);
		return -EACCES;
	}
	
	// allocate the super block
	struct superblock* super = (struct superblock*)kmalloc(sizeof(struct superblock));
	if(!super){
		path_put(&target);
		return -ENOMEM;
	}
	memset(super, 0, sizeof(struct superblock));
	
	INIT_LIST(&super->s_inode_list);
	INIT_LIST(&super->s_dentry_list);
	
	// read the superblock
	super->s_fs = filesystem;
	error = filesystem->fs_ops->read_super(filesystem, super, device, mountflags, (void*)data);
	
	// bad source/unable to load! :(
	if( error < 0 ){
		path_put(&target);
		kfree(super);
		return error;
	}
	
	root = super->s_root;
	
	// allocate a new mointpoint structure if needed
	if( !target.p_dentry->d_mountpoint )
	{
		target.p_dentry->d_mountpoint = (struct mountpoint*)(kmalloc(sizeof(struct mountpoint)));
		if(!target.p_dentry->d_mountpoint){
			path_put(&target);
			filesystem_put_super(filesystem, super);
			kfree(super);
			return -ENOMEM;
		}
		memset(target.p_dentry->d_mountpoint, 0, sizeof(struct mountpoint));
		path_copy(&target.p_dentry->d_mountpoint->mp_point, &target);
		INIT_LIST(&target.p_dentry->d_mountpoint->mp_mounts);
	}
	
	// link the root dentry to the mountpoint structure
	root->d_mountpoint = target.p_dentry->d_mountpoint;
	
	// create a new mount
	struct mount* mount = (struct mount*)(kmalloc(sizeof(struct mount)));
	if( !mount ){
		path_put(&target);
		filesystem_put_super(filesystem, super);
		kfree(super);
		return -ENOMEM;
	}
	mount->m_super = super;
	mount->m_flags = mountflags;
	mount->m_data  = data;
	mount->m_point = root->d_mountpoint;
	mount->m_refs = 1; // Initialize to 1 reference
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
 * 		flags: MS_* values OR'd together; see sys/mount.h or man 2 umount
 * return value:
 * 		An integer indicating success or failure. A zero return value
 * 		indicates success, while a negative number indicates error and
 * 		represents a negated errno.h error value.
 * notes:
 * 		See `man 2 umount' for more information
 */
int sys_umount(const char* target_name, int flags)
{
	struct path		target; // the target directory entry
	struct mount		*mount = NULL; // the mount for this target
	struct mountpoint	*mountpoint = NULL; // mountpoint
	struct superblock	*super = NULL; // the superblock that is mounted
	int			result = 0;
	UNUSED(flags);
	
	result = path_lookup(target_name, WP_DEFAULT, &target);
	
	// Was there a problem looking up the target?
	if( result != 0 ){
		return result;
	}
	
	// This dentry should be the root of the mounted filesystem
	if( target.p_dentry != target.p_mount->m_super->s_root )
	{
		path_put(&target);
		return -EINVAL;
	}
	
	// Grab the mount structure, and the superblock for reference
	mount = target.p_mount;
	super = mount->m_super;
	mountpoint = mount->m_point;
	
	// Release the path
	path_put(&target);
	
	// If it is not in use, m_refs should be 1 (the filesystem's reference)
	// This means that nothing is open, and it is simply still mounted
	if( mount->m_refs != 1 )
	{
		return -EBUSY;
	}
	
	// The superblock still has the root node open, so there should be one reference
	if( super->s_refs != 1 ){
		return -EBUSY;
	}
	
	if( super->s_fs->fs_ops->put_super )
	{
		result = super->s_fs->fs_ops->put_super(super->s_fs, super);
		if( result != 0 ){
			return result;
		}
	}
	
	// Remove the mount from its lists
	list_rem(&mount->m_mplink);
	list_rem(&mount->m_globlink);
	
	// No more mounts at this mountpoint. Destroy it.
	if( list_empty(&mountpoint->mp_mounts) )
	{
		
		mountpoint->mp_point.p_dentry->d_mountpoint = NULL;
		path_put(&mountpoint->mp_point);
		kfree(mountpoint);
	}
	
	// Free the mount and superblock
	kfree(mount);
	kfree(super);
	
	// We're done!
	return 0;
}

/* function: create_file
 * purpose:
 * 	Create a new file. filename indicates the full path name
 * 	of the new file.
 * 
 * 	Assumptions:
 * 		- The file is already garunteed to NOT EXIST.
 * 		- The file name is within the maximum length
 * parameters:
 * 	filename - the full path name of the new file
 * 	mode - the file permission modes of the file
 * 	path - if non-null, fill in the path structure after creating the file
 * return value:
 * 	Zero on success, or a negative error value for failure.
 */
int create_file(const char* filename, mode_t mode, struct path* filepath)
{
	// Grab the path, and the basename form the filename
	char path_buf[512];
	strcpy(path_buf, filename);
	char* path = path_buf;
	char* name = basename(path);
	if( name != path ) *(name-1) = 0;
	else path = (char*)"/";
	
	// Look up the containing directory
	struct path dir;
	int result = path_lookup(path, WP_DEFAULT, &dir);
	if( result != 0 ){
		return result;
	}
	
	// Check for write permissions on the parent directory
	result = path_access(&dir, W_OK);
	if( result != 0 ){
		path_put(&dir);
		return result;
	}
	
	// Check if the driver supports the creat function
	if( !dir.p_dentry->d_inode->i_ops->creat ){
		path_put(&dir);
		return -EACCES;
	}
	
	// Tell the filesystem to create the new file
	result = dir.p_dentry->d_inode->i_ops->creat(dir.p_dentry->d_inode,\
							name, mode,\
							filepath);
	
	// Free the containing directory
	path_put(&dir);
	
	// Return the result from the filesystem
	return result;
}

int sys_mknod(const char* path, mode_t mode, dev_t dev)
{
	UNUSED(path);
	UNUSED(mode);
	UNUSED(dev);

	struct path		exist_test;	
	struct path		parent;
	int			error;
	
	// Check if the name already exists
	error = path_lookup(path, WP_DEFAULT, &exist_test);
	if( error == 0 ){
		path_put(&exist_test);
		return -EEXIST;
	}
	
	// Lookup the parent directory
	error = path_lookup(path, WP_PARENT, &parent);
	if( error != 0 ){
		return error;
	}
	
	// Make sure we have write permissions
	error = path_access(&parent, W_OK);
	if( error != 0 ){
		path_put(&parent);
		return error;
	}
	
	if( !parent.p_dentry->d_inode->i_ops->mknod ){
		path_put(&parent);
		return -EPERM;
	}
	
	// All the nitty-gritty is done by the filesystem driver.
	error = parent.p_dentry->d_inode->i_ops->mknod(parent.p_dentry->d_inode, basename(path), mode, dev);
	
	// Release the parent path
	path_put(&parent);
	
	// Return any possible error (or zero on success)
	return error;
}

int inode_trunc(struct inode* inode)
{
	if( !inode->i_ops->truncate ){
		return -EACCES;
	}
	
	return inode->i_ops->truncate(inode);
}

int sys_resfd( void )
{
	for(int i = 0; i < TASK_MAX_OPEN_FILES; i++){
		if( !(current->t_vfs.v_openvect[i].flags & FD_OCCUPIED) ){
			// Reserve the file descriptor
			current->t_vfs.v_openvect[i].flags = FD_OCCUPIED | FD_INVALID;
			return i;
		}
	}
	return -1;
}

void sys_relfd( int fd )
{
	current->t_vfs.v_openvect[fd].flags = 0;
}

int sys_open(const char* filename, int flags, mode_t mode)
{
	struct path path;
	int	fd = -1,
		result = 0;
	
	// Find a free file descriptor
	for(int i = 0; i < TASK_MAX_OPEN_FILES; i++){
		if( !(current->t_vfs.v_openvect[i].flags & FD_OCCUPIED) ){
			fd = i;
			// Reserve the file descriptor
			current->t_vfs.v_openvect[i].flags = FD_OCCUPIED | FD_INVALID;
			break;
		}
	}
	// No free file descriptors
	if( fd == -1 ) return -EMFILE;
	
	// Locate the file in the directory tree
	result = path_lookup(filename, WP_DEFAULT, &path);
	if( result != 0 ){
		// Create the file if needed
		if( flags & O_CREAT )
		{
			// create a new regular file
			result = sys_mknod(filename, (mode & ~(S_IFMT)) | S_IFREG, 0);
			//result = create_file(filename, (mode & S_IFMT) | S_IFREG, &path);
			if( result != 0 ){
				current->t_vfs.v_openvect[fd].flags = 0;
				return result;
			}
			// Lookup the new file
			result = path_lookup(filename, WP_DEFAULT, &path);
			// This shouldn't happen, since we supposedly just successfully created the file
			if( result != 0 ){
				current->t_vfs.v_openvect[fd].flags = 0;
				return result;
			}
		} else { // the file doesn't exist and we don't want to create it
			return result;
		}
	// we want a new file, not an existing one
	} else if( flags & O_EXCL ){
		path_put(&path);
		return -EEXIST;
	}
	
	// Open the path as a file, and release our path reference
	current->t_vfs.v_openvect[fd].file = file_open(&path, flags);
	path_put(&path);
	
	// There was an error opening the file
	if( IS_ERR(current->t_vfs.v_openvect[fd].file) ){
		result = PTR_ERR(current->t_vfs.v_openvect[fd].file);
		current->t_vfs.v_openvect[fd].file = NULL;
		current->t_vfs.v_openvect[fd].flags = 0; // not occupied anymore
		return result;
	}
	
	// Remove the invalid flag, the file descriptor is ready
	current->t_vfs.v_openvect[fd].flags &= ~FD_INVALID;
	
	return fd;
}

/* function: sys_close
 * purpose:
 * 	Close an open file descriptor.
 * parameters:
 * 	fd - an open file descriptor
 * return values:
 * 	Zero on success or a negative error on failure.
 */
int sys_close(int fd)
{
	if( !FD_VALID(fd) )
		return -EBADF;
	
	struct file* file = current->t_vfs.v_openvect[fd].file;
	int error = file_close(file);
	
	if( error != 0 ){
		return error;
	}
		
	current->t_vfs.v_openvect[fd].file = NULL;
	current->t_vfs.v_openvect[fd].flags = 0;
	
	return 0;
}

/* function: sys_read
 * purpose:
 * 	read data from an open file descriptor
 * parameters:
 * 	fd - the open file descriptor
 * 	buf - the buffer to read into
 * 	count - the number of characters to read
 * return value:
 * 	the number of characters read from the file,
 * 	or a negative error value.
 */
ssize_t sys_read(int fd, void* buf, size_t count)
{
	if( !FD_VALID(fd) ){
		return -EBADF;
	}
	
	struct file* file = current->t_vfs.v_openvect[fd].file;
	
	return file_read(file, buf, count);
}

/* function: sys_write
 * purpose:
 * 	write data to an open file descriptor
 * parameters:
 * 	fd - the open file descriptor
 * 	buf - the buffer to write to the file
 * 	count - the number of bytes to write
 * return value:
 * 	the number of bytes actually written
 * 	or a negative error value
 */
ssize_t sys_write(int fd, const void* buf, size_t count)
{
	if( !FD_VALID(fd) ){
		return -EBADF;
	}
	
	struct file* file = current->t_vfs.v_openvect[fd].file;
	
	return file_write(file, buf, count);
}

/* function: sys_readdir
 * purpose:
 * 	read the next 'count' directory entries from the file
 * 	descriptor.
 * parameters:
 * 	fd - the open file descriptor
 * 	dirent - an array of directory entries at least 'count' long
 * 	count - the number of directory entries to read
 * return value:
 * 	the number of directory entries read on success or a negative
 * 	error value on failure.
 */
int sys_readdir(int fd, struct dirent* dirent, size_t count)
{
	if( !FD_VALID(fd) ){
		return -EBADF;
	}
	
	struct file* file = current->t_vfs.v_openvect[fd].file;
	
	return file_readdir(file, dirent, count);
}

/* function sys_lseek
 * purpose:
 * 	change the file position of an open file descriptor
 * parameters:
 * 	fd - the open file descriptor
 * 	offset - an offset to seek to
 * 	whence - the base of the offset (SEEK_SET, SEEK_END, SEEK_CUR)
 * return value:
 * 	the new offset from the beginning of the file
 */
off_t sys_lseek(int fd, off_t offset, int whence)
{
	if( !FD_VALID(fd) ){
		return (off_t)-EBADF;
	}
	
	struct file* file = current->t_vfs.v_openvect[fd].file;
	
	return file_seek(file, offset, whence);
}

int sys_dup(int old_fd)
{
	if( !FD_VALID(old_fd) ){
		return -EBADF;
	}
	
	struct file* file = current->t_vfs.v_openvect[old_fd].file;
	
	int new_fd = 0;
	for(; new_fd < FS_MAX_OPEN_FILES; new_fd++){
		if( !(current->t_vfs.v_openvect[new_fd].flags & FD_OCCUPIED) ){
			current->t_vfs.v_openvect[new_fd].flags = FD_OCCUPIED | FD_INVALID;
			break;
		}
	}
	if( new_fd == FS_MAX_OPEN_FILES ){
		return -EMFILE;
	}
	
	// Copy the file description
	file->f_refs++;
	current->t_vfs.v_openvect[new_fd].file = file;
	current->t_vfs.v_openvect[new_fd].flags = FD_OCCUPIED;
	
	// Return the new file descriptor
	return new_fd;
}

int sys_link(const char* old_path, const char* new_path)
{
	struct path oldp, newp;
	int result = 0;
	char new_parent[512];
	
	strcpy(new_parent, new_path);
	// basename just returns the character after the last slash
	char* new_base = basename(new_parent);
	// Make the slash a null character character a terminating character
	*(new_base-1) = 0;
	// new_parent is now the path to the parent, and new_base
	// is the basename
	
	// Lookup the old item
	result = path_lookup(old_path, WP_DEFAULT, &oldp);
	if( result != 0 ){
		return result;
	}
	// Lookup the new item's parent
	result = path_lookup(new_parent, WP_DEFAULT, &newp);
	if( result != 0 ){
		return result;
	}
	
	if( oldp.p_mount != newp.p_mount )
	{
		path_put(&oldp);
		path_put(&newp);
		return -EXDEV;
	}
	
	if( newp.p_mount->m_flags & MS_RDONLY ){
		path_put(&oldp);
		path_put(&newp);
		return -EROFS;
	}
	
	if( !newp.p_dentry->d_inode->i_ops->link ) {
		path_put(&oldp);
		path_put(&newp);
		return -EPERM;
	}
	
	result = newp.p_dentry->d_inode->i_ops->link(newp.p_dentry->d_inode, new_base, oldp.p_dentry->d_inode);
	
	path_put(&newp);
	path_put(&oldp);
	
	return result;
}

int sys_fstat(int fd, struct stat* st)
{
	if( !FD_VALID(fd) ){
		return -EBADF;
	}
	
	struct file* file = current->t_vfs.v_openvect[fd].file;
	
	return file_stat(file, st);
}

/* function: sys_isatty
 * parameters:
 * 	fd: the file descriptor to test
 * return value: 
 * 	Returns 1 for a valid tty, and a negative error value otherwise.
 * 	Valid errors are:
 * 		EBADF for a bad file descriptor
 * 		EINVAL or ENOTTY for a file descriptor which is not a tty
 */
int sys_isatty(int fd)
{
	if( !FD_VALID(fd) ){
		return -EBADF;
	}
	struct file* file = current->t_vfs.v_openvect[fd].file;
	return file_isatty(file);
}

int path_access(struct path* path, int mode)
{
	if( mode != F_OK && (mode & ~(X_OK|W_OK|R_OK)) != 0 ){
		return -EINVAL;
	}
	
	// The file obviously exists... we already parsed the path...
	if( mode == F_OK ) return 0;
	
	// Why this happens, no one knows...
	if( path->p_dentry == vfs_root && path->p_mount == NULL ){
		return 0;
	}
	
	struct inode* inode = path->p_dentry->d_inode;
	
	// we are root!
	if( current->t_uid == 0 )
	{
		// Even root can't write to a Read Only mounted filesystem...
		if( (path->p_mount) && (path->p_mount->m_flags & MS_RDONLY) && (mode & W_OK) ){
			return -EROFS;
		}
		
		// We also can't execute non-executables...
		if( (mode & X_OK) && !(inode->i_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ){
			return -EACCES;
		}
		
		// Otherwise, we are golden!
		return 0;
	}
	if( mode & W_OK )
	{
		if( (path->p_mount) && path->p_mount->m_flags & MS_RDONLY ){
			return -EACCES;
		}
		// Check the appropriate write bit
		if( current->t_uid == inode->i_uid ){
			if( !(inode->i_mode & S_IWUSR) )
				return -EACCES;
		} else if( current->t_gid == inode->i_gid ){
			if( !(inode->i_mode & S_IWGRP) )
				return -EACCES;
		} else {
			if( !(inode->i_mode & S_IWOTH) )
				return -EACCES;
		}
	}
	if( mode & R_OK )
	{
		if( current->t_uid == inode->i_uid ){
			if( !(inode->i_mode & S_IRUSR) )
				return -EACCES;
		} else if( current->t_gid == inode->i_gid ){
			if( !(inode->i_mode & S_IRGRP) )
				return -EACCES;
		} else {
			if( !(inode->i_mode & S_IROTH) )
				return -EACCES;
		}
	}
	if( mode & X_OK )
	{
		if( (path->p_mount) && (path->p_mount->m_flags & MS_NOEXEC) ){
			return -EACCES;
		}
		if( current->t_uid == inode->i_uid ){
			if( !(inode->i_mode & S_IXUSR) )
				return -EACCES;
		} else if( current->t_gid == inode->i_gid ){
			if( !(inode->i_mode & S_IXGRP) )
				return -EACCES;
		} else {
			if( !(inode->i_mode & S_IXOTH) )
				return -EACCES;
		}
	}
	
	return 0;
}

int sys_access(const char* file, int mode)
{
	struct path path;
	int result = 0;
	
	result = path_lookup(file, WP_DEFAULT, &path);
	if( result != 0 ){
		return result;
	}
	
	result = path_access(&path, mode);
	
	path_put(&path);
	
	return result;
}

int sys_chmod(const char* file, mode_t mode)
{
	struct path path;
	int result = 0;
	
	result = path_lookup(file, WP_DEFAULT, &path);
	
	if( result != 0 ){
		return result;
	}
	
	if( path.p_dentry->d_inode->i_ops->chmod ){
		result = path.p_dentry->d_inode->i_ops->chmod(path.p_dentry->d_inode, mode);
	} else {
		result = 0;
		// set only the modes we are aloud to set, and leave the file type/format bits
		path.p_dentry->d_inode->i_mode &= S_IFMT;
		path.p_dentry->d_inode->i_mode |= (mode & (mode_t)~S_IFMT);
	}

	path_put(&path);
	
	return result;
}

int sys_chown(const char* file, uid_t owner, gid_t group)
{
	struct path path;
	int result = 0;
	
	if( owner != (uid_t)-1 && current->t_uid != 0 ){
		return -EPERM;
	}
	
	result = path_lookup(file, WP_DEFAULT, &path);
	
	if( result != 0 ){
		return result;
	}
	
	if( path.p_dentry->d_inode->i_ops->chown ){
		result = path.p_dentry->d_inode->i_ops->chown(path.p_dentry->d_inode, owner, group);
	} else {
		result = 0;
		// set only the modes we are aloud to set, and leave the file type/format bits
		if( owner != (uid_t)-1 ) path.p_dentry->d_inode->i_uid = owner;
		if( group != (gid_t)-1 ) path.p_dentry->d_inode->i_gid = group;
	}

	path_put(&path);
	
	return result;
}

mode_t sys_umask(mode_t mask)
{
	mode_t old = current->t_umask;
	current->t_umask = mask;
	return old;
}

/* function: basename
 * purpose:
 * 	retrieve the last component in a file path.
 * 	e.g. /x/y/z/somefile.txt would return somefile.txt
 * 	trailing slashes are significant, so /x/y/z/ will
 * 	return an empty string.
 * parameters:
 * 	path - the full name or path of the file/directory
 * return value:
 * 	the base name of the file (last component).
 */
char* basename(const char* path)
{
	char* slash = NULL,
		*tmp = NULL;
	tmp = (char*)path - 1;
	while( tmp ){
		slash = tmp;
		tmp = strchr(slash+1, '/');
	}
	// slash is a pointer to the last slash, add one
	return slash+1;
}

/* function: sys_ioctl
 * purpose:
 * 	adjust device parameters of an open file descriptor
 * parameters:
 * 	fd - the open file descriptor
 * 	request - the request/command for the device
 * 	argp - device/request specific arguments
 * return value:
 * 	Normally zero on success or a negative error value.
 * 	But the real result depends on the device driver and
 * 	request/command.
 */
int sys_ioctl(int fd, int request, char* argp)
{
	struct file* file = current->t_vfs.v_openvect[fd].file;
	
	if( !file ){
		return -EBADF;
	}
	
	return file_ioctl(file, request, argp);
}

int sys_chdir(const char* name)
{
	struct path path;
	int result;
	
	result = path_lookup(name, WP_DEFAULT, &path);
	if( result < 0 ){
		return result;
	}
	
	if( !S_ISDIR(path.p_dentry->d_inode->i_mode) ){
		path_put(&path);
		return -ENOTDIR;
	}
	
	path_put(&current->t_vfs.v_cwd);
	path_copy(&current->t_vfs.v_cwd, &path);
	
	return 0;
}

// recursive function to build the path string of a dentry
int _build_path(int dotdot __attribute__((unused)), char** buf, char* end, struct dentry *dentry);
int _build_path(int dotdot __attribute__((unused)), char** buf, char* end, struct dentry *dentry)
{
	if( dentry->d_parent ){
		int result;
		if( strcmp(dentry->d_name, "..") == 0 ){
			result = _build_path(1, buf, end, dentry->d_parent);
		} else {
			result = _build_path(0, buf, end, dentry->d_parent);
		}
		if( result < 0 ){
			return result;
		}
		// don't print the "."
		if( strcmp(dentry->d_name, ".") == 0 || dotdot == 1 || strcmp(dentry->d_name,"..") == 0 ){
			return 0;
		}
	}
	// Is there room for this name?
	if( (size_t)(end-*buf) < strlen(dentry->d_name) ){
		return -ENAMETOOLONG;
	}
	
	// Copy the name and increment the iterator
	strcpy(*buf, dentry->d_name);
	*buf += strlen(dentry->d_name);
	
	if( dentry->d_inode == NULL || S_ISDIR(dentry->d_inode->i_mode) ){
		if( (size_t)(end-*buf) == 0 ){
			return -ENAMETOOLONG;
		}
		*(*buf) = '/';
		(*buf)++;
	}
	
	return 0;
}

int sys_getcwd(char* buf, size_t len)
{
	struct dentry* dentry = current->t_vfs.v_cwd.p_dentry;
	
	int result = _build_path(0, &buf, &buf[len-2], dentry);
	if( result == 0 ){
		*buf = 0;
	}
	
	return result;
}

/* function: super_get
 * purpose:
 * 	increase the reference count of an open superblock
 * parameters:
 * 	super - the superblock
 * return value:
 * 	The superblock with an increased reference count
 */
struct superblock* super_get(struct superblock* super)
{
	super->s_refs++;
	return super;
}

/* function: super_put
 * purpose:
 * 	decrease the reference count of an open superblock
 * parameters:
 * 	super - the superblock
 * return value:
 * 	none.
 */
void super_put(struct superblock* super)
{
	if( super->s_refs == 0 ){
		syslog(KERN_WARN, "super_put: superblock reference count going negative.\n");
	}
	super->s_refs--;
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
	list_t			*iter = NULL;
	
	// check if it is already loaded
	list_for_each(iter, &super->s_inode_list)
	{
		inode = list_entry(iter, struct inode, i_sblink);
		if( inode->i_ino == ino ){
			return i_getref(inode);
		}
	}
	
	// allocate the inode structure and clear its contents
	inode = (struct inode*)kmalloc(sizeof(struct inode));
	memset(inode, 0, sizeof(struct inode));
	
	// fill in the information we know
	inode->i_ino = ino;
	inode->i_super = super_get(super);
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
	super_put(inode->i_super);
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