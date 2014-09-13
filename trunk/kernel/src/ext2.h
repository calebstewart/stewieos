#ifndef _EXT2_H_
#define _EXT2_H_

#include "kernel.h"
#include "fs.h"
#include "sys/types.h"
#include <errno.h>
#include "kmem.h"
#include "block.h"
#include "dentry.h"

// Define Dynamic Revision constants and structures
#define EXT2_HAS_DYNAMIC_REV

#define EXT2_SUPER_MAGIC 0xEF53
#define EXT2_GOOD_OLD_INODE_SIZE 128
#define EXT2_GOOD_OLD_REV 0
#define EXT2_DYNAMIC_REV 1

#define EXT2_BAD_INO 1
#define EXT2_ROOT_INO 2
#define EXT2_ACL_IDX_INO 3
#define EXT2_ACL_DATA_INO 4
#define EXT2_BOOT_LOADER_INO 5
#define EXT2_UNDEL_DIR_INO 6

// Values for i_mode
#define EXT2_S_IFSOCK 0xC000
#define EXT2_S_IFLNK 0xA000
#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFBLK 0x6000
#define EXT2_S_IFDIR 0x4000
#define EXT2_S_IFCHR 0x2000
#define EXT2_S_IFIFO 0x1000
#define EXT2_S_ISUID 0x0800
#define EXT2_S_ISGID 0x0400
#define EXT2_S_ISVTX 0x0200
#define EXT2_S_IRUSR 0x0100
#define EXT2_S_IWUSR 0x0080
#define EXT2_S_IXUSR 0x0040
#define EXT2_S_IRGRP 0x0020
#define EXT2_S_IWGRP 0x0010
#define EXT2_S_IXGRP 0x0008
#define EXT2_S_IROTH 0x0004
#define EXT2_S_IWOTH 0x0002
#define EXT2_S_IXOTH 0x0001

typedef struct _e2_superblock
{
	u32 s_inodes_count;
	u32 s_blocks_count;
	u32 s_r_blocks_count;
	u32 s_free_blocks_count;
	u32 s_free_inodes_count;
	u32 s_first_data_block;
	u32 s_log_block_size;
	u32 s_log_frag_size;
	u32 s_blocks_per_group;
	u32 s_frags_per_group;
	u32 s_inodes_per_group;
	u32 s_mtime;
	u32 s_wtime;
	u16 s_mnt_count;
	u16 s_max_mnt_count;
	u16 s_magic;
	u16 s_state;
	u16 s_errors;
	u16 s_minor_rev_level;
	u32 s_lastcheck;
	u32 s_checkinterval;
	u32 s_creator_os;
	u32 s_rev_level;
	u16 s_def_resuid;
	u16 s_def_resgid;
// Specific to s_rev_level == EXT2_DYNAMIC_REV
#ifdef EXT2_HAS_DYNAMIC_REV
	u32 s_first_ino;
	u16 s_inode_size;
	u16 s_block_group_nr;
	u32 s_feature_compat;
	u32 s_feature_incompat;
	u32 s_feature_ro_compat;
	char s_uuid[16];
	char s_volume_name[16];
	char s_last_mounted[64];
	u32 s_algo_bitmap;
	// Performance Hints
	u8 s_prealloc_blocks;
	u8 s_prealloc_dir_blocks;
	u16 s_reserved0;
	// Journaling Support
	char s_journal_uuid[16];
	u32 s_journal_inum;
	u32 s_journal_dev;
	u32 s_last_orphan;
	// Directory Indexing Support
	char s_hash_seed[16];
	u8 s_def_hash_version;
	u16 s_reserved1;
	u8 s_reserved2;
	// Other Options
	u32 s_default_mount_options;
	u32 s_first_meta_bg;
	// There are 760 other bytes of unused memory after this
#endif
} e2_superblock_t;

typedef struct _e2_inode
{
	u16 i_mode;
	u16 i_uid;
	u32 i_size;
	u32 i_atime;
	u32 i_ctime;
	u32 i_mtime;
	u32 i_dtime;
	u16 i_gid;
	u16 i_links_count;
	u32 i_blocks;
	u32 i_flags;
	u32 i_osd1;
	u32 i_block[15];
	u32 i_generation;
	u32 i_file_acl;
	u32 i_dir_acl;
	u32 i_faddr;
	char i_osd2[12];
} e2_inode_t;

typedef struct _e2_dirent
{
	u32 inode;
	u16 rec_len;
	u8 name_len;
	u8 file_type;
	char name[];
} e2_dirent_t;

typedef struct _e2_bg_descr
{
	u32 bg_block_bitmap;
	u32 bg_inode_bitmap;
	u32 bg_inode_table;
	u16 bg_free_blocks_count;
	u16 bg_free_inodes_count;
	u16 bg_used_dirs_count;
	u16 bg_pad;
	char bg_reserved[12];
} e2_bg_descr_t;

void e2_setup(void );

ino_t e2_alloc_inode(struct superblock* sb, e2_inode_t* inode);
int e2_create_dirent(struct inode* dir, const char* name, ino_t ino);

int e2_get_bg_descr(struct superblock* sb, size_t bgidx, e2_bg_descr_t* descr);
int e2_write_bg_descr(struct superblock* sb, size_t bgidx, const e2_bg_descr_t* descr);

ssize_t e2_read_block(struct superblock* sb, off_t block, size_t count, void* buffer);
ssize_t e2_write_block(struct superblock* sb, off_t block, size_t count, const void* buffer);

ssize_t e2_read_inode_data(struct superblock* sb, struct inode* inode, off_t offset, size_t size, char* buffer);

u32 e2_get_direct_block(struct superblock* sb, e2_inode_t* inode, u32 blocknr);

#endif