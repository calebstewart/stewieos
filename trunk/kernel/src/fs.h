#ifndef _FS_H_
#define _FS_H_

#include "kernel.h"
#include <sys/stat.h>
// NOTE i need to implement this is in the standard library!
//#include <dirent.h>
#include "linkedlist.h"
#include <dirent.h>

// Filesystem flags
#define FS_NODEV		0x00000001
#define FS_RDONLY		0x00000002

// File Descriptor Flags
// The file descriptor is taken
#define FD_OCCUPIED		(1<<16)
// The file descriptor is invalid at the moment,
// but is still occupied (if FD_OCCUPIED)
#define FD_INVALID		(1<<17)

#define FS_MAX_OPEN_FILES TASK_MAX_OPEN_FILES

#define FD_VALID(fd) ( (fd) >= 0 && (fd) < FS_MAX_OPEN_FILES && (current->t_vfs.v_openvect[(fd)].flags & FD_OCCUPIED) && !(current->t_vfs.v_openvect[(fd)].flags & FD_INVALID) )

// walk_path flags
#define WP_DEFAULT		0x00000000
#define WP_PARENT		0x00000001

#define filesystem_put_super(fs, super)		do{ if( (fs)->fs_ops->put_super ) (fs)->fs_ops->put_super((fs),super); } while(0)

#define file_inode(filp) ((filp)->f_path.p_dentry->d_inode)
#define file_dentry(filp) ((filep)->f_path.p_dentry)

// Forward declerations for structures
struct inode; 				// forward decl
struct file;				// forward decl
struct dentry;				// see dentry.h
struct dirent;				// until I implement dirent.h...
struct filesystem;			// forward decl
struct superblock;			// forward decl
struct path;				// forward decl

/* type: struct inode_operations
 * purpose:
 * 	holds the filesystem functions for an inode
 * 	an inode is like a header for the file, it is
 * 	usually unnoticed, except when it is a directory.
 * 	Many of these functions operate on directories
 */
struct inode_operations
{
	// Working with children
	int(*lookup)(struct inode*, struct dentry*);
	// Creating children (only for inodes representing a directory)
	int(*creat)(struct inode*, const char*, mode_t, struct path* dentry);
	int(*mknod)(struct inode*, const char*, mode_t, dev_t);
	int(*mkdir)(struct inode*, const char*, mode_t);
	int(*link)(struct inode*, const char*, struct inode*);
	// Deleting self
	int(*unlink)(struct inode*);
	
	int(*chmod)(struct inode*, mode_t);
	int(*chown)(struct inode*, uid_t, gid_t);
	int(*truncate)(struct inode*);
};

/* type: struct file_operations
 * purpose:
 * 	holds the filesystem functions for a file
 */
struct file_operations
{
	int(*open)(struct file*, struct dentry*, int);
	int(*close)(struct file*, struct dentry*);
	ssize_t(*read)(struct file*, char*, size_t);
	ssize_t(*write)(struct file*, const char*, size_t);
	int(*readdir)(struct file*, struct dirent*, size_t);
	off_t(*lseek)(struct file*, off_t, int);
	int(*ioctl)(struct file*, int, char*);
	int(*fstat)(struct file*, struct stat*);
};

/* type: struct filesystem_operations
 * purpose:
 * 	holds the driver specific functions for a filesystem
 */
struct filesystem_operations
{
	int(*read_super)(struct filesystem*,struct superblock*, dev_t, unsigned long, void*);
	int(*put_super)(struct filesystem*, struct superblock*);
};

/* type: struct superblock_operations
 * purpose:
 * 	holds the driver specific superblock
 * 	functions for a filesystem.
 */
struct superblock_operations
{
	/* 
	 * function: superblock:read_inode
	 * description:
	 * 		Should read in the specified inode into the given inode
	 * 		structure. If the inode number is invalid or does not
	 * 		exist, an error should be returned (e.g. ENOENT).
	 * parameters:
	 * 		superblock*: pointer to the superblock the inode belongs to
	 * 		inode*: pointer to the inode it should fill in
	 * return value:
	 * 		a negative errno.h constant or zero on success.
	 * notes:
	 * 		the inode number to read should already be assigned to inode->i_ino
	 * 		upon calling this function. Also, i_super, i_ref, i_sblink, and
	 * 		i_dentries should already be initialized.
	 * 
	 * 		Upon error, the inode should be returned in the same state
	 * 		as it was passed into the function (e.g. no allocations
	 * 		and it should not be in ANY lists as it will not exist
	 * 		in a moment).
	 */
	int(*read_inode)(struct superblock*,struct inode*);
	/*
	 * function: put_node
	 * description:
	 * 		Optional. This function is called when the inode is
	 * 		being free'd (e.g. i_put called with i_ref==1). This
	 * 		is just a notification of it being free'd, there is
	 * 		no required action.
	 * paramters:
	 * 		superblock*: pointer to the referenced superblock
	 * 		inode*: the inode which is about to be free'd
	 * return value:
	 * 		none
	 */
	void(*put_inode)(struct superblock*, struct inode*);
};

