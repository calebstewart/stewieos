#include "ext2.h"
#include "task.h"

//static char e2_temp_block[1024];

int e2_read_super(struct filesystem* fs, struct superblock* sb, dev_t devid, unsigned long flags, void* data);
int e2_put_super(struct filesystem* fs, struct superblock* sb);
int e2_read_inode(struct superblock* sb, struct inode* inode);
void e2_put_inode(struct superblock* sb, struct inode* inode);
int e2_dir_inode_lookup(struct inode* inode, struct dentry* dentry);
int e2_file_open(struct file* file, struct dentry* dentry, int mode);
int e2_file_close(struct file* file, struct dentry* dentry);
ssize_t e2_file_read(struct file* file, char* buffer, size_t count);
int e2_dir_inode_mknod(struct inode* dir, const char* name, mode_t mode, dev_t devid);
int e2_file_readdir(struct file* file, struct dirent* dirent, size_t count);

struct file_operations e2_default_file_operations = {
	.open = e2_file_open, .close = e2_file_close,
	.read = e2_file_read, .readdir = e2_file_readdir
};

struct inode_operations e2_inode_operations = {
	.lookup = e2_dir_inode_lookup,
	.mknod = e2_dir_inode_mknod,
};

struct inode_operations e2_file_inode_operations = {
	.lookup = NULL,
};

struct superblock_operations e2_superblock_ops = {
	.read_inode = e2_read_inode,
	.put_inode = e2_put_inode,
};

struct filesystem_operations e2_filesystem_ops = {
	.read_super = e2_read_super,
	.put_super = e2_put_super
};

struct filesystem e2_filesystem = {
	.fs_name = "ext2",
	.fs_flags = 0,//FS_RDONLY,
	.fs_ops = &e2_filesystem_ops,
};

void e2_setup( void )
{
	printk("ext2: registering file system type...\n");
	int result = register_filesystem(&e2_filesystem);
	if( result != 0 ){
		printk("ext2: unable to register filesystem. error code %d\n");
		return;
	}
}

int e2_read_super(struct filesystem* fs, struct superblock* sb, dev_t devid, unsigned long flags, void* data)
{
	UNUSED(flags);
	UNUSED(data);
	UNUSED(fs);
	
	e2_superblock_t* e2super = NULL;
	
	// Allocate a ext2 superblock
	if( (e2super = (e2_superblock_t*)kmalloc(sizeof(e2_superblock_t))) == NULL ){
		return -ENOMEM;
	}
	
	ssize_t count = block_read(devid, 1024, sizeof(*e2super), (void*)e2super);
	if( count < 0 ){
		kfree(e2super);
		return (int)count;
	}
	
// 	// Read in the super block
// 	result = block_read(devid, 2, 2, e2_temp_block);
// 	if( result != 0 ){
// 		kfree(e2super);
// 		return result;
// 	}
// 	
// 	// Copy the relevant data to our superblock
// 	memcpy(e2super, e2_temp_block, sizeof(*e2super));
	
	// Check the superblock magic number
	if( e2super->s_magic != EXT2_SUPER_MAGIC ){
		return -EINVAL;
	}
	
	sb->s_private = e2super;
	sb->s_mtime = e2super->s_mtime;
	sb->s_wtime = e2super->s_wtime;
	sb->s_blocksize = 1024 << e2super->s_log_block_size;
	sb->s_inode_count = e2super->s_inodes_count;
	sb->s_block_count = e2super->s_blocks_count;
	sb->s_free_block_count = e2super->s_free_blocks_count;
	sb->s_free_inode_count = e2super->s_free_inodes_count;
	sb->s_magic = e2super->s_magic;
	sb->s_dev = devid;
	
	if( e2super->s_rev_level == EXT2_GOOD_OLD_REV ){
		e2super->s_inode_size = EXT2_GOOD_OLD_INODE_SIZE;
	}
	
	sb->s_ops = &e2_superblock_ops;
	struct inode* inode = i_get(sb, EXT2_ROOT_INO);
	sb->s_root = d_alloc_root(inode);
	i_put(inode);
	
	return 0;
}

int e2_put_super(struct filesystem* fs, struct superblock* sb)
{
	UNUSED(fs);
	if( sb->s_private ){
		kfree(sb->s_private);
	}
	return 0;
}

