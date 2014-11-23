#include "dentry.h"		/* dentry declerations */
#include "fs.h"			/* filesystem declerations */
#include "kmem.h"		/* kmalloc/kfree declerations */
#include "kernel.h"		/* basic kernel definitions */
#include "error.h"

/*
 * function: d_free
 * purpose:
 * 		to deallocate memory used by a dentry with no more references
 * parameters:
 * 		dentry: the directory entry to free
 * return value:
 * 		none.
 * notes:
 * 		should only be called if dentry->d_ref == 0
 */
void d_free(struct dentry* dentry)
{	
	list_rem(&dentry->d_fslink); // remove from the superblock list
	list_rem(&dentry->d_sibling); // remove from the d_parent children list
	// we should not have children, because refs should be zero
	// we should also not have a mountpoint because refs is zero
	
	// release references to the inode and parent
	if( dentry->d_inode ){
		i_put(dentry->d_inode);
	}
	d_put(dentry->d_parent);
	
	// free the dentry structure
	kfree(dentry);
}

/*
 * function: d_alloc
 * purpose:
 * 		allocate a new directory entry with the specified inode
 * 		the inode does not need to be valid (can be NULL), but
 * 		the name must be a valid name with no path seperators.
 * parameters:
 * 		name: a string representing the name of the node
 * 		parent: the parent of this dentry
 * return value:
 * 		the newly allocated directory entry with one reference.
 * notes:
 * 
 */
struct dentry* d_alloc(const char* name, struct dentry* parent)
{
	
	struct dentry* dentry = (struct dentry*)(kmalloc(sizeof(struct dentry)));
	if( !dentry ){
		return dentry;
	}
	
	memset(dentry, 0, sizeof(struct dentry));
	strncpy(dentry->d_name, name, MAXNAMELEN);
	
	dentry->d_parent = d_get(parent);
	dentry->d_ref = 1; // initialize the references to 1 (this reference/the one we return)
	
	INIT_LIST(&dentry->d_fslink);
	INIT_LIST(&dentry->d_sibling);
	INIT_LIST(&dentry->d_children);
	
	if( dentry->d_parent )
	{
		list_add(&dentry->d_sibling, &parent->d_children);
	}
	
	return dentry;
}

/*
 * function: d_alloc_root
 * purpose:
 * 		allocate a root directory entry for a filesystem from a given inode
 * 		the root directory entry has no parent dentry.
 * parameters:
 * 		inode: the inode to create the root directory entry from
 * return value:
 * 		a new directory entry pointing to the given inode
 * notes:
 * 		the name is unimportant for a root directory entry.
 */
struct dentry* d_alloc_root(struct inode* inode)
{
	struct dentry		*root = NULL;		// the newly allocated directory entry
	
	// create a parentless directory entry
	root = d_alloc("", NULL);
	// abort if we could not allocate the directory entry
	if( IS_ERR(root) ){
		return root;
	}
	
	root->d_inode = i_getref(inode);
	root->d_ino = inode->i_ino;
	root->d_uid = inode->i_uid;
	root->d_gid = inode->i_gid;
	root->d_size = inode->i_size;
	root->d_mode = inode->i_mode;
	
	return root;
}

/*
 * function: d_get
 * purpose:
 * 		increment the reference count of a directory entry.
 * parameters:
 * 		dentry: the directory entry to retrieve a reference of.
 * return value:
 * 		the function returns the same dentry that was passed to it.
 * notes:
 * 
 */
struct dentry* d_get(struct dentry* dentry)
{
	if(!dentry) return NULL;
	dentry->d_ref++;
	return dentry;
}

/*
 * function: d_put
 * purpose:
 * 		to decrement the reference count of a directory entry
 * 		and free the directory entry if the reference count
 * 		reaches zero.
 * parameters:
 * 		dentry: the directory entry to decrement
 * return value:
 * 		none.
 * notes:
 * 
 */
void d_put(struct dentry* dentry)
{
	if( dentry->d_ref == 1 ){
		d_free(dentry);
		return;
	}
	dentry->d_ref--;
}

/*
 * function: d_lookup
 * purpose:
 * 		lookup a child directory entry by name
 * parameters:
 * 		dir: the directory entry to search
 * 		name: the name of the child, containing no path seperators
 * return value:
 * 		the directory entry referred to by 'name' or an error value
 * 		retrieved by the PTR_ERR macro.
 * notes:
 * 
 */
struct dentry* d_lookup(struct dentry* dir, const char* name)
{
	struct dentry		*entry = NULL;			// The entry we find to return
	list_t			*iter = NULL;			// the iterator in the list of children
	int			 error = 0;			// error return value
	
	// check if the entry is already in memory
	list_for_each(iter, &dir->d_children)
	{
		entry = list_entry(iter, struct dentry, d_sibling);
		if( strcmp(name, entry->d_name) == 0 ){
			return d_get(entry);
		}
	}
	// the entry is not in memory, we need to read it from the inode
	
	// check if we have an inode
	if( !dir->d_inode ){
		return ERR_PTR(-ENOENT);
	}
	// check if the inode has a lookup function
	if( !dir->d_inode->i_ops->lookup ){
		return ERR_PTR(-ENOENT);
	}
	
	// allocate a directory entry in the right place with the right name
	entry = d_alloc(name, dir);
	// check if the allocation went okay
	if( IS_ERR(entry) ){
		return entry;
	}
	
	// attempt read the inode from the filesystem
	error = dir->d_inode->i_ops->lookup(dir->d_inode, entry);
	
	// check for an error while reading
	if( error != 0 ){
		d_put(entry);
		return ERR_PTR(error);
	}
	
	return entry;
}