/* type: struct filesystem
 * purpose:
 * 	encapsulates a file system type and its operations
 * 	created by the driver, and registered with
 * 	register_filesystem.
 */
struct filesystem
{
	// filesystem information
	char 				fs_name[32];		// short name used with sys_mount
	unsigned long 			fs_flags;		// flags for the file system
	// internal system data
	struct filesystem_operations*	fs_ops;			// file system operations (supplied by the driver)
	list_t				fs_slist;		// list of superblocks associated with the file system
	list_t				fs_fslink;		// link in the list of filesystem structures
	
};

/* type: struct superblock
 * purpose:
 * 	holds data about a file system and functions for accessing it
 * 	it is read in using the filesystem->f_ops->read_super function
 */
struct superblock
{
	// file system data read from the disk
	time_t				s_mtime;		// last mount time in Unix time
	time_t				s_wtime;		// last write access to the file system in Unix time
	size_t				s_blocksize;		// The block size in bytes
	size_t				s_inode_count;		// The total number of inodes in the file system
	size_t				s_block_count;		// The total number of blocks in the file system
	size_t				s_free_block_count;	// The number of free blocks in the file system
	size_t				s_free_inode_count;	// The number of free inodes in the file system
	size_t				s_magic;		// Magic number identifying the file system (needed here?)
	// driver data
	dev_t				s_dev;			// device file this superblock was read from (if any, or 0 if no device is needed)
	void*				s_private;		// private data for the filesystem driver
	// record keeping and system data
	u32				s_refs;			// Reference counter for the superblock
	struct filesystem*		s_fs;			// The file system this superblock belongs to
	struct dentry*			s_root;			// The root directory entry (usually ino=0)
	list_t				s_inode_list;		// list of inodes associated with this superblock
	list_t				s_dentry_list;		// list of directory entries associated with this superblock (or, with its inodes)
	struct superblock_operations*	s_ops;			// superblock operations implemented by the filesystem driver
};

/* type: struct inode
 * purpose: 
 * 	encapsulates a node in the file system.
 * 	It is modeled after the ext2 inode
 * 	structure.
 */
struct inode
{
	// Inode data
	mode_t				i_mode;			// Indicates format of the file and its access rights
	uid_t				i_uid;			// User ID associated with this file
	off_t				i_size;			// Size of (regular) the file in bytes
	time_t				i_atime,		// seconds representing the last time this inode was accessed
					i_ctime,		// seconds representing when the inode was created
					i_mtime,		// seconds representing when the inode was modified
					i_dtime;		// seconds representing when the inode was deleted
	gid_t				i_gid;			// POSIX group having access to this file
	u16				i_nlinks;		// Number of time this inode is linked (referred to).
	ino_t				i_ino;			// Inode number
	dev_t				i_dev;			// If this is a device file, the device id
	// Filesystem data
	void*				i_private;		// Private FS Data
	size_t				i_ref;			// Reference counting
	struct superblock*		i_super;		// pointer to the superblock this inode belongs to
	list_t				i_sblink;		// Link in the superblocks inode list
	list_t				i_dentries;		// List of dentries linked to this inode
	struct inode_operations*	i_ops;			// The list of operations functions for this inode
	struct file_operations*		i_default_fops;		// Default file operations for this inode
};

/* type: struct path
 * purpose:
 * 	Holds the path information for a dentry.
 * 	This includes the mount point and the
 * 	dentry itself.
 */
struct path
{
	struct dentry*			p_dentry;		// The dentry pointer
	struct mount*			p_mount;		// The mount point
};

/* type: struct mount
 * purpose:
 * 	holds information on a mount target including
 * 	its mount flags, and the dentry that is mounted
 * 	there and a pointer back to the mountpoint
 * 	structure
 */
struct mount
{
	struct superblock*		m_super;		// what is being mounted
	unsigned long			m_flags;		// the flags for this mount
	const void*			m_data;			// filesystem specific options passed to mount
	struct mountpoint*		m_point;		// the mount structure we belong to
	u32				m_refs;			// Reference Counter
	list_t				m_mplink;		// the link in the mount point list
	list_t				m_globlink;		// global mount list link
};
	

/* type: struct mountpoint
 * purpose:
 * 	holds information on a mountpoint
 * 	pointed to by directory entries which are
 * 	mount points themselves or have been mounted.
 */
