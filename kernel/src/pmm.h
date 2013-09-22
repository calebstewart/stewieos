#ifndef _PMM_H_
#define _PMM_H_

#include "kernel.h"
#include "paging.h"

u32 find_free_frame( void );					// Find the next free frame in memory
void reserve_frame(u32 frame);					// Reserve a frame in memory
void release_frame(u32 frame);					// Release a previously reserved frame

void alloc_frame(page_t* page, int user, int rw);		// Allocate a frame for a virtual page
void free_frame(page_t* page);					// Release the frame which belongs to a virtual page

#endif