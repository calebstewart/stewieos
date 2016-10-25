#ifndef _PIPE_H_
#define _PIPE_H_

#include "stewieos/kernel.h"
#include "stewieos/task.h"
#include "stewieos/spinlock.h"
#include "stewieos/fs.h"
#include "stewieos/linkedlist.h"

#define KERNEL_PIPE_LENGTH 1024

typedef struct _pipe
{
	char* buffer;
	size_t length;
	char* read;
	char* write;
	size_t nreaders;
	size_t nwriters;
	spinlock_t wrlock;
	spinlock_t rdlock;
	struct inode* inode; // the inode which this pipe refers to on disk
	struct task* reader;
	list_t link;
} pipe_t;

int pipe_open(struct file* file, struct dentry* dentry, int mode);
ssize_t pipe_read(struct file* pipe, char* buffer, size_t count);
ssize_t pipe_write(struct file* pipe, const char* buffer, size_t count);
int pipe_close(struct file* file, struct dentry* dentry);

extern struct file_operations pipe_operations;

#endif