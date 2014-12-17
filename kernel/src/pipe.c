#include "pipe.h"
#include "fs.h"
#include "dentry.h"
#include "error.h"
#include <fcntl.h>
#include <sys/stat.h>

struct file_operations pipe_operations = {
	.open = pipe_open, .close = pipe_close,
	.read = pipe_read, .write = pipe_write,
};

static list_t pipe_list = LIST_INIT(pipe_list);

static pipe_t* find_open_pipe(struct inode* inode)
{
	list_t* iter;
	list_for_each(iter, &pipe_list){
		pipe_t* pipe = list_entry(iter, pipe_t, link);
		if( pipe->inode == inode ){
			return pipe;
		}
	}
	return NULL;
}

int pipe_open(struct file* file, struct dentry* dentry ATTR((unused)), int mode)
{
	pipe_t* pipe = find_open_pipe(file_inode(file));
	
	if( pipe == NULL )
	{
		pipe = (pipe_t*)kmalloc(sizeof(pipe_t));
		if( !pipe ){
			return -ENOMEM;
		}

		memset(pipe, 0, sizeof(*pipe));

		pipe->buffer = (char*)kmalloc(KERNEL_PIPE_LENGTH);
		if( pipe->buffer == NULL ){
			kfree(pipe);
			return -ENOMEM;
		}

		pipe->length = KERNEL_PIPE_LENGTH;

		pipe->read = pipe->buffer;
		pipe->write = pipe->buffer;
		pipe->nreaders = 0;
		pipe->nwriters = 0;
		pipe->inode = file_inode(file);
		spin_init(&pipe->wrlock);
		spin_init(&pipe->rdlock);
		INIT_LIST(&pipe->link);
		pipe->reader = NULL;

		list_add(&pipe->link, &pipe_list);
	}
	
	// save the pipe pointer
	file->f_private = pipe;
	
	if( (mode & O_RDONLY) || (mode & O_RDWR) ){
		pipe->nreaders++;
	}
	if( (mode & O_WRONLY) || (mode & O_RDWR) ){
		pipe->nwriters++;
	}
	return 0;
}

int pipe_close(struct file* file, struct dentry* dentry ATTR((unused)))
{
	pipe_t* pipe = (pipe_t*)(file->f_private);
	
	if( (file->f_status & O_RDONLY) || (file->f_status & O_RDWR) ){
		pipe->nreaders--;
	}
	
	if( (file->f_status & O_WRONLY) || (file->f_status & O_RDWR) ){
		pipe->nwriters--;
	}
	
	if( (pipe->nwriters == 0) && (pipe->nreaders == 0) )
	{
		list_rem(&pipe->link);
		kfree(pipe->buffer);
		kfree(pipe);
	}
	
	return 0;
}

ssize_t pipe_read(struct file* file, char* buffer, size_t count)
{
	pipe_t* pipe = (pipe_t*)file->f_private;
	ssize_t n = 0;
	
	spin_lock(&pipe->rdlock);
	pipe->reader = current;
	
	while( n < (ssize_t)count ){
		// are all the writers closed? if so, there isn't going to be any more data
		if( pipe->nwriters == 0 && pipe->read == pipe->write ){
			spin_unlock(&pipe->rdlock);
			// if we never got any bytes at all, this is the EOF
			if( n == 0 ){
				return -1;
			} else {
				return n;
			}
		}
		
		// wait for data from the pipe
		if( pipe->read == pipe->write ){
			task_wait(current, TF_WAITIO);
		}
		
		// read as many bytes as possible/wanted
		while( pipe->read != pipe->write && (size_t)n < count){
			*buffer = *pipe->read;
			pipe->read++;
			buffer++;
			n++;
			// wrap the buffer
			if( pipe->read == &pipe->buffer[pipe->length] ){
				pipe->read = pipe->buffer;
			}
		}
	}
	
	pipe->reader = NULL;
	spin_unlock(&pipe->rdlock);
	
	return n;
}

ssize_t pipe_write(struct file* file, const char* buffer, size_t count)
{
	pipe_t* pipe = (pipe_t*)file->f_private;
	ssize_t n = 0;
	
	spin_lock(&pipe->wrlock);
	
	for(; n < (ssize_t)count; ++n){
		*pipe->write = buffer[n];
		pipe->write++;
		if( pipe->write == &pipe->buffer[pipe->length] ){
			pipe->write = pipe->buffer;
		}
	}
	
	spin_unlock(&pipe->wrlock);
	
	if( pipe->reader && T_WAITING(pipe->reader) ){
		task_wakeup(pipe->reader);
	}
	
	return n;
}