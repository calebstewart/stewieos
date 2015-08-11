#include "error.h"
#include "ext2fs/ext2.h"
#include "kmem.h"
#include "block.h"
#include "dentry.h"
#include "task.h"
#include "cmos.h"

/* Read an inode from the disk */
int e2_read_inode(struct superblock* sb, struct inode* inode)
{
	e2_inode_private_t* priv;
	e2_super_private_t* e2fs = EXT2_SUPER(sb);
	u32 bg = 0;
	u32 local = 0;
	u32 location = 0;
	ssize_t result = 0;
	
	// Check if this is a valid inode number
	if( inode->i_ino == 0 || inode->i_ino == EXT2_BAD_INO || inode->i_ino > sb->s_inode_count ){
		return -ENOENT;
	}
	
	// Allocate private data space
	priv = (e2_inode_private_t*)kmalloc(sizeof(*priv) + e2fs->super.s_inode_size);
	if( !priv ){
		return -ENOMEM;
	}
	
	// Clear the memory
	memset(priv, 0, sizeof(*priv));
	
	priv->inode = (e2_inode_t*)priv->inode_data; // convenience
	
	// Allocate the internal read/write buffer
	priv->rw = (char*)kmalloc(sb->s_blocksize);
	if( !priv->rw ){
		kfree(priv);
		return -ENOMEM;
	}
	
	// Initialize the lock
	spin_init(&priv->rw_lock);
	
	// calculate the block group number and local inode number
	bg = (inode->i_ino-1) / e2fs->super.s_inodes_per_group;
	local = (inode->i_ino-1) % e2fs->super.s_inodes_per_group;
	
	// Calculate the inode location
	location = e2fs->bgtable[bg].bg_inode_table*sb->s_blocksize + local * e2fs->super.s_inode_size;
	
	// Read the inode from disk
	result = block_read(sb->s_dev, location, e2fs->super.s_inode_size, priv->inode_data);
	if( result < 0 ){
		kfree(priv);
		return (int)result;
	}
	
	priv->block = (u32*)kmalloc(4 * priv->inode->i_blocks);
	if( !priv->block && priv->inode->i_blocks ){
		kfree(priv);
		return -ENOMEM;
	}
	
	// Copy some important information
	inode->i_mode = priv->inode->i_mode;
	inode->i_uid = priv->inode->i_uid;
	inode->i_size = priv->inode->i_size;
	inode->i_atime = priv->inode->i_atime;
	inode->i_ctime = priv->inode->i_ctime;
	inode->i_mtime = priv->inode->i_mtime;
	inode->i_dtime = priv->inode->i_dtime;
	inode->i_gid = priv->inode->i_gid;
	inode->i_nlinks = priv->inode->i_links_count;
	if( S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode) ){
		inode->i_dev = (dev_t)priv->inode->i_block[0];
	} else {
		inode->i_dev = 0;
	}
	
	// Setup the inode book-keeping data
	inode->i_private = priv;
	inode->i_ops = &e2_inode_operations;
	inode->i_default_fops = &e2_default_file_operations;
	
	e2_inode_read_block_map(inode, priv->block);
	
	// Success
	return 0;
}

/* Release private inode data */
void e2_put_inode(struct superblock* sb ATTR((unused)), struct inode* inode)
{
	e2_inode_private_t* priv = EXT2_INODE(inode);
	
	if( priv->inode->i_links_count == 0 ){
		e2_free_inode(sb, inode);
	} else {
		e2_inode_flush(inode);
	}
	
	kfree(priv->block);
	kfree(priv->rw);
	kfree(priv);
	inode->i_private = NULL;
} 

/* Look for a directory entry within this inode with the specified name */
int e2_inode_lookup(struct inode* inode, struct dentry* dentry)
{
	e2_inode_private_t* priv = EXT2_INODE(inode);
	struct superblock* sb = inode->i_super;
	u32 nblocks = (priv->inode->i_blocks*512)/sb->s_blocksize;
	
	spin_lock(&priv->rw_lock);
	
	for(u32 i = 0; i < nblocks; ++i)
	{
		e2_inode_io(inode, EXT2_READ, i*sb->s_blocksize, sb->s_blocksize, priv->rw);
		e2_dirent_t* iter = (e2_dirent_t*)priv->rw;
		while( ((u32)iter) < ((u32)priv->rw + sb->s_blocksize) )
		{
			if( iter->inode != 0 && strlen(dentry->d_name) == iter->name_len && strncmp(dentry->d_name, iter->name, iter->name_len) == 0 )
			{
				dentry->d_inode = i_get(inode->i_super, (ino_t)iter->inode);
				if( !IS_ERR(dentry->d_inode) ){
					dentry->d_ino = (ino_t)iter->inode;
					spin_unlock(&priv->rw_lock);
					return 0;
				} else { // this means the filesystem if FUCKED up... -_-
					spin_unlock(&priv->rw_lock);
					dentry->d_inode = NULL;
					return -EIO;
				}
			}
			// This was happening for some reason...
			if( iter->rec_len == 0 ){
				break;
			}
			iter = (e2_dirent_t*)( (u32)iter + iter->rec_len );
		}
	}
	
	spin_unlock(&priv->rw_lock);
	
	return -ENOENT;
}

