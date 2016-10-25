#ifndef _PMM_H_
#define _PMM_H_

#include "stewieos/kernel.h"
#include "stewieos/paging.h"

#define FRAME_TO_ADDR(frame) ((frame) << 12)
#define ADDR_TO_FRAME(addr) ((addr) >> 12)

u32 find_free_frame( void );					// Find the next free frame in memory
void reserve_frame(u32 frame);					// Reserve a frame in memory
void release_frame(u32 frame);					// Release a previously reserved frame

void clone_frame(page_t* dest, page_t* src);			// Map a page to the same frame as antoher page
void alloc_frame(page_t* page, int user, int rw);		// Allocate a frame for a virtual page
void free_frame(page_t* page);					// Release the frame which belongs to a virtual page

void push_frame(u32 frame);					// push (free) a frame
u32 pop_frame( void );						// pop (alloc) a frame

#endif