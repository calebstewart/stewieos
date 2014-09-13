#ifndef _CHRDEV_H_
#define _CHRDEV_H_

#include "fs.h"

typedef struct _chrdev
{
	char name[64];
	struct file_operations* fops;
} chrdev_t;

int register_chrdev(unsigned int major, const char* name, struct file_operations* fops);
int unregister_chrdev(int major);

struct file_operations* get_chrdev_fops(int device);

#endif