struct mountpoint
{
	struct path			mp_point;		// Where these directory entries are mounted
	list_t				mp_mounts;		// A list of directory entries that are mounted here
								// m_mounts->prev is the last dentry to be mounted,
								// and therefore represents what the user sees.
};

/* type: struct file
 * purpose:
 * 	connects a directory entry to an open file
 * 	it contains the flags, current file pointer
 * 	and file operations structures.
 */
struct file
{
	//struct dentry*			f_dentry;		// The directory entry this file is connected to
	struct path			f_path;			// The path to the file we have open
	struct file_operations*		f_ops;			// The operations structure (either from the device type list or f_dentry->d_inode->i_default_fops)
	off_t				f_off;			// The offset into the file to read/write
	int				f_status;		// The current flags (including open flags and states)
	int				f_refs;			// Number of file references
	struct file_lock*		f_lock;			// A file lock (NULL if unlocked)
	void*				f_private;		// Private driver data
};

/* type: struct file_descr
 * purpose:
 * 	file descriptor structure
 */
struct file_descr
{
	struct file* file;
	int flags;
};

/* type: struct vfs
 * purpose:
 * 	encapsulates the virtual file system state
 * 	of a process including current working
 * 	directory and open files array.
 */
struct vfs
{
	struct path v_cwd;					// Current Working Directory
	struct file_descr v_openvect[1024];			// The open file vector
};

void initialize_filesystem( void );				// setup the filesystem for boot

int register_filesystem(struct filesystem* fs);
int unregister_filesystem(struct filesystem* fs);

struct filesystem* get_filesystem(const char* id);		// get a filesystem pointer from its name

struct inode*	i_getref(struct inode* inode);			// get a reference to an inode
struct inode*	i_get(struct superblock* super, ino_t ino);	// get an inode from the superblock
void		i_put(struct inode* inode);			// release a reference to an inode (possible free)

char* basename(const char * path);

int path_lookup(const char* name, int flags,\
		struct path* path);				// Lookup an entry in the file system

struct mount* mnt_get(struct mount* mount);
void mnt_put(struct mount* mount);

struct superblock* super_get(struct superblock* super);
void super_put(struct superblock* super);

// Truncate an inode
int inode_trunc(struct inode* inode);
// Create a new file and optionally return its path
int create_file(const char* name, mode_t mode, struct path* path);

// exactly the same as sys_access but takes a path structure
int path_access(struct path* path, int mode);
// Release references to containing structures
// There is no path_get, because there is no reference
// counting for path. You should not keep path pointers
// laying around that you did not create yourself. You
// can copy their contents, though.
void path_put(struct path* path);
void path_copy(struct path* dst, struct path* src);
// 'steps' the path up one directory. If the return value is zero,
// the path now points to its parent directory (event through mount
// points).
int path_get_parent(struct path* path);

struct file* file_open(struct path* path, int flags);
int file_close(struct file* file);
ssize_t file_read(struct file* file, void* buf, size_t count);
ssize_t file_write(struct file* file, const void* buf, size_t count);
off_t file_seek(struct file* file, off_t offsets, int whence);
int file_ioctl(struct file* file, int request, char* argp);
int file_stat(struct file* file, struct stat* buf);
int file_readdir(struct file* file, struct dirent* dirent, size_t count);

/* Unix System Call Definitions
 * 
 * For more information about a sys_* routine, see the syscall manual entry
 * with "man 2 routine_name" where routine name is the function without
 * "sys_".
 */
int sys_mount(const char* source, const char* target,
	      const char* filesystemtype, unsigned long mountflags, const void* data);
int sys_umount(const char* target, int flags);
int sys_open(const char* filename, int flags, mode_t mode);
int sys_close(int fd);
ssize_t sys_read(int fd, void* buf, size_t count);
ssize_t sys_write(int fd, const void* buf, size_t count);
off_t sys_lseek(int fd, off_t offset, int whence);
int sys_ioctl(int fd, int request, char* argp);
int sys_creat(const char* pathname, mode_t mode);
int sys_dup(int oldfd);
int sys_link(const char* oldpath, const char* newpath);
int sys_fstat(int fd, struct stat* buf);
int sys_access(const char* file, int mode);
int sys_chmod(const char* file, mode_t mode);
int sys_chown(const char* path, uid_t owner, gid_t group);
mode_t sys_umask(mode_t mask);
int sys_mknod(const char* path, mode_t mode, dev_t dev);
int sys_readdir(int fd, struct dirent* dirent, size_t count);

void copy_task_vfs(struct vfs* dest, struct vfs* src);
void init_task_vfs(struct vfs* vfs);

#endif