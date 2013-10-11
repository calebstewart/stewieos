#include "dentry.h"		/* dentry declerations */
#include "fs.h"			/* filesystem declerations */
#include "kmem.h"		/* kmalloc/kfree declerations */
#include "kernel.h"		/* basic kernel definitions */

void d_free(struct dentry* dentry)
{	
	list_rem(&dentry->d_fslink); // remove from the supernode list
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

struct dentry* d_alloc(const char* name, struct inode* inode)
{
	
	struct dentry* dentry = (struct dentry*)(kmalloc(sizeof(struct dentry)));
	if( !dentry ){
		return dentry;
	}
	
	memset(dentry, 0, sizeof(struct dentry));
	strncpy(dentry->d_name, name, MAXNAMELEN);
	
	dentry->d_inode = i_getref(inode);
	dentry->d_uid = inode->i_uid;
	dentry->d_gid = inode->i_gid;
	dentry->d_size = inode->i_size;
	dentry->d_mode = inode->i_mode;
	dentry->d_ino = inode->i_ino;
	dentry->d_ref = 1; // initialize the references to 1 (this reference/the one we return)
	
	INIT_LIST(&dentry->d_fslink);
	INIT_LIST(&dentry->d_sibling);
	INIT_LIST(&dentry->d_children);
	
	
	return dentry;
}

void d_put(struct dentry* dentry)
{
	if( dentry->d_ref == 1 ){
		d_free(dentry);
		return;
	}
	dentry->d_ref--;
}