int e2_read_inode(struct superblock* sb, struct inode* inode)
{
	e2_inode_t* e2_inode;
	e2_superblock_t* e2;
	u32 block_group;
	u32 local_ino;
	off_t inode_location;
	ssize_t result;
	e2_bg_descr_t bg;
	
	// Check if this is a valid inode number 
	if( inode->i_ino == EXT2_BAD_INO || inode->i_ino == 0 || inode->i_ino > sb->s_inode_count ){
		return -ENOENT;
	}
	
	// Allocate the private inode structure
	e2_inode = kmalloc(sizeof(e2_inode_t));
	if(!e2_inode){
		return -ENOMEM;
	}
	
	// Get the private superblock structure
	e2 = (e2_superblock_t*)sb->s_private;
	
	// Calculate local offsets
	block_group = (inode->i_ino-1) / e2->s_inodes_per_group;
	local_ino = (inode->i_ino-1) % e2->s_inodes_per_group;
	
	// Read the block group
	e2_get_bg_descr(sb, block_group, &bg);
	
	// Calculate the inode location based on the local inode table
	inode_location = bg.bg_inode_table * sb->s_blocksize + local_ino * e2->s_inode_size;
	
	// Read the inode from the disk
	result = block_read(sb->s_dev, inode_location, sizeof(e2_inode_t), (void*)e2_inode);
	
	if( result < 0 ){
		kfree(e2_inode);
		return result;
	}
	
	inode->i_mode = e2_inode->i_mode;
	inode->i_uid = e2_inode->i_uid;
	inode->i_size = e2_inode->i_size;
	inode->i_atime = e2_inode->i_atime;
	inode->i_ctime = e2_inode->i_ctime;
	inode->i_mtime = e2_inode->i_mtime;
	inode->i_dtime = e2_inode->i_dtime;
	inode->i_gid = e2_inode->i_gid;
	inode->i_nlinks = e2_inode->i_links_count;
	if( S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode) ){
		inode->i_dev = (dev_t)e2_inode->i_block[0];
	} else {
		inode->i_dev = 0;
	}
	inode->i_private = e2_inode;
	inode->i_ops = &e2_inode_operations;
	inode->i_default_fops = &e2_default_file_operations;
	
	return 0;
}

void e2_put_inode(struct superblock* sb, struct inode* inode)
{
	UNUSED(sb);
	kfree(inode->i_private);
	return;
}

int e2_dir_inode_lookup(struct inode* inode, struct dentry* dentry)
{
	UNUSED(inode);
	UNUSED(dentry);
	
//	e2_inode_t* e2ino = (e2_inode_t*)inode->i_private; // ext2 inode structure
	struct superblock* sb = inode->i_super; // super block
//	e2_superblock_t* e2 = (e2_superblock_t*)sb->s_private; // ext2 superblock
	e2_dirent_t* dirent = (e2_dirent_t*)kmalloc(sb->s_blocksize); // one directory entry table
	u32 blockid = 0; // which block to read from
	
	// check for memory error
	if( !dirent ){
		return -ENOMEM;
	}

	// Read each block
	while( e2_read_inode_data(sb, inode, blockid*sb->s_blocksize, sb->s_blocksize, (void*)dirent) == (ssize_t)sb->s_blocksize )
	{
		e2_dirent_t* iter = dirent;
		// Check all dirent within each block
		while( (void*)iter < (void*)((char*)dirent + sb->s_blocksize) )
		{
			// Zero inode number is an unused entry
			if( iter->inode != 0 )
			{
				// Check the name
				if( strlen(dentry->d_name) == iter->name_len && strncmp(dentry->d_name, iter->name, iter->name_len) == 0 ){
					dentry->d_inode = i_get(sb, (ino_t)iter->inode);
					dentry->d_ino = (ino_t)iter->inode;
					kfree(dirent);
					return 0;
				}
			}
			iter = (e2_dirent_t*)( (char*)iter + iter->rec_len );
		}
		blockid++;
	}
	
	kfree(dirent);
	
	return -ENOENT;
}

