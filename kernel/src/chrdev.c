#include "chrdev.h"
#include "error.h"
#include "kmem.h"

static chrdev_t* chrdev[256];

int register_chrdev(unsigned int major, const char* name, struct file_operations* fops)
{
	if( major >= 256 ){
		return -EEXIST;
	}
	if( chrdev[major] != NULL ){
		return -EEXIST;
	}
	
	chrdev[major] = (chrdev_t*)kmalloc(sizeof(chrdev_t));
	if( chrdev[major] == NULL ){
		return -ENOMEM;
	}
	
	strncpy(chrdev[major]->name, name, 64);
	chrdev[major]->name[63] = 0;
	
	chrdev[major]->fops = fops;
	
	return 0;
}

int unregister_chrdev(int major)
{
	if( major >= 256 ){
		return -EEXIST;
	}
	
	if( chrdev[major] == NULL ) return 0;
	
	kfree(chrdev[major]);
	chrdev[major] = 0;
	
	return 0;
}

struct file_operations* get_chrdev_fops(int major)
{
	if( major >= 256 ){
		return NULL;
	}
	return chrdev[major]->fops;
}
		