#ifndef _FS_H_
#define _FS_H_

#include "kernel.h"
#include <sys/stat.h>
// NOTE i need to implement this is in the standard library!
//#include <dirent.h>
#include "linkedlist.h"

// Filesystem flags
#define FS_NODEV		0x00000001
#define FS_RDONLY		0x00000002

#define filesystem_put_super(fs, super)		do{ if( (fs)->fs_ops->put_super ) (fs)->fs_ops->put_super((fs),super); } while(0)

// Forward declerations for structures
struct inode; 				// forward decl
struct file;				// forward decl
struct dentry;				// see dentry.h
struct dirent;				// until I implement dirent.h...
struct filesystem;			// forward decl
struct superblock;			// forward decl

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
	int(*creat)(struct inode*, const char*, mode_t);
	int(*mknod)(struct inode*, const char*, mode_t, dev_t);
	int(*mkdir)(struct inode*, const char*, mode_t);
	// Deleting self
	int(*unlink)(struct inode*);
};

/* type: struct file_operations
 * purpose:
 * 	holds the filesystem functions for a file
 */
struct file_operations
{
	int(*open)(struct file*, struct dentry*, int);
	int(*close)(struct file*, struct dentry*);
	size_t(*read)(struct file*, char*, size_t);
	size_t(*write)(struct file*, const char*, size_t);
	int(*readdir)(struct file*, struct dirent*);
};

/* type: struct filesystem_operations
 * purpose:
 * 	holds the driver specific functions for a filesystem
 */
struct filesystem_operations
{
	int(*read_super)(struct filesystem*,struct superblock*, dev_t, unsigned long);
	void(*put_super)(struct filesystem*, struct superblock*);
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
	size_t				i_size;			// Size of (regular) the file in bytes
	time_t				i_atime,		// seconds representing the last time this inode was accessed
					i_ctime,		// seconds representing when the inode was created
					i_mtime,		// seconds representing when the inode was modified
					i_dtime;		// seconds representing when the inode was deleted
	gid_t				i_gid;			// POSIX group having access to this file
	u16				i_nlinks;		// Number of time this inode is linked (referred to).
	ino_t				i_ino;			// Inode number
	dev_t				i_dev;			// If this is a device file, the device id
	// Filesystem data
	size_t				i_ref;			// Reference counting
	struct superblock*		i_super;		// pointer to the superblock this inode belongs to
	list_t				i_sblink;		// Link in the superblocks inode list
	list_t				i_dentries;		// List of dentries linked to this inode
	struct inode_operations*	i_ops;			// The list of operations functions for this inode
	struct file_operations*		i_default_fops;		// Default file operations for this inode
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
	struct dentry*			mp_point;		// Where these directory entries are mounted
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
	struct dentry*			f_dentry;		// The directory entry this file is connected to
	struct file_operations*		f_ops;			// The operations structure (either from the device type list or f_dentry->d_inode->i_default_fops)
	off_t				f_off;			// The offset into the file to read/write
	int				f_flags;		// The current flags (including open flags and states)
	struct file_lock*		f_lock;			// A file lock (NULL if unlocked)
};

int register_filesystem(struct filesystem* fs);
int unregister_filesystem(struct filesystem* fs);

struct inode*	i_getref(struct inode* inode);			// get a reference to an inode
struct inode*	i_get(struct superblock* super, ino_t ino);	// get an inode from the superblock
void		i_put(struct inode* inode);			// release a reference to an inode (possible free)

int sys_mount(const char* source, const char* target,
	      const char* filesystemtype, unsigned long mountflags, const void* data);
int sys_umount(const char* target, int flags);

#endif