int e2_dir_inode_mknod(struct inode* dir, const char* name, mode_t mode, dev_t devid)
{
	e2_inode_t e2node;
	
	// Do a lookup to see if the file already exists within this directory
	struct dentry search_entry;
	strcpy(search_entry.d_name, name);
	if( e2_dir_inode_lookup(dir, &search_entry) == 0 ){
		i_put(search_entry.d_inode);
		return -EEXIST;
	}
	
	// Build the inode structure
	e2node.i_mode = (u16)mode;
	e2node.i_uid = current->t_uid;
	e2node.i_size = 0;
	e2node.i_atime = e2node.i_ctime = e2node.i_mtime = e2node.i_dtime;
	e2node.i_gid = (u16)current->t_gid;
	e2node.i_links_count = 1; // We are going to create on in a second anyway
	e2node.i_blocks = 0;
	e2node.i_flags = 0;
	e2node.i_osd1 = 0;
	memset(e2node.i_block, 0, sizeof(u32)*15);
	e2node.i_generation = 0;
	e2node.i_file_acl = 0;
	e2node.i_dir_acl = 0;
	e2node.i_faddr = 0;
	memset(e2node.i_osd2, 0, 12);
	
	// The device id is stored in the first block
	if( S_ISBLK(mode) || S_ISCHR(mode) ){
		e2node.i_block[0] = devid;
	}
	
	// Allocate the inode on disk
	ino_t ino = e2_alloc_inode(dir->i_super, &e2node);
	
	// Well, we apparently couldn't do that...
	if( ino == EXT2_BAD_INO ){
		return -ENOSPC;
	}
	
	// Create a directory entry within this directory inode
	int result = e2_create_dirent(dir, name, ino);
	
	// This is a problem... we need to remove the inode now...
	if( result != 0 ){
		//e2_remove_inode(dir->i_super, ino);
		return result;
	}
	
	return 0;
}

int e2_create_dirent(struct inode* dirnode, const char* name, ino_t ino)
{
	// Local Variables
	struct superblock* sb = dirnode->i_super; // super block
	e2_dirent_t* dirent; // read/write buffer
	u32 inode_block = 0; // which block to read from (relative to inode)
	u32 blockid = 0; // Absolute block nubmer to read from
	struct inode* inode = NULL;
	u16 reclen = (u16)(8 + strlen(name)); // the size of the dirent we will create
	u8 file_type = 0;
	ssize_t rw_result = 0;
	
	if( strlen(name) > 255 ){
		return -ENAMETOOLONG;
	}
	
	// Get the inode so that we can get the file type and make sure it exists
	inode = i_get(sb, ino);
	if(!inode){
		return -EINVAL;
	}
	
	// Decide on the file type based on the inode mode
	if( S_ISREG(inode->i_mode) ){
		file_type = DT_REGULAR;
	} else if( S_ISBLK(inode->i_mode) ){
		file_type = DT_BLOCK;
	} else if( S_ISCHR(inode->i_mode) ){
		file_type = DT_CHARACTER;
	} else if( S_ISDIR(inode->i_mode) ){
		file_type = DT_DIRECTORY;
	} else if( S_ISFIFO(inode->i_mode) ){
		file_type = DT_FIFO;
	} else if( S_ISLNK(inode->i_mode) ){
		file_type = DT_SYMLINK;
	} else if( S_ISSOCK(inode->i_mode) ){
		file_type = DT_SOCKET;
	} else {
		file_type = DT_UNKNOWN;
	}
	
	// Allocate our read/write buffer
	dirent = (e2_dirent_t*)kmalloc(sb->s_blocksize);
	// check for memory error
	if( !dirent ){
		return -ENOMEM;
	}
	
	// Read each block (first get the absolute block number then read it within the loop
	while( (blockid = e2_get_direct_block(sb, (e2_inode_t*)dirnode->i_private, inode_block)) )
	{
		// Read in the absolute block
		if( (rw_result = e2_read_block(sb, blockid, 1, dirent)) != 1 ){
			kfree(dirent);
			return rw_result;
		}
		
		e2_dirent_t* iter = dirent;
		// Check all dirent within each block
		while( (void*)iter < (void*)((char*)dirent + sb->s_blocksize) )
		{
			if( (iter->rec_len-(10+iter->name_len)) >= reclen ){
				reclen = (u16)(iter->rec_len-(10+iter->name_len));
				iter->rec_len = (u16)(10+iter->name_len);
				iter = (e2_dirent_t*)( (char*)iter + iter->rec_len );
				iter->rec_len = reclen;
				iter->inode = ino;
				iter->name_len = (u8)strlen(name);
				iter->file_type = file_type;
				strncpy(iter->name, name, iter->name_len);
				e2_write_block(sb, blockid, 1, dirent);
				return 0;
			}
			iter = (e2_dirent_t*)( (char*)iter + iter->rec_len );
		}
		inode_block++;
	}
	
	kfree(dirent);
	
	return -ENOSPC;
}

