#ifndef _DENTRY_H_
#define _DENTRY_H_

#include "stewieos/kernel.h"
#include "stewieos/linkedlist.h"
#include <sys/stat.h>

struct filesystem;
struct inode;
struct mountpoint;

/* type: struct dentry
 * purpose:
 * 		encapsulates an entry in the file system (of
 * 	any type) It holds some common information from the
 * 	inode, a pointer to the inode itself and the string
 * 	name of the entry.
 * 	
 * 		There may be multiple dentry's pointing to one
 * 	inode (e.g. sym/hard links)
 */
struct dentry
{
	// Entry information
	char				d_name[MAXNAMELEN];			// The name given to this entry
	struct inode*			d_inode;				// The inode for this entry
	ino_t				d_ino;					// The inode number for this entry
	
	// Some useful things from the inode that we might want to
	// grab without the inode itself being loaded (might remove)
	uid_t				d_uid;					// The user id for this inode
	gid_t				d_gid;					// The POSIX group id for this inode
	size_t				d_size;					// The size of the inode data in bytes
	mode_t				d_mode;					// The mode of the inode (e.g. file type and permissions)
	
	// List related items (lists we are a part of or lists we maintain)
	size_t				d_ref;					// Reference count, so we know when to delete it
	struct dentry*			d_parent;				// the parent directory entry of this entry
	list_t				d_fslink;				// Link in the filesystem's dentry list
	list_t				d_sibling;				// link in the inodes dentry list
	list_t				d_children;				// list of already loaded children (if this is a directory)
	struct mountpoint*		d_mountpoint;				// mountpoint information (if mounted)
};

struct dentry*		d_alloc(const char* name, struct dentry* parent);	// allocate a new dentry pointing to inode with the name 'name'
struct dentry*		d_alloc_root(struct inode* root);			// allocate a new dentry pointing to the root node 'root'
void			d_free(struct dentry* dentry);				// free the dentry and associated resources (DO NOT CALL, internal use only)

struct dentry*		d_lookup(struct dentry* dir, const char* name);		// Look up a directory entry within a directory by name
struct dentry*		d_get(struct dentry* dentry);				// increase the reference count for this dentry
void			d_put(struct dentry* dentry);				// decrease the reference count for this dentry

#endif
