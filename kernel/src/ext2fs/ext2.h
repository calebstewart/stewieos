#ifndef _EXT2_H_
#define _EXT2_H_

#include "kernel.h"
#include "spinlock.h"
#include "fs.h"

#define MODULE_NAME "ext2fs"
#define MODULE_START ext2fs_load
#define MODULE_END ext2fs_remove

// Convert things like fragment, and block sizes to their linear equivalents
#define EXT2_LOG2LIN(l) (1024 << (l))

// Value of superblock->s_magic
#define EXT2_SUPER_MAGIC				0xEF53

// Values for superblock->s_state
#define EXT2_VALID_FS					1
#define EXT2_ERROR_FS					2

// Values for superblock->s_errors
#define EXT2_ERRORS_CONTINUE				1 /* continue as if nothing happened */
#define EXT2_ERRORS_RO					2 /* remount read-only */
#define EXT2_ERRORS_PANIC				3 /* cause a kernel panic */

// Values for s_creator_os
#define EXT2_OS_LINUX					0
#define EXT2_OS_HURD					1
#define EXT2_OS_MASIX					2
#define EXT2_OS_FREEBSD					3
#define EXT2_OS_LITES					4

// Values for s_rev_level
#define EXT2_GOOD_OLD_REV				0
#define EXT2_DYNAMIC_REV				1

// A convenience value for inode size
#define EXT2_GOOD_OLD_INODE_SIZE			128

// Specify where we support the dynamic revision
#define EXT2_HAS_DYNAMIC_REV				0

// Values for s_feature_compat
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC		0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES		0x0002
#define EXT2_FEATURE_COMPAT_HAS_JOURNAL			0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR			0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INO			0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX			0x0020

// Values for s_feature_incompat
#define EXT2_FEATURE_INCOMPAT_COMPRESSION		0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE			0x0002
#define EXT2_FEATURE_INCOMPAT_RECOVER			0x0004
#define EXT2_FEATURE_INCOMPAT_JOURNAL_DEV		0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG			0x0010

// Values for s_feature_ro_compat
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER		0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE		0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR		0x0004

// Values for s_algo_bitmap
#define EXT2_LZV1_ALG					0x0001
#define EXT2_LZRW3A_ALG					0x0002
#define EXT2_GZIP_ALG					0x0004
#define EXT2_BZIP2_ALG					0x0008
#define EXT2_LZO_ALG					0x0010

// Defined Reserved Inodes
#define EXT2_BAD_INO					1 /* bad blocks inode */
#define EXT2_ROOT_INO					2 /* root directory inode */
#define EXT2_ACL_IDX_INO				3 /* ACL index inode (deprecated?) */
#define EXT2_ACL_DATA_INO				4 /* ACL data inode (deprecated?) */
#define EXT2_BOOT_LOADER_INO				5 /* boot loader inode */
#define EXT2_UNDEL_DIR_INO				6 /* undelete directory inode */

// Values for i_mode
#define EXT2_S_IFSOCK					0xC000 /* socket */
#define EXT2_S_IFLNK					0xA000 /* symbolic link */
#define EXT2_S_IFREG					0x8000 /* regular file */
#define EXT2_S_IFBLK					0x6000 /* block device */
#define EXT2_S_IFDIR					0x4000 /* directory */
#define EXT2_S_IFCHR					0x2000 /* character device */
#define EXT2_S_IFIFO					0x1000 /* fifo */
#define EXT2_S_ISUID					0x0800 /* set process user id */
#define EXT2_S_ISGID					0x0400 /* set process group id */
#define EXT2_S_ISVTX					0x0200 /* sticky bit */
#define EXT2_S_IRUSR					0x0100 /* user read */
#define EXT2_S_IWUSR					0x0080 /* user write */
#define EXT2_S_IXUSR					0x0040 /* user execute */
#define EXT2_S_IRGRP					0x0020 /* group read */
#define EXT2_S_IWGRP					0x0010 /* group write */
#define EXT2_S_IXGRP					0x0008 /* group execute */
#define EXT2_S_IROTH					0x0004 /* other read */
#define EXT2_S_IWOTH					0x0002 /* other write */
#define EXT2_S_IXOTH					0x0001 /* other execute */

