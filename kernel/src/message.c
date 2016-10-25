#include "stewieos/task.h"
#include "stewieos/spinlock.h"
#include "stewieos/linkedlist.h"
#include "stewieos/error.h"

int sys_message_send(pid_t pid, unsigned int type, const char* what, size_t length)
{
	// make sure the length is within reason
	if( length > MESG_MAX_LENGTH ){
		return -E2BIG;
	}
	
	// lookup the task
	struct task* who = task_lookup(pid);
	if( who == NULL ){
		return -ENOENT;
	}
	
	// allocate a new message and container
	message_container_t* container = (message_container_t*)kmalloc(sizeof(message_container_t) + length);
	if( container == NULL ){
		return -ENOMEM;
	}
	
	// prepare the message for sending
	INIT_LIST(&container->link);
	container->message.from = current->t_pid;
	container->message.id = type;
	container->message.length = length;
	memcpy(container->message.data, what, length);
	
	// lock the message queue
	spin_lock(&who->t_mesgq.lock);
	
	// add the message to the end of the list
	list_add_before(&container->link, &who->t_mesgq.queue);
	
	// unlock the message queue
	spin_unlock(&who->t_mesgq.lock);
	
	// wake up the task if needed
	if( who->t_flags & TF_WAITMESG ){
		task_wakeup(who);
	}
	
	return 0;
}

int sys_message_pop(message_t* message, unsigned int id, unsigned int flags)
{
	list_t* iter = NULL;
	
	// lock the queue
	spin_lock(&current->t_mesgq.lock);
	
	// no messages to return
	if( list_empty(&current->t_mesgq.queue) ){
		spin_unlock(&current->t_mesgq.lock);
		if( flags & MESG_POP_WAIT ){
			// when we wake up we will have a message
			task_wait(current, TF_WAITMESG);
			spin_lock(&current->t_mesgq.lock);
		} else {
			return 0;
		}
	}
	
	// iterate over all messages
	list_for_each(iter, &current->t_mesgq.queue){
		message_container_t* container = list_entry(iter, message_container_t, link);
		// if we didn't care about id or if this matches
		if( container->message.id == id || id == MESG_ANY ){
			// grab the message
			memcpy(message, &container->message, sizeof(message_t) + container->message.length);
			// Remove the message unless told otherwise
			if( !(flags & MESG_POP_LEAVE) ){
				list_rem(&container->link);
				kfree(container);
			}
			// unlock the queue
			spin_unlock(&current->t_mesgq.lock);
			// return success
			return 1;
		}
	}
	
	// unlock the queue
	spin_unlock(&current->t_mesgq.lock);
	
	// no messages matching your id were found
	return -ENOENT;
}