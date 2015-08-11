#include "ext2fs/ext2.h"
#include "kmem.h"
#include "dentry.h"
#include <errno.h>
#include "block.h"

/* Read in the super block structure and initial internal data structures */
int e2_read_super(struct filesystem* fs ATTR((unused)), struct superblock* sb, dev_t devid,
		  unsigned long flags ATTR((unused)), void* data ATTR((unused)))
{
	
	// Allocate space for the private data
	e2_super_private_t* e2sup = (e2_super_private_t*)kmalloc(sizeof(e2_super_private_t));
	if( !e2sup ){
		return -ENOMEM;
	}
	
	// reset the memory
	memset(e2sup, 0, sizeof(*e2sup));
	spin_init(&e2sup->rw_lock);
	
	// Read the superblock structure from disk
	ssize_t rdresult = block_read(devid, 1024, sizeof(e2sup->super), (void*)&e2sup->super);
	if( rdresult < 0 ){
		kfree(e2sup);
		return (int)rdresult;
	}
	
	// Check the magic values
	if( e2sup->super.s_magic != EXT2_SUPER_MAGIC ){
		kfree(e2sup);
		return -EINVAL;
	}
	
	// copy some useful values
	sb->s_private = e2sup;
	sb->s_mtime = e2sup->super.s_mtime;
	sb->s_wtime = e2sup->super.s_wtime;
	sb->s_blocksize = 1024 << e2sup->super.s_log_block_size;
	sb->s_inode_count = e2sup->super.s_inodes_count;
	sb->s_block_count = e2sup->super.s_blocks_count;
	sb->s_free_block_count = e2sup->super.s_free_blocks_count;
	sb->s_free_inode_count = e2sup->super.s_free_inodes_count;
	sb->s_magic = e2sup->super.s_magic;
	sb->s_dev = devid;
	
	// Assign the operations structure
	sb->s_ops = &e2_superblock_ops;
	
	// Calculate the size of the block group table (in blocks)
	size_t table_size = sizeof(e2_bg_descr_t)*EXT2_BGCOUNT(sb);
// 	if( (table_size % sb->s_blocksize) != 0 ){
// 		table_size = (table_size-(table_size%sb->s_blocksize))+sb->s_blocksize;
// 	}
	
	// Calculate the starting block of the block group table
	// This is the first block following the superblock
	size_t start_block = (sb->s_blocksize == 1024) ? 2 : 1;
	
	// Allocate data for the table
	e2sup->bgtable = (e2_bg_descr_t*)kmalloc(table_size);
	if( !e2sup->bgtable ){
		kfree(e2sup);
		return -EINVAL;
	}
	
	block_read(sb->s_dev, start_block*sb->s_blocksize, table_size, (void*)e2sup->bgtable);
	
// 	// Read in the blocks
// 	if( e2_read_block(sb, start_block, table_size / sb->s_blocksize, (void*)e2sup->bgtable) < 0 ){
// 		kfree(e2sup);
// 		return -EINVAL;
// 	}
	
	// read in and allocate the root inode/dentry
	struct inode* root = i_get(sb, EXT2_ROOT_INO);
	sb->s_root = d_alloc_root(root);
	i_put(root);
	
	return 0;
}

/* Release internal data structures and return state to that of before calling e2_read_super */
int e2_put_super(struct filesystem* fs ATTR((unused)), struct superblock* sb)
{
	e2_super_private_t* e2sup = EXT2_SUPER(sb);
	
	// Flush any pending changes
	e2_super_flush(sb);
	
	// Free internal data structures
	kfree(e2sup->bgtable);
	kfree(e2sup);
	
	return 0;
}

/* Flush pending superblock or block group table changes to disk */
int e2_super_flush(struct superblock* sb)
{
	e2_super_private_t* e2sup = EXT2_SUPER(sb);
	
	if( !e2sup->dirty ){
		return 0;
	} else {
		e2sup->dirty = 0;
	}
	
	// Write the superblock to disk
	block_write(sb->s_dev, 1024, sizeof(e2sup->super), (void*)&e2sup->super);
	
	// Calculate the number of blocks in the block group table
	size_t bgsize = (sizeof(e2_bg_descr_t)*EXT2_BGCOUNT(sb));
	// Calculate block group table location
	size_t bgloc = (sb->s_blocksize == 1024) ? 2 : 1;
	
	block_write(sb->s_dev, bgloc*sb->s_blocksize, bgsize, (void*)e2sup->bgtable);
	
	// Write the block group table back to disk
	//e2_write_block(sb, bgloc, bgsize, (void*)e2sup->bgtable);
	
	return 0;
}