// Values for i_flags
#define EXT2_SECRM_FL					0x00000001 /* secure deletion */
#define EXT2_UNRM_FL					0x00000002 /* record for undelete */
#define EXT2_COMPR_FL					0x00000004 /* compressed file */
#define EXT2_SYNC_FL					0x00000008 /* synchronous updates */
#define EXT2_IMMUTABLE_FL				0x00000010 /* immutable file */
#define EXT2_APPEND_FL					0x00000020 /* append only */
#define EXT2_NODUMP_FL					0x00000040 /* do not dump/delete file */
#define EXT2_NOATIME_FL					0x00000080 /* do not update .i_atime */
#define EXT2_DIRTY_FL					0x00000100 /* dirty  (modified) */
#define EXT2_COMPRBLK_FL				0x00000200 /* compressed blocks */
#define EXT2_NOCOMPR_FL					0x00000400 /* access raw compressed data */
#define EXT2_ECOMPR_FL					0x00000800 /* compression error */
#define EXT2_BTREE_FL					0x00001000 /* b-tree format directory */
#define EXT2_INDEX_FL					0x00002000 /* hash indexed directory */
#define EXT2_IMAGIC_FL					0x00004000 /* AFS directory */
#define EXT3_JOURNAL_DATA_FL				0x00008000 /* journal file data */
#define EXT2_RESERVED_FL				0x80000000 /* reserved for ext2 library */

// Values for file_type within e2_dirent_t
#define EXT2_FT_UNKNOWN					0 /* unknown file type */
#define EXT2_FT_REG_FILE				1 /* regualr file */
#define EXT2_FT_DIR					2 /* directory file */
#define EXT2_FT_CHRDEV					3 /* character device */
#define EXT2_FT_BLKDEV					4 /* block device */
#define EXT2_FT_FIFO					5 /* buffer file */
#define EXT2_FT_SOCK					6 /* socket file */
#define EXT2_FT_SYMLINK					7 /* symbolic link */

// Values for hash_version within e2_dx_root_t
#define DX_HASH_LEGACY					0
#define DX_HASH_HALF_MD4				1
#define DX_HASH_TEA					2

// Values for h_magic with e2_xattr_header_t
#define EXT2_XATTR_MAGIC				0xEA020000

// Macros to retrieve private ext2fs data from various OS structures
#define EXT2_SUPER(s)					((e2_super_private_t*)((s)->s_private))
#define EXT2_INODE(i)					((e2_inode_private_t*)((i)->i_private))

// Calculate the number of block groups
#define EXT2_BGCOUNT(s)					(EXT2_SUPER(s)->super.s_blocks_count/EXT2_SUPER(s)->super.s_blocks_per_group)

// Values for e2_inode_io command
#define EXT2_READ					0
#define EXT2_WRITE					1

/* The Ext2 Superblock Layout */
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

/* Block Group Descriptor Table */
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

/* Inode Structure */
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
	// any item past here should be checked for against super->s_inode_size and inode->i_extra_isize to amke sure it actually exists!
	u16 i_extra_isize;
	u16 i_checksum_hi;
	u32 i_ctime_extra;
	u32 i_mtime_extra;
	u32 i_atime_extra;
	u32 i_crtime;
	u32 i_crtime_extra;
	u32 i_version_hi;
} e2_inode_t;

/* Linked Directory Entry Structure */
typedef struct _e2_dirent
{
	u32 inode;
	u16 rec_len;
#ifdef EXT2_HAS_DYNAMIC_REV
	u8 name_len;
	u8 file_type;
#else
	u16 name_len;
#endif
	char name[];
} e2_dirent_t;

/* Index Directory Root Information Structure */
typedef struct _e2_dx_root
{
	u32 reserved0;
	u8 hash_version;
	u8 info_length;
	u8 indirect_levels;
	u8 unused_flags;
} e2_dx_root_t;

/* Indexed Directory Entry Structure */
typedef struct _e2_dx_entry
{
	u32 hash;
	u32 block;
} e2_dx_entry_t;

/* Indexed Directory Entry Count and Limit Structure */
typedef struct _e2_dx_limit
{
	u16 limit;
	u16 count;
} e2_dx_limit_t;

