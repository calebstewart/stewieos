#include <errno.h>
#include "ext2fs/ext2.h"
#include "kmem.h"
#include "block.h"
#include "fs.h"

/* Utility function for read blocks from the device */
int e2_read_block(struct superblock* sb, u32 block, size_t count, char* buffer)
{
	return (int)block_read(sb->s_dev, block*sb->s_blocksize, count*sb->s_blocksize, buffer);
}

/* Utility function for writing blocks to the device */
int e2_write_block(struct superblock* sb, u32 block, size_t count, const char* buffer)
{
	return (int)block_write(sb->s_dev, block*sb->s_blocksize, count*sb->s_blocksize, buffer);
}

/* Allocate a new block of data on the disk and return its index */
u32 e2_alloc_block(struct superblock* sb)
{
	e2_super_private_t* e2fs = EXT2_SUPER(sb);
	u32 block = 0;
	
	// Check if there are any free blocks
	if( e2fs->super.s_free_blocks_count == 0 ){
		return 0;
	}
	
	// Allocate a temporary bitmap structure
	u32* bitmap = (u32*)kmalloc(sb->s_blocksize);
	if(!bitmap){
		return -ENOMEM;
	}
	
	// Lock the private data
	spin_lock(&e2fs->rw_lock);
	
	// Iterate over every block group
	for(u32 bg = 0; bg < EXT2_BGCOUNT(sb); ++bg)
	{
		// Check if there are any free blocks
		if( e2fs->bgtable[bg].bg_free_blocks_count == 0 ) continue;
		
		// read in the block group block bitmap
		e2_read_block(sb, e2fs->bgtable[bg].bg_block_bitmap, 1, (char*)bitmap);
		
		// Iterate over every block in the group
		for(u32 b = 0; b < e2fs->super.s_blocks_per_group; ++b)
		{
			// check if it's free
			if( !(bitmap[b/32] & (1<<(b%32))) ){
				bitmap[b/32] |= (1<<(b%32)); // reserve it
				// write the bitmap back to disk
				e2_write_block(sb, e2fs->bgtable[bg].bg_block_bitmap, 1, (char*)bitmap);
				// Adjust account parameters
				e2fs->bgtable[bg].bg_free_blocks_count--;
				e2fs->super.s_free_blocks_count--;
				// Notify the system that the super block is dirty
				e2fs->dirty = 1;
				// Calculate the block index
				block = b + (bg*e2fs->super.s_blocks_per_group);
				// Unlock and return
				spin_unlock(&e2fs->rw_lock);
				kfree(bitmap);
				return block;
			}
		}
		
	}
	
	
	// Unlock, free and return
	spin_unlock(&e2fs->rw_lock);
	
	kfree(bitmap);
	
	return 0;
}

/* Free a block of data from the disk */
int e2_free_block(struct superblock* sb, u32 block)
{
	e2_super_private_t* e2fs = EXT2_SUPER(sb);
	
	// Allocate a temporary buffer
	u32* bitmap = (u32*)kmalloc(sb->s_blocksize);
	if(!bitmap){
		return -ENOMEM;
	}
	
	// lock the private data
	spin_lock(&e2fs->rw_lock);
	
	// Calculate indices
	u32 bg = block / e2fs->super.s_blocks_per_group;
	u32 local = block % e2fs->super.s_blocks_per_group;
	
	// Read in the bitmap, modify, and write it again
	e2_read_block(sb, e2fs->bgtable[bg].bg_block_bitmap, 1, (char*)bitmap);
	bitmap[local/32] &= ~(1<<(local%32));
	e2_write_block(sb, e2fs->bgtable[bg].bg_block_bitmap, 1, (char*)bitmap);
	
	// Adjust accounting details
	e2fs->super.s_free_blocks_count++;
	e2fs->bgtable[bg].bg_free_blocks_count++;
	e2fs->dirty = 1;
	
	// unlock free and return
	spin_unlock(&e2fs->rw_lock);
	
	kfree(bitmap);
	
	return 0;
}