int e2_inode_mknod(struct inode* parent, const char* name, mode_t mode, dev_t dev)
{
	e2_inode_t data;
	
	// Fill in the initial internal inode structure
	memset(&data, 0, sizeof(data));
	data.i_mode = (u16)mode;
	data.i_uid = (u16)current->t_uid;
	data.i_gid = (u16)current->t_gid;
	if( S_ISBLK(mode) || S_ISCHR(mode) ){
		data.i_block[0] = dev;
	}
	
	// Allocate a new inode on disk
	ino_t ino = e2_alloc_inode(parent->i_super, &data);
	
	// No more inodes or no more free blocks... :(
	if( ino == EXT2_BAD_INO ){
		return -ENOSPC;
	}
	
	// Grab a reference to inode
	struct inode* inode = i_get(parent->i_super, ino);
	
	// Link the new inode to a dirent in the given directory
	int result = e2_inode_link(parent, name, inode);
	if( result < 0 ){
		// we couldn't create the name, so cleanup after ourselves
		e2_free_inode(inode->i_super, inode);
		return result;
	}
	
	e2_inode_flush(parent);
	e2_inode_flush(inode);
	
	// release our reference to the inode
	i_put(inode);
	
	return 0;
}

int e2_inode_flush(struct inode* inode)
{
	e2_inode_private_t* priv = EXT2_INODE(inode);
	struct superblock* sb = inode->i_super;
	e2_super_private_t* e2fs = EXT2_SUPER(inode->i_super);
	ssize_t result;

	if( !priv->dirty ) return 0;
	
	// flush the new block map to the disk
	e2_inode_write_block_map(inode, priv->block);
	
	// Flush the superblock first
	result = (ssize_t)e2_super_flush(sb);
	if( result < 0 ){
		return (int)result;
	}

	priv->dirty =0;
	
	// Calculate related indices for location
	u32 bg = (inode->i_ino-1) / e2fs->super.s_inodes_per_group;
	u32 local = (inode->i_ino-1) % e2fs->super.s_inodes_per_group;
	// use the block group table to find the inode address
	u32 location = e2fs->bgtable[bg].bg_inode_table*sb->s_blocksize + local*e2fs->super.s_inode_size;
	// Attemp to write the inode back to disk
	result = block_write(sb->s_dev, location, e2fs->super.s_inode_size, priv->inode_data);
	// Check for failure
	if( result < 0 ){
		return (int)result;
	}
	
	return 0;
}

ino_t e2_alloc_inode(struct superblock* sb, e2_inode_t* data)
{
	e2_super_private_t* e2fs = EXT2_SUPER(sb);
	u32 bg = 0;
	int idx = 0;
	int bit = 0;
	
	if( e2fs->super.s_free_inodes_count == 0 ){
		return EXT2_BAD_INO;
	}
	
	// Calculate the size of the bitmap
	size_t bitmap_size = e2fs->super.s_inodes_per_group / 8;
	if( e2fs->super.s_inodes_per_group % 8 ) bitmap_size++;
	
	spin_lock(&e2fs->rw_lock);
	
	// look for a block group with free inodes
	for(bg = 0; bg < EXT2_BGCOUNT(sb); ++bg){
		if( e2fs->bgtable[bg].bg_free_inodes_count != 0 ) break;
	}
	
	// this shouldn't happen if the filesystem is undamaged
	if( bg == EXT2_BGCOUNT(sb) ){
		debug_message("corrupt filesystem on device 0x%X. invalid free inode counts.\n", sb->s_dev);
		spin_unlock(&e2fs->rw_lock);
		return EXT2_BAD_INO;
	}
	
	// Allocate space for the inode bitmap
	u8* bitmap = (u8*)kmalloc(sb->s_blocksize);
	if( bitmap == 0 ){
		spin_unlock(&e2fs->rw_lock);
		return EXT2_BAD_INO;
	}
	
	// Read inode bitmap from disk
	e2_read_block(sb, e2fs->bgtable[bg].bg_inode_bitmap, 1, (char*)bitmap);
// 	ssize_t io_res = block_read(sb->s_dev, e2fs->bgtable[bg].bg_inode_bitmap*sb->s_blocksize, bitmap_size, (char*)bitmap);
// 	if( io_res < 0 ){
// 		kfree(bitmap);
// 		spin_unlock(&e2fs->rw_lock);
// 		return EXT2_BAD_INO;
// 	}
	
	// Iterate over each by in the bitmap
	for(idx = 0; idx < (int)(bitmap_size); ++idx)
	{
		// discard full bytes
		if( bitmap[idx] == 0xFF ) continue;
		// iterate over each bit
		for(bit = 0; bit < 8; ++bit){
			if( !(bitmap[idx] & (1<<bit)) ){ // break early for empty bit
				break;
			}
		}
		if( bit != 8 ) break; // if we broke early in the previous loop, break early here
	}
	
	// This shouldn't happen. This is a sign of a corrupt file system (bad inode counts)
	if( idx == ((int)(bitmap_size)) ){
		kfree(bitmap);
		spin_unlock(&e2fs->rw_lock);
		debug_message("corrupt filesystem on device 0x%X. invalid free inode counts.", sb->s_dev);
		return EXT2_BAD_INO;
	}
	
	ino_t ino = (ino_t)(bg*e2fs->super.s_inodes_per_group + idx*8 + bit + 1);
	
	// Allocate the inode in the bitmap, and sync it with the disk
	bitmap[idx] = (u8)(bitmap[idx] | (1<<bit));
	e2_write_block(sb, e2fs->bgtable[bg].bg_inode_bitmap, 1, (char*)bitmap);
// 	io_res = block_write(sb->s_dev, e2fs->bgtable[bg].bg_inode_bitmap*sb->s_blocksize, bitmap_size, (char*)bitmap);
// 	if( io_res < 0 ){
// 		kfree(bitmap);
// 		spin_unlock(&e2fs->rw_lock);
// 		return EXT2_BAD_INO;
// 	}
	
	//debug_message("allocated inode #%d with mod 0x%X", ino, data->i_mode);
	
	// Fix accounting information
	e2fs->bgtable[bg].bg_free_inodes_count--;
	e2fs->super.s_free_inodes_count--;
	e2fs->dirty = 1;
	
	data->i_ctime = data->i_atime = data->i_mtime = data->i_crtime = rtc_read(); // set the correct creation time!
	data->i_extra_isize = 28;
	
	// Write the inode data to disk
	block_write(sb->s_dev, e2fs->bgtable[bg].bg_inode_table*sb->s_blocksize + e2fs->super.s_inode_size*(idx*8 + bit), 
		    sizeof(e2_inode_t), (char*)data);
	
	// Free the bitmap data
	kfree(bitmap);
	
	// Unlock the superblock
	spin_unlock(&e2fs->rw_lock);
	
	// Flush accounting changes to disk
	e2_super_flush(sb);
	
	return ino;
}

