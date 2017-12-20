#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "./mm.h"
#include "./memlib.h"
#include "./mminline.h"

#define MAX(x, y) ((x) > (y)? (x) : (y))
#define MIN(X, Y)  ((X) < (Y) ? (X) : (Y))

block_t *prologue;
block_t *epilogue;






// rounds up to the nearest multiple of WORD_SIZE
static inline size_t align(size_t size) {
    return (((size) + (WORD_SIZE - 1)) & ~(WORD_SIZE - 1));
}

int mm_check_heap(void);
 
/*
 *                             _       _ _
 *     _ __ ___  _ __ ___     (_)_ __ (_) |_
 *    | '_ ` _ \| '_ ` _ \    | | '_ \| | __|
 *    | | | | | | | | | | |   | | | | | | |_
 *    |_| |_| |_|_| |_| |_|___|_|_| |_|_|\__|
 *                       |_____|
 *
 * initializes the dynamic storage allocator (allocate initial heap space)
 * arguments: none
 * returns: 0, if successful
 *         -1, if an error occurs

mm_init first sets flist_first to be null, then expands the heap to fit it, the prologue and the epilogue, making sure to use
mminline functions to properly set the size and allocation of the first blocks. It also inserts an initial free_block into the free list.

It then calls check_heap to ensure that the heap is valid from the start.
 */
int mm_init(void) {
	flist_first = NULL;
    if ((prologue = mem_sbrk(TAGS_SIZE)) == (void *)-1){
		perror("mem_sbrk");
		return -1;
	}
	block_set_size_and_allocated(prologue, TAGS_SIZE, 1);
	block_t *initial;
	if ((initial = mem_sbrk(MINBLOCKSIZE)) == (void *)-1){
		perror("mem_sbrk");
		return -1;
	}

	block_set_size_and_allocated(initial, MINBLOCKSIZE, 0);
	insert_free_block(initial);

	if ((epilogue = mem_sbrk(TAGS_SIZE)) == (void *)-1){
		perror("mem_sbrk");
		return -1;
	}

	block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
	

    return 0;
}

/*     _ __ ___  _ __ ___      _ __ ___   __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '_ ` _ \ / _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | | | | | (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_| |_| |_|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * allocates a block of memory and returns a pointer to that block's payload
 * arguments: size: the desired payload size for the block
 * returns: a pointer to the newly-allocated block's payload (whose size
 *          is a multiple of ALIGNMENT), or NULL if an error occurred

first check's for proper size input, and aligns the size accordingly
if a fit can be found, place the block there and return its payload

if a fit can't be found

extend the heap, and then place the allocated block in the new space and return the payload


 */
void *mm_malloc(size_t size) {
	
	size_t asize;
	size_t extend_size;
	block_t *bp;

	if (size <= 0){
		return NULL;
	}
	if (size <= TAGS_SIZE){
		asize = 2*TAGS_SIZE;
	} else {
		asize = align(size) + TAGS_SIZE;
	}


	if ((bp = find_fit(asize)) != NULL){
		bp = place(bp, asize);
		return bp->payload;
	}
	extend_size = MAX(asize, MINBLOCKSIZE);
	if ((bp = mm_extend_heap(extend_size)) == NULL){
		return NULL;
	}	
	assert(!block_allocated(bp));
	bp = place(bp, asize);
	
	return bp->payload;
}

/*                              __
 *     _ __ ___  _ __ ___      / _|_ __ ___  ___
 *    | '_ ` _ \| '_ ` _ \    | |_| '__/ _ \/ _ \
 *    | | | | | | | | | | |   |  _| | |  __/  __/
 *    |_| |_| |_|_| |_| |_|___|_| |_|  \___|\___|
 *                       |_____|
 *
 * frees a block of memory, enabling it to be reused later
 * arguments: ptr: pointer to the block's payload
 * returns: nothing
 */
void mm_free(void *ptr) {
	// TODO
	block_t *block = payload_to_block(ptr);
	block_set_allocated(block, 0);
	insert_free_block(block);
	if (!block_allocated(block_next(block)) || !block_allocated(block_prev(block))){
	coalesce(block);
	}
}

/*
 *                                            _ _
 *     _ __ ___  _ __ ___      _ __ ___  __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '__/ _ \/ _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | |  __/ (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_|  \___|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * reallocates a memory block to update it with a new given size
 * arguments: ptr: a pointer to the memory block's payload
 *            size: the desired new block size
 * returns: a pointer to the new memory block's payload
*
*
*
*
 *
 */