ssize_t e2_read_block(struct superblock* sb, off_t block, size_t count, void* buffer)
{
	e2_superblock_t* e2 = (e2_superblock_t*)sb->s_private;
	off_t end_block = (block+count) > e2->s_blocks_count ? e2->s_blocks_count : (block+count);
	ssize_t result = block_read(sb->s_dev, block*sb->s_blocksize, (end_block-block)*sb->s_blocksize, buffer);
	if( result > 0 ){
		return result/sb->s_blocksize;
	}
	return result;
}
ssize_t e2_write_block(struct superblock* sb, off_t block, size_t count, const void* buffer)
{
	e2_superblock_t* e2 = (e2_superblock_t*)sb->s_private;
	off_t end_block = (block+count) > e2->s_blocks_count ? e2->s_blocks_count : (block+count);
	ssize_t result = block_write(sb->s_dev, block*sb->s_blocksize, (end_block-block)*sb->s_blocksize, buffer);
	if( result > 0 ){
		return result/sb->s_blocksize;
	}
	return result;
}

int e2_get_bg_descr(struct superblock* sb, size_t bgidx, e2_bg_descr_t* descr)
{
	//e2_superblock_t* e2 = (e2_superblock_t*)sb->s_private;
	off_t location = sizeof(e2_bg_descr_t)*bgidx;
	
	if( sb->s_blocksize == 1024 ){
		location += 2048;
	} else {
		location += sb->s_blocksize;
	}
	
	ssize_t result = block_read(sb->s_dev, location, sizeof(e2_bg_descr_t), (void*)descr);
	if( result < 0){
		return result;
	}
	
	return 0;
}

int e2_write_bg_descr(struct superblock* sb, size_t bgidx, const e2_bg_descr_t* descr)
{
	//e2_superblock_t* e2 = (e2_superblock_t*)sb->s_private;
	off_t location = sizeof(e2_bg_descr_t)*bgidx;
	
	if( sb->s_blocksize == 1024 ){
		location += 2048;
	} else {
		location += sb->s_blocksize;
	}
	
	ssize_t result = block_write(sb->s_dev, location, sizeof(e2_bg_descr_t), (const void*)descr);
	if( result < 0){
		return result;
	}
	
	return 0;
}

ino_t e2_alloc_inode(struct superblock* sb, e2_inode_t* inode)
{
	e2_superblock_t* e2 = (e2_superblock_t*)sb->s_private;
	size_t grpcnt = e2->s_blocks_count / e2->s_blocks_per_group;
	e2_bg_descr_t bg;
	ino_t ino = (ino_t)-1;
	//int result;
	
	if( e2->s_free_inodes_count == 0 ){
		return EXT2_BAD_INO;
	}
	
	for(size_t i = 0; i < grpcnt; i++){
		
		// Grab the block group descriptor
		e2_get_bg_descr(sb, i, &bg);
		if( bg.bg_free_inodes_count == 0 ) continue;
		
		// Allocate space for the group bitmap
		u32* bitmap = (u32*)kmalloc(sb->s_blocksize);
		if( !bitmap ){
			return EXT2_BAD_INO;
		}
		// Read in the inode bitmap
		block_read(sb->s_dev, bg.bg_inode_bitmap*sb->s_blocksize, sb->s_blocksize, (void*)bitmap);
		
		// Look for the open inode
		for(size_t d = 0; d < (e2->s_inodes_per_group/8) && ino == (ino_t)-1; ++d){
			if( bitmap[d] == 0xFFFFFFFF ) continue;
			for(int b = 0; b < 32 && ino == (ino_t)-1; ++b){
				if( !(bitmap[d] & (1 << b)) ){
					ino = (ino_t)(d*32 + b);
					bitmap[d] |= (1<<b);
				}
			}
		}
		if( ino == (ino_t)-1 ){
			kfree(bitmap);
			continue;
		}
		
		ino_t local_ino = ino;
		ino = (ino_t)(ino + i*e2->s_inodes_per_group + 1);
		
		// Re-write the new bitmap to the device
		block_write(sb->s_dev, bg.bg_inode_bitmap*sb->s_blocksize, sb->s_blocksize, (const void*)bitmap);
		// Decrement the block group free inode count and rewrite the block group
		bg.bg_free_inodes_count--;
		e2_write_bg_descr(sb, i, &bg);
		//  Decrement the total free inodes count and rewrite the header to the disk
		e2->s_free_inodes_count--;
		block_write(sb->s_dev, 1024, sizeof(*e2), (void*)e2);
		
		// Calculate the inode location based on the local inode table
		off_t inode_location = bg.bg_inode_table * sb->s_blocksize + local_ino * e2->s_inode_size;
		
		// Write the inode to the disk
		block_write(sb->s_dev, inode_location, sizeof(e2_inode_t), (const void*)inode);
		
		return ino;
	}
	
	return EXT2_BAD_INO;
}