int e2_free_inode(struct superblock* sb, struct inode* inode)
{
	e2_super_private_t* e2fs = EXT2_SUPER(sb);
	
	// Get the block group number
	u32 bg = (inode->i_ino-1) / e2fs->super.s_inodes_per_group;
	u32 local = (inode->i_ino-1) % e2fs->super.s_inodes_per_group;
	
	// Allocate a bitmap buffer
	u32* bitmap = (u32*)kmalloc(e2fs->super.s_inodes_per_group/8);
	if(!bitmap){
		return -ENOMEM;
	}
	
	// free all the inode data blocks
	int result = e2_inode_resize(inode, 0);
	if( result < 0 ){
		kfree(bitmap);
		return result;
	}
	
	// Lock the superblock data
	spin_lock(&e2fs->rw_lock);
	
	// Fix the inode bitmap
	block_read(sb->s_dev, e2fs->bgtable[bg].bg_inode_bitmap*sb->s_blocksize, e2fs->super.s_inodes_per_group/8, (char*)bitmap);
	bitmap[local/32] &= ~(1 << (local % 32));
	block_write(sb->s_dev, e2fs->bgtable[bg].bg_inode_bitmap*sb->s_blocksize, e2fs->super.s_inodes_per_group/8, (char*)bitmap);
	
	// Fix reference counts in the superblock/block group
	e2fs->bgtable[bg].bg_free_inodes_count++;
	e2fs->super.s_free_inodes_count++;
	e2fs->dirty = 1;
	
	// Unlock the data
	spin_unlock(&e2fs->rw_lock);
	
	// Flush the superblock
	e2_super_flush(sb);
	
	return 0;
	
}

/* Read in the entire block map for the inode
 * 	map must be inode->i_private->inode->i_blocks*4 bytes long!
 */