void *mm_realloc(void *ptr, size_t size) {
	// edge case handling
	if (!ptr){
		return mm_malloc(size);
	}
	if (size == 0){
		mm_free(ptr);
		return ptr;
	} 

	//align requested size
	size_t req_size = align(size) + TAGS_SIZE;

	//get block pointer from payload argument
	block_t *block = payload_to_block(ptr);

	//calculate original size slash initial available size
	size_t original_size;
	size_t available_size =  block_size(block) - TAGS_SIZE;
	original_size = available_size;

	if (original_size >= req_size){
		return ptr;
	}

	//if next block is free, add their size to available size
	if (!block_allocated(block_next(block))){
		available_size += block_size(block_next(block));
	}

	//determine size of payload to copy
	size_t to_copy = original_size;

	//create a new block
	block_t *new_block = NULL;

	//if requested size is bigger than available size even with neighbors
	if (req_size > available_size){

	new_block = payload_to_block(mm_malloc(req_size)); //call mm_malloc to find a new place for the new block or extend the heap
	memmove(new_block->payload, ptr, to_copy); //use memmove to copy the payload from the original block to the new block
	mm_free(ptr);	

	return new_block->payload; //return the payload of the new block
	} 
		if (!block_allocated(block_next(block))){
	
			 size_t full_size = original_size + block_size(block_next(block));

			 pull_free_block(block_next(block));
			 block_set_size(block, full_size+16);
		
		}
		return block->payload;


}

/*
 * checks the state of the heap for internal consistency and prints informative
 * error messages
 * arguments: none
 * returns: 0, if successful
 *          nonzero, if the heap is not consistent
 */
int mm_check_heap(void) {
	int ret = 0;
	block_t *checkblock = mem_heap_lo();

	while (block_next(checkblock) != epilogue){
		
		if (block_allocated(checkblock) != block_end_allocated(checkblock)){
			fprintf(stderr, "%s%p%s%u\n", "Error: Block header and footer allocated bit are not the same at ", (void *)checkblock, " of size ", (int) block_size(checkblock));
			ret = 1;
		}

		if (block_size(checkblock) != block_end_size(checkblock)){
			fprintf(stderr, "%s%p%s%u\n", "Error: Block header and footer size tag are not the same at ", (void *)checkblock, " of size ", (int) block_size(checkblock));
			ret = 1;
		}

		block_t *next = block_next(checkblock);
		if (block_prev(next) != checkblock){
			fprintf(stderr, "%s%p%s%u\n", "Error: order of heap is incorrect at ", (void *)checkblock, " of size ", (int) block_size(checkblock));
			ret = 1;
		}
		 char * hi = mem_heap_hi();
		 char * lo =  mem_heap_lo();
		if ((size_t)checkblock > (size_t)hi){
			fprintf(stderr, "%s%p%s%u\n", "Error: Block out of bounds at ", (void *)checkblock, " of size ", (int) block_size(checkblock));
			ret = 1;
		}
		if ((size_t)checkblock < (size_t)lo){
			fprintf(stderr, "%s%p%s%u\n", "Error: Block out of bounds at ", (void *)checkblock, " of size ", (int) block_size(checkblock));
			ret = 1;
		}
		if (!block_allocated(checkblock)){
			block_t *free = flist_first;
			free = block_next_free(free);
			ret = 1;
			while (free != flist_first ){
				
				if (checkblock == free){
					ret = 0;
				}
				free = block_next_free(free);
			}
			if (ret == 1){
				fprintf(stderr, "%s%p%s%u\n", "Error: Free block not in free list at ", (void *)checkblock, " of size", (int) block_size(checkblock));

			}
		}
		if (block_size(checkblock) < MINBLOCKSIZE && checkblock != prologue && checkblock != epilogue){
			fprintf(stderr, "%s%p%s%u\n", "Error! Block too small at ", (void *)checkblock, " of size ", (int) block_size(checkblock));
		}
		checkblock = next;
	}

	//checking heap going the other way to ensure end tags are correct

	checkblock = epilogue;
	while(block_prev(checkblock) != prologue){
		block_t *prev = block_prev(checkblock);


		if (block_allocated(checkblock) != block_end_allocated(checkblock)){
			fprintf(stderr, "%s%p%s%u\n", "Error: Block header and footer allocated bit are not the same at ", (void *)checkblock, " of size ", (int)block_size(checkblock));
			ret = 1;
		}

		if (block_size(checkblock) != block_end_size(checkblock)){
			fprintf(stderr, "%s%p%s%u\n", "Error: Block header and footer size tag are not the same at ", (void *)checkblock, " of size ", (int)block_size(checkblock));
			ret = 1;
		}



		if (block_next(prev) != checkblock){
			fprintf(stderr, "%s%p%s%u\n", "Error: Alignment of heap is incorrect at  ", (void *)checkblock, " of size ",  (int)block_size(checkblock));
			ret = 1;
		}
		checkblock = prev;
	}

	//checking free_list to make sure they're all correctly tagged as free and have been coalesced

	checkblock = flist_first;
	if (checkblock != NULL){
	checkblock = block_next_free(checkblock);
	while (checkblock != flist_first){


		if (block_allocated(checkblock) != 0 || block_end_allocated(checkblock) != 0){
			fprintf(stderr, "%s%p%s%u\n", "Error: Free blocks not correctly tagged as free at ", (void *)checkblock, " of size", (int) block_size(checkblock));
			ret = 1;
		}

		if (block_next(checkblock) == block_next_free(checkblock)){
			 fprintf(stderr, "%s%p%s%u\n", "Error: Free blocks not coelesced at ", (void *)checkblock, " of size", (int) block_size(checkblock));
			 ret = 1;
		}

		if (block_allocated(block_next_free(checkblock)) || block_allocated(block_prev_free(checkblock))){
			fprintf(stderr, "%s%p%s%u\n", "Error: Free block pointers incorrect at ", (void *)checkblock, " of size", (int) block_size(checkblock));
		}
		checkblock = block_next_free(checkblock);
	}
}
    return ret;
}


