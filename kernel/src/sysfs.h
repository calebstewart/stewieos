#ifndef _SYSFS_H_
#define _SYSFS_H_

#include "kernel.h"
#include "fs.h"
#include "linkedlist.h"

struct sysfs_node;
struct sysfs_file;
struct sysfs_dir;
struct sysfs_meta;
struct sysfs_hook;

typedef struct sysfs_node sysfs_node_t;
typedef struct sysfs_file sysfs_file_t;
typedef struct sysfs_dir sysfs_dir_t;
typedef struct sysfs_meta sysfs_meta_t;
typedef struct sysfs_hook sysfs_hook_t;

struct sysfs_meta
{
	char name[64];
	time_t ctime;
	time_t mtime;
	time_t atime;
	mode_t mode;
};

struct sysfs_node
{
	sysfs_meta_t meta;
	list_t entry;
	union {
		sysfs_file_t file;
		sysfs_dir_t dir;
	};
};

struct sysfs_file
{
	void* data;
};

struct sysfs_dir
{
	list_t entries;
};

// Kernel-Level Sysfs methods
sysfs_node_t* sysfs_get_node(const char* path);
sysfs_node_t* sysfs_lookup(sysfs_node_t* node, const char* name);
sysfs_node_t* sysfs_create(sysfs_node_t* node, const char* name, mode_t mode);
sysfs_node_t* sysfs_mkdir(sysfs_node_t* node, const char* name);

// Filesystem abstractions
int sysfs_read_super(struct filesystem* fs, struct superblock* super, dev_t devid, unsigned long options, void* data);
int sysfs_put_super(struct filesystem* fs, struct superblock* super);
// Superblock abstractions
int sysfs_read_inode(struct superbock* super, struct inode* inode);
void sysfs_put_inode(struct superblock* super, struct indoe* inode);
// Inode Abstractions
int sysfs_inode_lookup(struct inode* inode, struct dentry* dentry);
// File Abstractions
int sysfs_file_open(struct file* file, struct dentry* dentry, int mode);
int sysfs_file_close(struct file* file, struct dentry* dentry);
ssize_t sysfs_file_read(struct file* file, char* buffer, size_t count);
ssize_t sysfs_file_write(struct file* file, const char* buffer, size_t count);
int sysfs_file_readdir(struct file* file, struct dirent* dirent, size_t count);
int sysfs_file_fstat(struct file* file, struct stat* stat);


#endif