int e2_inode_read_block_map(struct inode* inode, u32* map)
{
	e2_inode_private_t* priv = EXT2_INODE(inode);
	u32 idx = 0;
	u32 nblocks = (priv->inode->i_blocks * 512) / inode->i_super->s_blocksize;
	
	// if there are no blocks, don't continue
	if( nblocks == 0 ){
		if( map ) map[0] = 0;
		return 0;
	}
	
	// grab the first twelve direct blocks
	for(int i = 0; i < 12 && idx < nblocks; ++i,++idx){
		map[idx] = priv->inode->i_block[i];
	}
	
	// are we done?
	if( idx == nblocks ){
		return 0;
	}
	
	// We now need the private buffer, so we lock the internal data spinlock
	spin_lock(&priv->rw_lock);
	
	// an easier typed pointer to the buffer
	u32* buffer = (u32*)priv->rw;
	
	// read the indirect layer
	e2_read_block(inode->i_super, priv->inode->i_block[12], 1, priv->rw);
	
	// copy the block addresses
	for(u32 i = 0; i < (inode->i_super->s_blocksize/4) && idx < nblocks; ++i,++idx){
		map[idx] = buffer[i];
	}
	
	// are we done?
	if( idx == nblocks ){
		spin_unlock(&priv->rw_lock);
		return 0;
	}
	
	// We need a second buffer for the indirect levels within the doubly indirect layer
	u32* indirect = (u32*)kmalloc(inode->i_super->s_blocksize);
	if( !indirect ){
		spin_unlock(&priv->rw_lock);
		return -ENOMEM;
	}
	
	// read the doubly indirect layer
	e2_read_block(inode->i_super, priv->inode->i_block[13], 1, priv->rw);
	
	// iterate over each indirect layer within the doubly indirect
	for(u32 d = 0; d < (inode->i_super->s_blocksize/4) && idx < nblocks; ++d)
	{
		// read in the indirect layer
		e2_read_block(inode->i_super, buffer[d], 1, (char*)indirect);
		// copy the direct blocks
		for(u32 i = 0; i < (inode->i_super->s_blocksize/4) && idx < nblocks; ++i, ++idx)
		{
			map[idx] = indirect[i];
		}
	}
	
	// Are we done?
	if( idx == nblocks ){
		kfree(indirect);
		spin_unlock(&priv->rw_lock);
		return 0;
	}
	
	// We need, yet another, buffer for the doubly indirect levels within the triply indirect layers
	u32* doubly = (u32*)kmalloc(inode->i_super->s_blocksize);
	if( !indirect ){
		kfree(indirect);
		spin_unlock(&priv->rw_lock);
		return -ENOMEM;
	}
	
	// read in the triply indirect layer
	e2_read_block(inode->i_super, priv->inode->i_block[14], 1, priv->rw);
	
	// iterate over each double indirect layer
	for(u32 t = 0; t < (inode->i_super->s_blocksize/4) && idx < nblocks; ++t)
	{
		// read in doubly indirect layer
		e2_read_block(inode->i_super, buffer[t], 1, (char*)doubly);
		// iterate over each indirect layer
		for(u32 d = 0; d < (inode->i_super->s_blocksize/4) && idx < nblocks; ++d)
		{
			// read in indirect layer
			e2_read_block(inode->i_super, buffer[d], 1, (char*)indirect);
			// iterate over each direct block
			for(u32 i = 0; i < (inode->i_super->s_blocksize/4) && idx < nblocks; ++i, ++idx)
			{
				map[idx] = indirect[i];
			}
		}
	}
	
	// free the extra memory
	kfree(doubly);
	kfree(indirect);
	
	// unlock the buffer
	spin_unlock(&priv->rw_lock);
	
	return 0;
}

int e2_inode_write_block_map(struct inode* inode, u32* map)
{
	e2_inode_private_t* priv = EXT2_INODE(inode);
	u32 idx = 0;
	u32 nblocks = (priv->inode->i_blocks * 512) / inode->i_super->s_blocksize;
	
	if( S_ISBLK(priv->inode->i_mode) || S_ISCHR(priv->inode->i_mode) || S_ISFIFO(priv->inode->i_mode) ){
		return 0;
	}
	
	// if there are no blocks, don't continue
	if( nblocks == 0 ){
		memset(priv->inode->i_block, 0, 4*15);
		return 0;
	}
	
	// grab the first twelve direct blocks
	for(int i = 0; i < 12 && idx < nblocks; ++i,++idx){
		priv->inode->i_block[i] = map[idx];
	}
	
	priv->dirty = 1;
	
	// are we done?
	if( idx == nblocks ){
		priv->inode->i_block[12] = 0;
		priv->inode->i_block[13] = 0;
		priv->inode->i_block[14] = 0;
		return 0;
	}
	
	// We now need the private buffer, so we lock the internal data spinlock
	spin_lock(&priv->rw_lock);
	
	// if we recently resized the inode, the indirect blocks may not be allocated
	if( priv->inode->i_block[12] == 0 ){
		priv->inode->i_block[12] = e2_alloc_block(inode->i_super);
	}
	
	// an easier typed pointer to the buffer
	u32* buffer = (u32*)priv->rw;
	
	e2_write_block(inode->i_super, priv->inode->i_block[12], 1, (char*)&map[idx]);
	idx += inode->i_super->s_blocksize/4;
	
	// are we done?
	if( idx >= nblocks ){
		spin_unlock(&priv->rw_lock);
		return 0;
	}
	
	// We need a second buffer for the indirect levels within the doubly indirect layer
	u32* indirect = (u32*)kmalloc(inode->i_super->s_blocksize);
	if( !indirect ){
		spin_unlock(&priv->rw_lock);
		return -ENOMEM;
	}
	
	if( priv->inode->i_block[13] == 0 ){
		// allocate a new double indirect block and clear the buffer
		if( (priv->inode->i_block[13] = e2_alloc_block(inode->i_super)) == 0 ){
			kfree(indirect);
			spin_unlock(&priv->rw_lock);
			return -ENOSPC;
		}
		memset(priv->rw, 0, inode->i_super->s_blocksize);
	} else {
		// read the doubly indirect layer
		e2_read_block(inode->i_super, priv->inode->i_block[13], 1, priv->rw);
	}
	
	// iterate over each indirect layer within the doubly indirect
	for(u32 d = 0; d < (inode->i_super->s_blocksize/4) && idx < nblocks; ++d)
	{
		// allocate a new indirect block if needed
		if( buffer[d] == 0 ){
			if( (buffer[d] = e2_alloc_block(inode->i_super)) == 0 ){
				kfree(indirect);
				spin_unlock(&priv->rw_lock);
				return -ENOSPC;
			}
		}
		e2_write_block(inode->i_super, buffer[d], 1, (char*)&map[idx]);
		idx += inode->i_super->s_blocksize/4;
	}
	
	// rewrite the doubly indirect as it may have been altered
	e2_write_block(inode->i_super, priv->inode->i_block[13], 1, priv->rw);
	
	// Are we done?
	if( idx == nblocks ){
		kfree(indirect);
		spin_unlock(&priv->rw_lock);
		return 0;
	}
	
	// We need, yet another, buffer for the doubly indirect levels within the triply indirect layers
	u32* doubly = (u32*)kmalloc(inode->i_super->s_blocksize);
	if( !indirect ){
		kfree(indirect);
		spin_unlock(&priv->rw_lock);
		return -ENOMEM;
	}
	
	if( priv->inode->i_block[14] == 0 ) {
		// allocate the new triply indirect block
		if( (priv->inode->i_block[14] = e2_alloc_block(inode->i_super)) == 0 ){
			kfree(doubly);
			kfree(indirect);
			spin_unlock(&priv->rw_lock);
			return -ENOSPC;
		}
		memset(priv->rw, 0, inode->i_super->s_blocksize);
	} else {
		// read in the triply indirect layer
		e2_read_block(inode->i_super, priv->inode->i_block[14], 1, priv->rw);
	}
	
	// iterate over each double indirect layer
	for(u32 t = 0; t < (inode->i_super->s_blocksize/4) && idx < nblocks; ++t)
	{
		// allocate the doubly indirect block if needed
		if( buffer[t] == 0 ){
			buffer[t] = e2_alloc_block(inode->i_super);
			if( buffer[t] == 0 ){
				kfree(doubly);
				kfree(indirect);
				spin_unlock(&priv->rw_lock);
				return -ENOSPC;
			}
			memset(doubly, 0 , inode->i_super->s_blocksize);
		} else {
			// read in doubly indirect layer
			e2_read_block(inode->i_super, buffer[t], 1, (char*)doubly);
		}
		// iterate over each indirect layer
		for(u32 d = 0; d < (inode->i_super->s_blocksize/4) && idx < nblocks; ++d)
		{
			// allocate the indirect block if needed
			if( buffer[d] == 0 ){
				if( (buffer[d] = e2_alloc_block(inode->i_super)) == 0 ){
					kfree(doubly);
					kfree(indirect);
					spin_unlock(&priv->rw_lock);
					return -ENOSPC;
				}
			}
			e2_write_block(inode->i_super, buffer[d], 1, (char*)&map[idx]);
			idx += inode->i_super->s_blocksize/4;
		}
		// write the doubly indirect block back to disk (probably modified)
		e2_write_block(inode->i_super, buffer[t], 1, (char*)doubly);
	}
	
	// write the triply indirect block back to disk (probably modified)
	e2_write_block(inode->i_super, priv->inode->i_block[14], 1, priv->rw);
	
	// free the extra memory
	kfree(doubly);
	kfree(indirect);
	
	// unlock the buffer
	spin_unlock(&priv->rw_lock);
	
	return 0;
}