u32 e2_alloc_block(struct superblock* sb)
{
	e2_superblock_t* e2 = (e2_superblock_t*)sb->s_private;
	size_t grpcnt = e2->s_blocks_count / e2->s_blocks_per_group;
	e2_bg_descr_t bg;
	u32 block = (u32)-1;
	//int result;
	
	if( e2->s_free_blocks_count == 0 ){
		return 0;
	}
	
	for(size_t i = 0; i < grpcnt; i++){
		
		// Grab the block group descriptor
		e2_get_bg_descr(sb, i, &bg);
		if( bg.bg_free_inodes_count == 0 ) continue;
		
		// Allocate space for the group bitmap
		u32* bitmap = (u32*)kmalloc(sb->s_blocksize);
		if( !bitmap ){
			return EXT2_BAD_INO;
		}
		// Read in the block bitmap
		block_read(sb->s_dev, bg.bg_block_bitmap*sb->s_blocksize, sb->s_blocksize, (void*)bitmap);
		
		// Look for the open block
		for(size_t d = 0; d < (e2->s_inodes_per_group/8) && block == (u32)-1; ++d){
			if( bitmap[d] == 0xFFFFFFFF ) continue;
			for(int b = 0; b < 32 && block == (u32)-1; ++b){
				if( !(bitmap[d] & (1 << b)) ){
					block = (u32)(d*32 + b);
					bitmap[d] |= (1<<b);
				}
			}
		}
		if( block == (u32)-1 ){
			kfree(bitmap);
			continue;
		}
		block = (u32)(block + i*e2->s_blocks_per_group);
		
		// Re-write the new bitmap to the device
		block_write(sb->s_dev, bg.bg_block_bitmap*sb->s_blocksize, sb->s_blocksize, (const void*)bitmap);
		// Decrement the block group free block count and rewrite the block group
		bg.bg_free_blocks_count--;
		e2_write_bg_descr(sb, i, &bg);
		//  Decrement the total free blocks count and rewrite the header to the disk
		e2->s_free_blocks_count--;
		block_write(sb->s_dev, 1024, sizeof(*e2), (void*)e2);
		
		kfree(bitmap);
		
		return block;
	}
	
	return 0;
}

u32 e2_get_direct_block(struct superblock* sb, e2_inode_t* inode, u32 block)
{
	u32 ids_per_block = sb->s_blocksize / 4; // number of block ids within an indirect block
	
	// Direct Blocks
	if( block < 12 ){
		return inode->i_block[block];
	} else{
		block -= 12;
	}
	
	u32* blocks = (u32*)kmalloc(sb->s_blocksize);
	
	// Indirect blocks
	if( block < ids_per_block ){
		u32 indirect = inode->i_block[12];
		block_read(sb->s_dev, indirect*sb->s_blocksize, sb->s_blocksize, (void*)blocks);
		block = blocks[block];
		kfree(blocks);
		return block;
	} else {
		block -= ids_per_block;
	}
	
	// Doubly Idirect
	if( block < (ids_per_block*ids_per_block) ){
		block_read(sb->s_dev, inode->i_block[13]*sb->s_blocksize, sb->s_blocksize, (void*)blocks);
		if( blocks[block/ids_per_block] == 0 ){
			kfree(blocks);
			return 0;
		}
		block_read(sb->s_dev, blocks[block/ids_per_block]*sb->s_blocksize, sb->s_blocksize, (void*)blocks);
		block = blocks[block % ids_per_block];
		kfree(blocks);
	} else {
		block -= ids_per_block*ids_per_block;
	}
	
	// Triply Indirect Blocks
	if( block < (ids_per_block*ids_per_block*ids_per_block) ){
		block_read(sb->s_dev, inode->i_block[14]*sb->s_blocksize, sb->s_blocksize, (void*)blocks);
		block_read(sb->s_dev, blocks[block/(ids_per_block*ids_per_block)], sb->s_blocksize, (void*)blocks);
		block_read(sb->s_dev, blocks[(block%(ids_per_block*ids_per_block))/ids_per_block], sb->s_blocksize, (void*)blocks);
		block = blocks[(block%(ids_per_block*ids_per_block))%ids_per_block];
		kfree(blocks);
	} else {
		return -EINVAL;
	}
	
	return 0;
}