/* Extended Attributes Block Header */
typedef struct _e2_xattr_header
{
	u32 h_magic;
	u32 h_refcount;
	u32 h_blocks;
	u32 h_hash;
	u32 reserved[4];
} e2_xattr_header_t;

/* Extended Attributes Entry */
typedef struct _e2_xattr_entry
{
	u8 e_name_len;
	u8 e_name_index;
	u16 e_value_offs;
	u32 e_value_block;
	u32 e_value_size;
	u32 e_hash;
	char e_name[];
} e2_xattr_entry_t;

/* Private Ext2 Filesystem information for runtime filesystem driver */
typedef struct _e2_super_private
{
	e2_superblock_t super; // internal superblock structure
	e2_bg_descr_t* bgtable; // block group table
	u8** inomap; // inode bitmap
	u8* blkmap; // block bitmap
	int dirty;
	spinlock_t rw_lock;
} e2_super_private_t;

/* Private Ext2 Inode Information */
typedef struct _e2_inode_private
{
	e2_inode_t* inode; // the internal inode structure (pointer to inode_data for convenience)
	spinlock_t rw_lock; // lock for the read/write buffer
	char* rw; // the read/write buffer (should be super->s_blocksize bytes long)
	u32* block; // the entire block map for this inode
	int dirty; // 1/0 for dirty/clean. If it is dirty, its internal inode will be written to disk before closing.
	char inode_data[]; // inode data (will occupy super->s_inode_size bytes)
} e2_inode_private_t;

/* Filesystem Callback Functions */
int e2_read_super(struct filesystem* fs, struct superblock* sb, dev_t devid, unsigned long flags, void* data);
int e2_put_super(struct filesystem* fs, struct superblock* sb);
/* Superblock Callback Functions */
int e2_read_inode(struct superblock* sb, struct inode* inode);
void e2_put_inode(struct superblock* sb, struct inode* inode);
int e2_super_flush(struct superblock* sb);
/* Inode Callback Functions */
int e2_inode_lookup(struct inode* inode, struct dentry* dentry);
int e2_inode_mknod(struct inode* inode, const char* name, mode_t mode, dev_t dev);
int e2_inode_flush(struct inode* inode);
/* File Callback Functiosn */
int e2_file_open(struct file* file, struct dentry* dentry, int mode);
int e2_file_close(struct file* file, struct dentry* dentry);
ssize_t e2_file_read(struct file* file, char* buffer, size_t count);
ssize_t e2_file_write(struct file* file, const char* buffer, size_t count);
int e2_file_readdir(struct file* file, struct dirent* dirent, size_t count);

/* Low-Level Filesystem Operations */
int e2_read_block(struct superblock* sb, u32 block, size_t count, char* buffer);
int e2_write_block(struct superblock* sb, u32 block, size_t count, const char* buffer);
u32 e2_alloc_block(struct superblock* sb);
int e2_free_block(struct superblock* sb, u32 block);
ino_t e2_alloc_inode(struct superblock* sb, e2_inode_t* inode_data);
int e2_free_inode(struct superblock* sb, struct inode* inode);
/* Low-Level Inode Operations */
//u32 e2_absolute_block(struct inode* inode, u32 offset); // function superseded by the cached block array within e2_inode_private_t
int e2_inode_read_block_map(struct inode* inode, u32* buffer); // read the entire block map (direct, indirect, doubly, and triply)
int e2_inode_write_block_map(struct inode* inode, u32* buffer); // write the entire block map
ssize_t e2_inode_resize(struct inode* inode, size_t newsize);
ssize_t e2_inode_io(struct inode* inode, int cmd, off_t offset, size_t size, char* buffer);
/* Middle-Level Inode Operations */
int e2_inode_link(struct inode* parent, const char* name, struct inode* inode); // associate the given with the inode within the given parent directory
int e2_inode_unlink(struct inode* inode, const char* name); // unlink the given name from within the given directory

// Filesystem Operations Structures (defined in module.c)
extern struct inode_operations e2_inode_operations;
extern struct file_operations e2_default_file_operations;
extern struct superblock_operations e2_superblock_ops;
extern struct filesystem_operations e2_filesystem_ops;
extern struct filesystem e2_filesystem;

void e2_install_filesystem( void );

#endif