static void e2_release_singly_indirect(struct inode* inode, u32 blocknr)
{
	struct superblock* sb = inode->i_super;
	e2_inode_private_t* priv = (e2_inode_private_t*)inode->i_private;
	
	// Allocate a block for the singly indirect map
	u32* block = kmalloc(sb->s_blocksize);
	if( block == NULL ){
		syslog(KERN_ERR, "unable to allocate singly indirect block!\n");
		return;
	}
	
	// Read in the indirect map
	e2_read_block(sb, blocknr, 1, (void*)block);
	
	// Free each block in the map
	for(u32 i = 0; i < (sb->s_blocksize/4); ++i){
		if( block[i] != 0 ){
			e2_free_block(sb, block[i]);
			if( priv->inode->i_size < sb->s_blocksize )
				priv->inode->i_size = 0;
			else
				priv->inode->i_size -= sb->s_blocksize;
			priv->inode->i_blocks -= (sb->s_blocksize/512);
		} else {
			break;
		}
	}
	
	// Free the block buffer
	kfree(block);
	
	// Free the indirect block
	e2_free_block(sb, blocknr);
	
	return;
}

static void e2_release_doubly_indirect(struct inode* inode, u32 blocknr)
{
	struct superblock* sb = inode->i_super;
	
	// Allocate a block for the doubly indirect map
	u32* block = kmalloc(sb->s_blocksize);
	if( block == NULL ){
		syslog(KERN_ERR, "unable to allocate doubly indirect block!\n");
		return;
	}
	
	// Read in the indirect map
	e2_read_block(sb, blocknr, 1, (void*)block);
	
	// Free each singly indirect block in the map and the block in the map
	for(u32 i = 0; i < (sb->s_blocksize/4); ++i){
		if( block[i] != 0 ){
			// release all the indirect blocks
			e2_release_singly_indirect(inode, block[i]);
		} else {
			break;
		}
	}
	
	// Free the block buffer
	kfree(block);
	
	// Free the indirect block
	e2_free_block(sb, blocknr);
	
	return;
}