/*Extend heap by words bytes
*
* using epilogue pointer to return pointer to last free_block, this function extends the heap by words, 
* then swaps the epilogue and bp pointers 
* extends the heap with a free block to make rest of implementation work better
*/
void *mm_extend_heap(size_t words){

	block_t *bp;
	size_t size = words;

	if ((bp = mem_sbrk(size)) == (void *)-1){
		perror("mem_sbrk 1");
		return NULL;
	}
	bp = epilogue;
	block_set_size_and_allocated(bp, size, 0);
	insert_free_block(bp);
	epilogue = block_next(bp);
	block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
	if (!block_allocated(block_next(bp)) || !block_allocated(block_prev(bp))){
		return coalesce(bp);
	}
	return bp;
}


/*
* merges adjacent free_blocks by pulling from freelist and adjusting size via block_set_size() 
*
*
*
*returns pointer to whole free block
*/
void *coalesce(void *bp){

	size_t prev_alloc = block_allocated(block_prev(bp));
	size_t next_alloc = block_allocated(block_next(bp));
	size_t size = block_size(bp);
	assert(!block_allocated(bp));
	
	if (block_allocated(bp)){ //CASE 0: when the block inputted is allocated
		return bp;
	}

	if (prev_alloc && next_alloc){ //CASE 1: When neither of the adjacent blocks are free
		return bp;
	}

	else if (prev_alloc && !next_alloc){ //CASE 2: When the next block is free, add its size to size, pull it from the free list and reset the size of bp
		size += block_size(block_next(bp));
		pull_free_block(block_next(bp));
		block_set_size(bp, size);
		return bp;
	}

	else if (!prev_alloc && next_alloc){ //CASE 3: When the prev block is free, add its size, store its address in, set ret's size to include both, pull bp, return
		size += block_size(block_prev(bp));
		block_t *ret = block_prev(bp);
		pull_free_block(bp);
		block_set_size(ret, size);
		assert(!block_allocated(ret));
		if (block_next_free(ret) == 0){
			insert_free_block(ret);
		}
		return ret;
	}

	else if (!prev_alloc && !next_alloc){ //CASE 4: When both adjacent blocks are free, store the prev block in ret, add up sizes, pull bp and next, set ret's size, return
		block_t *ret = block_prev(bp);
		size += block_size(block_next(bp)) + block_size(block_prev(bp));
		pull_free_block(block_next(bp));
		pull_free_block(bp);
		block_set_size(ret, size);
		assert(!block_allocated(ret));
		return ret;
	}
	fprintf(stderr, "well here's your error\n");
	return NULL;
}


/*
*Searches through freelist to see if there is a block that is as big or smaller than asize (already aligned)
*
*returns NULL is no fit is found, otherwise returns pointer to free block that can fit asize
*/
void *find_fit(size_t asize){
	block_t *check = flist_first;
	block_t *bp = NULL;

	if (check != NULL){
	if (block_size(check)  >= asize){
			return check;
		}
		
		assert(!block_allocated(check));
		check = block_next_free(flist_first);

	while (check != flist_first){
		if (block_size(check)   >= asize){
			return check;
		}
		check = block_next_free(check);
	}
	} 
	
	return bp;

}



void *place(void *bp, size_t asize){
	size_t original_size = block_size(bp);	
	if (asize < original_size){
		size_t new_size = original_size - asize;
		if (new_size < MINBLOCKSIZE){
			pull_free_block(bp);
			block_set_allocated(bp, 1);
			return bp;
		}
		pull_free_block(bp);
		block_set_size_and_allocated(bp, asize, 1);
		block_t *new_free = block_next(bp);
		block_set_size_and_allocated(new_free, new_size, 0);
		insert_free_block(new_free);
		assert(!block_allocated(new_free));
		return bp;

	} else if (asize == original_size){

		pull_free_block(bp);
		block_set_allocated(bp, 1);
		return bp;
	}
	else {
		fprintf(stderr, "%s", "Invalid size for free block to become allocated"); 
		return NULL;
	}
	
}