// I hate this function...
ssize_t e2_read_inode_data(struct superblock* sb, struct inode* inode, off_t offset, size_t size, char* buffer)
{
	e2_inode_t* e2ino = (e2_inode_t*)inode->i_private;
	u32 block = 0;
	
	// these are the block IDs for starting and ending blocks within the inode
	u32 start_bid = offset / sb->s_blocksize;
	size_t requested_size = size;
	int result = 0;
	
	if( (offset % sb->s_blocksize) != 0 )
	{
		block = e2_get_direct_block(sb, e2ino, start_bid);
		if( block == 0 ){
			return requested_size - size;
		}
		if( size < (sb->s_blocksize - (offset % sb->s_blocksize)) ){
			result = block_read(sb->s_dev, block*sb->s_blocksize + (offset%sb->s_blocksize), size, buffer);
			if( result < 0 ){
				return result;
			}
			return size;
		} else {
			result = block_read(sb->s_dev, block*sb->s_blocksize + (offset%sb->s_blocksize), (sb->s_blocksize-(offset%sb->s_blocksize)), buffer);
			if( result < 0 ){
				return result;
			}
			buffer += (sb->s_blocksize - (offset % sb->s_blocksize));
			size -= (sb->s_blocksize - (offset % sb->s_blocksize));
		}
		start_bid++;
		offset = start_bid*sb->s_blocksize;
	}
	
	if( size >= sb->s_blocksize )
	{
		// Read all the whole blocks
		for(size_t i = 0; i < (size / sb->s_blocksize); ++i)
		{
			block = e2_get_direct_block(sb, e2ino, start_bid);
			if( block == 0 ){
				return (i * sb->s_blocksize);
			}
			result = block_read(sb->s_dev, block*sb->s_blocksize, sb->s_blocksize, buffer);
			if( result < 0 ){
				return result;
			}
			start_bid++;
			buffer += sb->s_blocksize;
		}
		size = (size % sb->s_blocksize);
	}
	
	// One last partial block
	if( size != 0 )
	{
		block = e2_get_direct_block(sb, e2ino, start_bid);
		if( block == 0 ){
			return requested_size - size;
		}
		result = block_read(sb->s_dev, e2_get_direct_block(sb, e2ino, start_bid)*sb->s_blocksize, size, buffer);
		if( result < 0 ){
			return result;
		}
		size = 0;
	}
	
	return requested_size;
}

int e2_file_open(struct file* file, struct dentry* dentry, int flags)
{
	UNUSED(file); UNUSED(flags); UNUSED(dentry);
	return 0;
}

int e2_file_close(struct file* file, struct dentry* dentry)
{
	UNUSED(file); UNUSED(dentry);
	return 0;
}

ssize_t e2_file_read(struct file* file, char* buffer, size_t count)
{
	if( file->f_off >= file_inode(file)->i_size ){
		return 0;
	}
	ssize_t completed = e2_read_inode_data(file_inode(file)->i_super, file_inode(file), file->f_off, count, buffer);
	if( completed < 0 ){
		return completed;
	}
	file->f_off += completed;
	return completed;
}

int e2_file_readdir(struct file* file, struct dirent* dirent, size_t count)
{
	e2_dirent_t* e2dirent = kmalloc(264); // e2_dirent_t is 8 bytes plus the name (with a max length of 256)
	if( !e2dirent ){
		return -ENOMEM;
	}
	for(size_t i = 0; i < count; ++i)
	{
		ssize_t completed = e2_read_inode_data(file_inode(file)->i_super, file_inode(file), file->f_off, 264, (void*)e2dirent);
		if( completed < 0 ){
			kfree(e2dirent);
			return (int)completed;
		} else if( completed == 0 ){
			kfree(e2dirent);
			return i;
		} else if( e2dirent->inode != 0 ){
			dirent[i].d_ino = (ino_t)e2dirent->inode;
			dirent[i].d_type = e2dirent->file_type;
			strncpy(dirent[i].d_name, e2dirent->name, e2dirent->name_len);
			dirent[i].d_name[e2dirent->name_len] = 0;
		} else {
			i--; // this wasn't a valid entry
		}
		file->f_off += e2dirent->rec_len;
	}

	return count;
}