static void e2_release_triply_indirect(struct inode* inode, u32 blocknr)
{
	struct superblock* sb = inode->i_super;
	
	// Allocate a block for the triply indirect map
	u32* block = kmalloc(sb->s_blocksize);
	if( block == NULL ){
		syslog(KERN_ERR, "unable to allocate doubly indirect block!\n");
		return;
	}
	
	// Read in the indirect map
	e2_read_block(sb, blocknr, 1, (void*)block);
	
	// Delete everything!
	for(u32 i = 0; i < (sb->s_blocksize/4); ++i){
		if( block[i] != 0 ){
			e2_release_doubly_indirect(inode, block[i]);
		} else {
			break;
		}
	}
	
	// Free the block buffer
	kfree(block);
	
	// Free the indirect block
	e2_free_block(sb, blocknr);
	
	return;
}

/* truncate a given inode (resize to zero) */
int e2_inode_truncate(struct inode* inode)
{
	struct superblock* sb = inode->i_super;
	e2_inode_private_t* priv = (e2_inode_private_t*)inode->i_private;
	
	if( 1 )
	{
		e2_inode_resize(inode, 0);
	} else {
		
		// Free each block in the map
		for(u32 i = 0; i < 12; ++i){
			if( priv->inode->i_block[i] != 0 ){
				e2_free_block(sb, priv->inode->i_block[i]);
				if( priv->inode->i_size < sb->s_blocksize )
					priv->inode->i_size = 0;
				else
					priv->inode->i_size -= sb->s_blocksize;
				priv->inode->i_blocks -= (sb->s_blocksize/512);
				priv->inode->i_block[i] = 0;
			}
		}
		
		if( priv->inode->i_block[12] ){
			e2_release_singly_indirect(inode, priv->inode->i_block[12]);
			priv->inode->i_block[12] = 0;
		}
		if( priv->inode->i_block[13] ){
			e2_release_doubly_indirect(inode, priv->inode->i_block[13]);
			priv->inode->i_block[13] = 0;
		}
		if( priv->inode->i_block[14] ){
			e2_release_triply_indirect(inode, priv->inode->i_block[14]);
			priv->inode->i_block[14] = 0;
		}
	}
	
	priv->dirty = 1;
	e2_inode_flush(inode);
	return 0;
}

ssize_t e2_inode_resize(struct inode* inode, size_t newsize)
{
	e2_inode_private_t* priv = EXT2_INODE(inode);
	struct superblock* sb = inode->i_super;
	
	u32 nblocks = (priv->inode->i_blocks * 512)/sb->s_blocksize;

	// that's dumb...
	if( newsize == priv->inode->i_size ){
		return newsize;
	} else if( newsize < priv->inode->i_size ) { // less than old size
		u32 new_nblocks = newsize / sb->s_blocksize;
		if( newsize % sb->s_blocksize ) new_nblocks++;
		
		if( new_nblocks == nblocks ){
			priv->inode->i_size = newsize;
			priv->dirty = 1;
			e2_inode_flush(inode);
			return newsize;
		}
		
		// realloc the new block structure for the new blocks
		u32* newblock;
		u32 newblock_length = new_nblocks*4;
		if( new_nblocks < 15 ) newblock_length = 15*4;
		newblock = (u32*)kmalloc(newblock_length);
		if( newblock == NULL ){
			return -ENOMEM;
		} else {
			memset(newblock, 0, newblock_length);
		}
		
		memcpy(newblock, priv->block, new_nblocks*4);
		
		// free the old blocks that aren't needed anymore
		for( u32 b = new_nblocks; b < nblocks; ++b){
			e2_free_block(sb, priv->block[b]);
		}
		kfree(priv->block);
		priv->block = newblock;
		
		// Update reference counting
		priv->dirty = 1;
		priv->inode->i_size = newsize;
		priv->inode->i_blocks = (new_nblocks*sb->s_blocksize)/512;
		e2_inode_flush(inode);
		
	} else if( (u32)(priv->inode->i_size/sb->s_blocksize) == (u32)(newsize/sb->s_blocksize) && priv->inode->i_size != 0){
		priv->inode->i_size = newsize;
		priv->dirty = 1;
		e2_inode_flush(inode);
		return newsize;
	} else if( newsize > priv->inode->i_size ){
		u32 extra_blocks = (newsize - priv->inode->i_size)/sb->s_blocksize;
		if( ((newsize-priv->inode->i_size)%sb->s_blocksize) ) extra_blocks++;
		
		// realloc the block structure to accomadate the new blocks
		u32* newblock = (u32*)kmalloc((nblocks+extra_blocks)*4);
		if( newblock == NULL ){
			return -ENOMEM;
		}
		memcpy(newblock, priv->block, nblocks*4);
		kfree(priv->block);
		priv->block = newblock;
		
		priv->dirty = 1;
		
		for(u32 i = nblocks; i < (nblocks+extra_blocks); ++i){
			priv->block[i] = e2_alloc_block(sb);
			if( priv->block[i] == 0 ){
				e2_inode_flush(inode);
				return priv->inode->i_size;
			}
			// update block and byte count
			priv->inode->i_blocks += sb->s_blocksize/512;
			priv->inode->i_size += sb->s_blocksize;
		}
		// correct size
		priv->inode->i_size = newsize;
		
		e2_inode_flush(inode);
	}
		
	
	return 0;
}

ssize_t e2_inode_io(struct inode* inode, int cmd, off_t offset, size_t size, char* buffer)
{
	e2_inode_private_t* priv = EXT2_INODE(inode);
	struct superblock* sb = inode->i_super;
	ssize_t completed = 0;
	
	//spin_lock(&priv->rw_lock);
	
	// Fix odd offset/size combos
	if( cmd == EXT2_READ ){
		// we can't read past the end of the file
		if( (u32)offset >= priv->inode->i_size ){
			return 0;
		}
		// same as above, but we can just read part of the requested area
		if( (offset+size) > priv->inode->i_size ){
			size = priv->inode->i_size-offset;
		}
	} else {
		// we can write past the end. Just resize
		if( (offset+size) > priv->inode->i_size ){
			// resize the inode
			e2_inode_resize(inode, (size_t)offset+size);
		}
	}
	
	// Read/Write initial partial block
	if( offset % sb->s_blocksize )
	{
		// No matter read or write, we need to read this block first
		e2_read_block(sb, priv->block[offset/sb->s_blocksize], 1, priv->rw);
		// if we only wanted to read/write less than a block
		if( size <= (sb->s_blocksize-(offset%sb->s_blocksize)) )
		{
			// do the read/write
			if( cmd == EXT2_READ ){
				memcpy(buffer, priv->rw+(offset%sb->s_blocksize), size);
			} else {
				memcpy(priv->rw+(offset%sb->s_blocksize), buffer, size);
				e2_write_block(sb, priv->block[offset/sb->s_blocksize], 1, priv->rw);
			}
			// unlock and return
			//spin_unlock(&priv->rw_lock);
			return size;
		} else {
			// we wanted more than this block
			// do the read/write
			if( cmd == EXT2_READ ){
				memcpy(buffer, priv->rw+(offset%sb->s_blocksize), sb->s_blocksize-(offset%sb->s_blocksize));
			} else {
				memcpy(priv->rw+(offset%sb->s_blocksize), buffer, sb->s_blocksize-(offset%sb->s_blocksize));
				e2_write_block(sb, priv->block[offset/sb->s_blocksize], 1, priv->rw);
			}
			// update counters
			completed += sb->s_blocksize-(offset%sb->s_blocksize);
			buffer += sb->s_blocksize-(offset%sb->s_blocksize);
			size -= sb->s_blocksize-(offset%sb->s_blocksize);
			offset += sb->s_blocksize-(offset%sb->s_blocksize);
		}
	}
	
	// Transfer any (if any) block sized chunks
	if( (u32)(size / sb->s_blocksize) != 0 )
	{
		u32 nblocks = size / sb->s_blocksize;
		for(u32 i = 0; i < nblocks; ++i)
		{
			// do the read/write
			if( cmd == EXT2_READ ){
				e2_read_block(sb, priv->block[offset/sb->s_blocksize], 1, buffer);
			} else {
				e2_write_block(sb, priv->block[offset/sb->s_blocksize], 1, buffer);
			}
			// update counters
			completed += sb->s_blocksize;
			offset += sb->s_blocksize;
			buffer += sb->s_blocksize;
			size -= sb->s_blocksize;
		}
	}
	
	// transfer any partial block pieces
	if( size != 0 )
	{
		// it's a partial block so we need to read the whole thing for both read and write
		e2_read_block(sb, priv->block[offset/sb->s_blocksize], 1, priv->rw);
		if( cmd == EXT2_READ ){
			memcpy(buffer, priv->rw, size);
		} else {
			memcpy(priv->rw, buffer, size);
			e2_write_block(sb, priv->block[offset/sb->s_blocksize], 1, priv->rw);
		}
		completed += size;
	}
	
	return completed;
}

int e2_inode_link(struct inode* parent, const char* name, struct inode* inode)
{
	e2_inode_private_t* parent_priv = EXT2_INODE(parent);
	e2_inode_private_t* inode_priv = EXT2_INODE(inode);
	e2_dirent_t* dirent = NULL;
	u32 dirent_block = 0;
	struct superblock* sb = parent->i_super;
	
	// parent must be a directory
	if( !S_ISDIR(parent->i_mode) ){
		return -ENOTDIR;
	}
	
	// they must both belong to the same device
	if( parent->i_super != inode->i_super ){
		return -EINVAL;
	}
	
	spin_lock(&parent_priv->rw_lock);
	spin_lock(&inode_priv->rw_lock);
	
	u32 nblocks = (parent_priv->inode->i_blocks*512)/parent->i_super->s_blocksize;
	
	// search every dirent in every block for an unused one that can fit our name
	for( u32 b = 0; b < nblocks && dirent == NULL; ++b)
	{
		e2_read_block(parent->i_super, parent_priv->block[b], 1, parent_priv->rw);
		e2_dirent_t* iter = (e2_dirent_t*)parent_priv->rw;
		while( ((u32)iter) < ((u32)parent_priv->rw + parent->i_super->s_blocksize) )
		{
			if( iter->inode == 0 && (u32)(iter->rec_len-8) >= strlen(name)) {
				dirent = iter;
				dirent_block = parent_priv->block[b];
				break;
			}
			iter = (e2_dirent_t*)( (u32)iter + iter->rec_len );
		}
	}
	
	// no space in already allocated blocks, allocate a new block!
	if( dirent == NULL ){
		ssize_t result = e2_inode_resize(parent, parent_priv->inode->i_size + sb->s_blocksize);
		if( result < 0 ){
			spin_unlock(&inode_priv->rw_lock);
			spin_unlock(&parent_priv->rw_lock);
			return -ENOSPC;
		}
		// we don't need to read from disk, just initialize our in memory block
		memset(parent_priv->rw, 0, sb->s_blocksize);
		dirent = (e2_dirent_t*)parent_priv->rw;
		dirent->rec_len = (u16)sb->s_blocksize;
		dirent->inode = 0;
		// we will then write this block to disk later
		dirent_block = parent_priv->block[nblocks]; // nblocks was the size, now it is the last index (we increased size by 1)
		e2_inode_flush(parent);
	}
	
	e2_dirent_t* split = NULL;
	
	// needed size, aligned to 4 byte boundary
	u32 needed_size = 8 + strlen(name);
	if( (needed_size % 4) ){
		needed_size = (needed_size & ~3) + 4;
	}
	
	// split the dirent into two if possible
	if( (dirent->rec_len - needed_size) > 8 ){
		split = (e2_dirent_t*)( (u32)dirent + needed_size );
		split->rec_len = (u16)(dirent->rec_len-needed_size);
		split->inode = 0;
		dirent->rec_len = (u16)needed_size;
	}
	
	// Fill in the dirent data
	dirent->inode = inode->i_ino;
#ifdef EXT2_HAS_DYNAMIC_REV
	dirent->name_len = (u8)strlen(name);
	// Decide on a file type
	if( S_ISREG(inode->i_mode) ){
		dirent->file_type = DT_REGULAR;
	} else if( S_ISBLK(inode->i_mode) ){
		dirent->file_type = DT_BLOCK;
	} else if( S_ISCHR(inode->i_mode) ){
		dirent->file_type = DT_CHARACTER;
	} else if( S_ISDIR(inode->i_mode) ){
		dirent->file_type = DT_DIRECTORY;
	} else if( S_ISFIFO(inode->i_mode) ){
		dirent->file_type = DT_FIFO;
	} else if( S_ISLNK(inode->i_mode) ){
		dirent->file_type = DT_SYMLINK;
	} else if( S_ISSOCK(inode->i_mode) ){
		dirent->file_type = DT_SOCKET;
	} else {
		dirent->file_type = DT_UNKNOWN;
	}
#else
	dirent->name_len = (u16)strlen(name);
#endif
	// Copy the name string
	strcpy(dirent->name, name);
	
	// we have another link!
	inode_priv->inode->i_links_count++;
	inode_priv->dirty = 1;
	
	// write the block that contains this dirent back to the disk
	e2_write_block(sb, dirent_block, 1, parent_priv->rw);
	
	spin_unlock(&inode_priv->rw_lock);
	spin_unlock(&parent_priv->rw_lock);
	
	e2_inode_flush(inode);
	e2_inode_flush(parent);
	
	return 0;	
}

int e2_inode_unlink(struct inode* parent, const char* name)
{
	e2_inode_private_t* parent_priv = EXT2_INODE(parent);
	e2_inode_private_t* child_priv = NULL;
	
	// lock the parent
	spin_lock(&parent_priv->rw_lock);
	
	// calculate number of fs blocks and length of the name
	u32 nblocks = (parent_priv->inode->i_blocks*512)/parent->i_super->s_blocksize;
	size_t name_len = strlen(name);
	
	// search every dirent in every block for the given name
	for( u32 b = 0; b < nblocks; ++b)
	{
		// read in the block
		e2_read_block(parent->i_super, parent_priv->block[b], 1, parent_priv->rw);
		e2_dirent_t* iter = (e2_dirent_t*)parent_priv->rw;
		while( ((u32)iter) < ((u32)parent_priv->rw + parent->i_super->s_blocksize) )
		{
			if( iter->inode == 0 ){
				continue;
			}
			// If the lengths match and the names are the same
			if( name_len == iter->name_len && strncmp(name, iter->name, name_len) == 0 )
			{
				// grab the inode
				struct inode* child = i_get(parent->i_super, (ino_t)iter->inode);
				if( !child ){
					spin_unlock(&parent_priv->rw_lock);
					return -ENOENT;
				}
				
				// Reset the entry and rewrite it back to the drive
				iter->inode = 0;
				e2_write_block(parent->i_super, parent_priv->block[b], 1, parent_priv->rw);
				
				// lock the child
				child_priv = EXT2_INODE(child);
				spin_lock(&child_priv->rw_lock);
				
				// decremeent reference count
				child_priv->inode->i_links_count--;
				child_priv->dirty = 1;
				
				// unlock the child
				spin_unlock(&child_priv->rw_lock);
				
				e2_inode_flush(child);
				
				// free our inode reference
				i_put(child); // the inode probably gets freed here, unless it is open somewhere else
				
				spin_unlock(&parent_priv->rw_lock);
				
				e2_inode_flush(parent);
				
				return 0;
			}
		}
	}
	
	// unlock the parent
	spin_unlock(&parent_priv->rw_lock);
	
	return -ENOENT;
}