/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <basic.h>
#include <stdbool.h>
#include "debug.h"
#include "sfmm.h"
#include <errno.h>

static long double max_aggregate = -1;

void *sf_malloc(sf_size_t size) {
    //First check if the size of the argument is 0: if it is, then return NULL w/o setting errno
    if (size == 0) return NULL;
    //Then, check if the heap has been initialized yet: if the starting and ending
    //address of the heap are the same, then call memgrow and set up prologue/epilogue
    if (sf_mem_start() == sf_mem_end()){
        //Initialize the main free lists
        int i;
        //Make sure the next and prev pointers of each
        //dummy node in free list heads point back to them
        for (i=0; i<NUM_FREE_LISTS; i++){
            sf_free_list_heads[i].body.links.next = sf_free_list_heads[i].body.links.prev = 
            &sf_free_list_heads[i];
        }

        void* heap_start = sf_mem_start();
        sf_mem_grow();
        *(sf_header*)heap_start = (sf_header)0;
        create_alloc_block(sf_mem_start(), 32, 0, false, false); //Initialize prologue
        create_alloc_block(sf_mem_end() - 16, 0, 0, false, false); //Initialize epilogue
        sf_block* free_block = create_free_block(sf_mem_start() + 32, PAGE_SZ-48, 0, false, false, true);
        sf_show_heap();
        //Then, add this free block to the respective free list
        int index = determine_freelist_index((double)GET_BLOCK_SIZE((free_block->header ^ MAGIC))/32);
        sf_block* dummy = &sf_free_list_heads[index];
        dummy->body.links.next = free_block;
        dummy->body.links.prev = free_block;
        free_block->body.links.prev = dummy;
        free_block->body.links.next = dummy;
        sf_show_heap();
    }
    debug("Reached start of satisfying malloc request \n");
    //First, determine the block size by adding up the payload and header 
    //and rounding to nearest multiple of 16
    int block_size = HEADER_SIZE + size;
    if (block_size % 16 != 0) block_size += GET_ROUND_NUM(block_size);
    if (block_size == 16) block_size*=2;
    //Then, check if the quick lists contain any exact sized blocks that can be allocated
    debug("Starting check for quick lists...\n");
    int startIndex = GET_QK_INDEX(block_size);
    sf_block* block; 
    //If this block can be put into a quick list and the quick list has blocks, then allocate 
    if (startIndex >= 0 && startIndex < NUM_QUICK_LISTS && sf_quick_lists[startIndex].length > 0) {
        block=sf_quick_lists[startIndex].first;
        int header = (block->header ^ MAGIC); //Make sure to XOR the header before accessing it
        //Update the quick list before allocating
        sf_quick_lists[startIndex].first = block->body.links.next; 
        sf_quick_lists[startIndex].length--;
        //Then allocate the block and return the address of the payload
        block = create_alloc_block(block, block_size, size, true, GET_PREV_BIT(header));
        get_aggregate_payload();
        return block->body.payload;
    }
    debug("Checking for block in free list..\n");
    //Then, check if theres an available block in the main free lists
    for(startIndex = determine_freelist_index((double)block_size/32); startIndex<NUM_FREE_LISTS; startIndex++){
        sf_block* sentinel = &sf_free_list_heads[startIndex];
        sf_block* head = sentinel->body.links.next;
        while (head != sentinel){
            int header = (head->header ^ MAGIC); //Make sure to XOR the header before accessing it
            bool prevalloc = GET_PREV_BIT(header);
            if (GET_BLOCK_SIZE(header) >= block_size){
                //If a sufficient block is found, remove the free block from this list first
                remove_from_list(head);
                sf_show_heap();
                //Split the block if possible, using upper half to allocate the block
                split_block(head, block_size, true);
                head = create_alloc_block(head, block_size, size, false, prevalloc);
                get_aggregate_payload();
                return head->body.payload;
            }
            head = head->body.links.next;
        }
    }
    debug("Calling grow heap...\n");
    //If no sufficient free block was found, call sf mem grow until the block size is met
    void* payload = grow_heap(block_size, size);
    if (payload == NULL) sf_errno = 12; //If error, set errno to ENONEM 
    else get_aggregate_payload();
    return payload;
}



sf_block* create_alloc_block(void* start_address, int block_size, int payload_size, bool qk, bool prevalloc){
    sf_block* new_block;
    new_block = start_address; 

    adjust_header(new_block, block_size, payload_size, true, qk, prevalloc);

    //Upon allocating a block, update the prevalloc bit to of the block following it to 1
    //Make sure this isn't done for prologue or epilogue
    if (start_address != sf_mem_start() && start_address != (sf_mem_end() - 16)){
        //Make sure to use void or char pointers whenever advancing mem locations
        sf_block* block = start_address + block_size; 
        block->header = ((block->header ^ MAGIC) | PREV_BLOCK_ALLOCATED) ^ MAGIC;
    }
    return new_block;
}


sf_block* create_free_block(void* start_address, int block_size, int payload_size, bool alloc, bool qk, bool prevalloc){
    sf_block* new_block;
    new_block = start_address; 

    adjust_header(new_block, block_size, payload_size, alloc, qk, prevalloc);
    new_block->body.links.next = new_block->body.links.prev = NULL; 

    //Go to the last 8 bytes in this block and set the footer  
    //start_address is at the footer of the prev block so can add 
    //block size to get to this block's footer
    sf_footer* footer = start_address + GET_BLOCK_SIZE((new_block->header^MAGIC)); 
    *footer = new_block->header; //footer automatically obfuscated 

    //Upon creating a free block, update the prevalloc bit of the block following it to 0
    //Make sure this isn't done for prologue or epilogue (initial free block will get set as prevalloc = 1 later)
    if (start_address != sf_mem_start() && start_address != (sf_mem_end() - 16)){
        //Make sure to use void or char pointers whenever advancing mem locations
        sf_block* block= start_address + block_size; 
        if (GET_PREV_BIT((block->header ^ MAGIC)) != 0) block->header = ((block->header ^ MAGIC) - PREV_BLOCK_ALLOCATED) ^ MAGIC;
    }
    return new_block;
}

void adjust_header(sf_block* new_block, int block_size, int payload_size, bool alloc, bool qk, bool prevalloc){
    new_block->header = 0; 
    sf_header payload = ((sf_header)payload_size) << 32;
    new_block->header+=payload;
    new_block->header = SET_BLOCK_SIZE((new_block->header), block_size);
    if (alloc) new_block->header |= THIS_BLOCK_ALLOCATED;
    if (qk) new_block->header |= IN_QUICK_LIST;
    if (prevalloc) new_block->header |= PREV_BLOCK_ALLOCATED;
    new_block->header ^= MAGIC; //Make sure to obfuscate the header 
}

/**
 * @brief 
 * determine_freelist_index determines which free list a block goes in 
 * through performing a log base 2 operation on the parameter
 * 
 * @param size the number of M's in a block size (ie, 32 in 32M) 
 * @return the index of the free list 
 */
int determine_freelist_index(double size){
    int counter = 0;
    while(size > 1){
        counter++;
        size/=2;
    }
    return counter;
}

void remove_from_list(sf_block* blk){
    sf_block* prev = blk->body.links.prev;
    if (prev == NULL) return;
    prev->body.links.next = blk->body.links.next;
    sf_block* next = prev->body.links.next;
    if (next != NULL) next->body.links.prev = prev;
    blk->body.links.next = blk->body.links.prev = NULL;
}

void split_block(sf_block* block, int block_size, bool no_coalesce){
    //Try to split the block first if possible: first half for allocating, second block for free block
    int remainder = GET_BLOCK_SIZE((block->header^MAGIC))-block_size;
    sf_block *rem_blk;
    if (remainder >= 32) {
        rem_blk = (void *)block + block_size; //Move to the second half of the block: use void casting to do properly
        adjust_header(rem_blk, remainder, 0, false, false, true);
        freeListAdder((void *)rem_blk, remainder, true, no_coalesce); //When splitting a block, the free part goes back into main free lists
    }
}

/**
 * @brief coalesce tries to coalesce the current block 
 * with the one before and/or after it
 * 
 * @param block_address address of the block to attempt to coalesce
 */
sf_block* coalesce(sf_block* curr){
    int prev_alloc = GET_PREV_BIT((curr->header^MAGIC));
    //Make sure to cast to void to move up (curr's block size) bytes
    sf_block* next_blk = (void*)curr + GET_BLOCK_SIZE((curr->header^MAGIC));
    sf_show_block(next_blk);
    int next_alloc = GET_ALLOC_BIT((next_blk->header^MAGIC));

    sf_block* hdr_blk; sf_block* ftr_blk; int block_size = 0;

    //Case 1: prev and next are both free, so all 3 blocks are coalesced
    if (!prev_alloc & !next_alloc){
        sf_block* prev = (void*)curr-GET_BLOCK_SIZE((curr->prev_footer^MAGIC));
        //If all 3 are coalesced, prev's header and next's footer have to be updated
        block_size = GET_BLOCK_SIZE((prev->header^MAGIC)) + GET_BLOCK_SIZE((curr->header^MAGIC)) + GET_BLOCK_SIZE((next_blk->header ^ MAGIC));
        hdr_blk = prev; ftr_blk = next_blk;
    }
    //Case 2: only prev is free
    else if (!prev_alloc){
        //Make sure to cast to void to move back (prev's block size) bytes
        sf_block* prev = (void*)curr-GET_BLOCK_SIZE((curr->prev_footer^MAGIC));
        //If only prev, update prev's header and curr's footer
        block_size = GET_BLOCK_SIZE((curr->prev_footer^MAGIC)) + GET_BLOCK_SIZE((curr->header^MAGIC));
        hdr_blk = prev; ftr_blk = curr;
    }
    //Case 3: only next is free
    else if (!next_alloc){
        //If only next, update curr's header and next's footer        
        block_size = GET_BLOCK_SIZE((curr->header^MAGIC)) + GET_BLOCK_SIZE((next_blk->header ^ MAGIC));
        hdr_blk = curr; ftr_blk = next_blk;
    }
    //Remove prev/next from their free lists before updating the header and footer of the new block
    if (block_size > 0){
        if (hdr_blk != curr) remove_from_list(hdr_blk);
        if (ftr_blk != curr) remove_from_list(ftr_blk);
        hdr_blk->header = SET_BLOCK_SIZE((hdr_blk->header^MAGIC), block_size) ^ MAGIC;
        sf_footer* footer = ((void *) ftr_blk) + GET_BLOCK_SIZE((ftr_blk->header^MAGIC));
        *footer = hdr_blk->header;
        return hdr_blk;
    }
    //Return curr if no coalescing was able to be performed
    return curr;
}


/**
 * @brief grow_heap is used to grow the heap, update the epilogue, and 
 * coalesce the new page of memory in search for a block to satisfy a 
 * sf_malloc request 
 * 
 * @return void* 
 */
void* grow_heap(int block_size, int size){
    debug("Reached grow heap\n");
    sf_block* old_epilogue = sf_mem_end() - 16;
    while (GET_BLOCK_SIZE((old_epilogue->header^MAGIC)) < block_size){
        //If the heap cannot be grown anymore, add this block to a free list and return NULL
        //sf_malloc then has to set error using errno 
        if (!sf_mem_grow()){ 
            debug("Request could not be satisfied.\n");
            sf_header header= old_epilogue->header^MAGIC;
            freeListAdder(old_epilogue, GET_BLOCK_SIZE(header), GET_PREV_BIT(header), true);
            return NULL;
        }
        old_epilogue = sf_mem_end() - (PAGE_SZ + 16);
        sf_show_heap();
        create_alloc_block(sf_mem_end() - 16, 0, 0, false, false); //Create a new epilogue
        sf_show_heap();
        //Use the old epilogue as the new header of this block, and create this block
        old_epilogue->header = SET_BLOCK_SIZE((old_epilogue->header ^ MAGIC), PAGE_SZ) ^ MAGIC;
        create_free_block(old_epilogue, PAGE_SZ, 0, false, false, GET_PREV_BIT((old_epilogue->header^MAGIC)));
        old_epilogue = coalesce(old_epilogue); //Coalesce
        sf_show_heap();
    }
    //Try to split the block first if possible: first half for allocating, second block for free block
    split_block(old_epilogue, block_size, true);
    old_epilogue = create_alloc_block(old_epilogue, block_size, size, false, GET_PREV_BIT((old_epilogue->header ^ MAGIC)));
    return old_epilogue->body.payload;
}

void sf_free(void *pp) {
    //First check that the passed in pointer is valid
    sf_block* blk = validate_pointer(pp);
    if (!blk) abort();

    //If pointer is validated, proceed with coalescing and putting the block in the appropriate list 
    sf_header hdr = blk->header ^ MAGIC;
    int block_size = GET_BLOCK_SIZE(hdr);

    //First, try to add this block to the front of a quick list
    //if it can go in a quick list, DON'T COALESCE- this is so that deferred coalescing
    //can be satisfied for blocks in quick lists
    int qk_index = GET_QK_INDEX(block_size);
    if (qk_index < NUM_QUICK_LISTS) quickListAdder(blk, block_size, GET_PREV_BIT(hdr));
    else freeListAdder(blk, block_size, GET_PREV_BIT(hdr), false);
}

sf_block* validate_pointer(void *pp){
    if (!pp) return NULL;

    sf_block* blk = (sf_block*)(pp-16);

    if ((void*)&blk->header < sf_mem_start() + 40) return NULL; //If header is before the first block in the heap, abort
    sf_header hdr = blk->header ^ MAGIC;
    int block_size = (int)GET_BLOCK_SIZE(hdr);
    if (block_size < 32 || block_size % 16 != 0 || !GET_ALLOC_BIT(hdr)) return NULL;
    
    void* ftr_ptr = (void*)blk + block_size;
    if (ftr_ptr >= sf_mem_end() - 8) return NULL; //If footer is past the last block in the heap, abort

    sf_footer prev_blk_ftr = blk->prev_footer ^ MAGIC;
    if (!GET_PREV_BIT(hdr) && GET_ALLOC_BIT(prev_blk_ftr)) return NULL; //If prev alloc is 0  but alloc of header is 1, abort
    return blk;
}

void quickListAdder(void* block_address, int block_size, bool prevalloc){
    int qk_index = GET_QK_INDEX(block_size);
    //Make sure both the qk_list and alloc bit are set 
    sf_block* new_block = create_free_block(block_address, block_size, 0, true, true, prevalloc);
    //Since it's in a quick list, set the prevalloc bit of the next block in the heap to 1
    sf_block* next = (void*)new_block + block_size;
    next->header = ((next->header ^ MAGIC) | PREV_BLOCK_ALLOCATED)^MAGIC;
    sf_block* block = sf_quick_lists[qk_index].first;
    //Flush the quick list if it has reached capacity first, and reset its length
    //Note that blocks in a quick list are only coalesced when they are flushed: this is deferred coalescing
    if (sf_quick_lists[qk_index].length == QUICK_LIST_MAX){
        int i; 
        for (i = 0; i<QUICK_LIST_MAX; i++){
            sf_block* tmp = block->body.links.next;
            sf_quick_lists[qk_index].first = tmp;
            freeListAdder(block, GET_BLOCK_SIZE((block->header^MAGIC)), GET_PREV_BIT((block->header^MAGIC)), false);
            block = tmp;
        }
        sf_quick_lists[qk_index].length = 1; //Reset the length after flushing
    }
    else sf_quick_lists[qk_index].length++;

    sf_quick_lists[qk_index].first = new_block;
    if (block) new_block->body.links.next = block;
}

/**
 * @brief freeListAdder is used to coalesce and add a block to a free list
 * 
 * @param block_address starting address of the block
 * @param block_size size of the free block
 * @param prevalloc prevalloc bit of the block
 */
void freeListAdder(void* block_address, int block_size, bool prevalloc, bool coalesced){
    sf_block* new_block;

    //First, attempt to coalesce if not already done
    //NOTE: coalescing is only performed before adding to a FREE LIST
    if (!coalesced){
        new_block = coalesce(block_address);
        block_size = GET_BLOCK_SIZE((new_block->header^MAGIC));
    }

    //Then, create a new free block that satisfies requirements of the main free list
    new_block = create_free_block(block_address, block_size, 0, false, false, prevalloc);
    debug("Block size: %ld\n", GET_BLOCK_SIZE((new_block->header ^ MAGIC)));
    //Finally, add this block to the front of the appropriate free list
    int index = determine_freelist_index((double)GET_BLOCK_SIZE((new_block->header ^ MAGIC))/32);
    if (index > NUM_FREE_LISTS-1) index = NUM_FREE_LISTS-1; //If its a request of blocks of size >=256M, then it goes in the last free list
    sf_block* sentinel = &sf_free_list_heads[index];
    sf_block* old_next = sentinel->body.links.next;
    sentinel->body.links.next = new_block;
    old_next->body.links.prev = new_block;
    new_block->body.links.prev = sentinel;
    new_block->body.links.next = old_next;
}


void *sf_realloc(void *pp, sf_size_t rsize) {
    //First, validate the pointer using the same rules (and in this case, helper function) as sf_free()
    sf_block* blk = validate_pointer(pp);
    //If invalid, set errno to EINVAL and return NULL
    if (!blk){errno = EINVAL; return NULL;}
    //If pointer is valid, but size is 0, free the block and return NULL
    if (rsize == 0){
        sf_free(pp);
        return NULL;
    }
    int payload_size = (blk->header ^ MAGIC) >> 32;
    /* If allocating to larger size: follow these three steps:
            1. Call malloc to get a block for this payload
            2. Call memcpy to copy over the payload from the old block to new one
            3. Free the old block
     */
    if (rsize > payload_size){
        void* new_blk = sf_malloc(rsize);
        //If there isn't enough memory, then return NULL (sf_malloc will have set errno to ENOMEM)
        if (!new_blk) return NULL;
        memcpy(new_blk, pp, payload_size);
        sf_show_heap();
        sf_free(pp);
        return new_blk;
    }
    
    //If allocating to a smaller size, an attempt should be made to split
    //the block. If it can't be split, readjust the payload size and return
    else if (rsize < payload_size){
        //First, determine the block size by adding up the rsize and header 
        //and rounding to nearest multiple of 16
        int block_size = HEADER_SIZE + rsize;
        if (block_size % 16 != 0) block_size += GET_ROUND_NUM(block_size);
        if (block_size == 16) block_size*=2;
        //If splitting would create a splinter, readjust the payload size and return
        if (GET_BLOCK_SIZE((blk->header ^ MAGIC))-block_size < 32){
            adjust_header(blk, GET_BLOCK_SIZE((blk->header ^ MAGIC)), rsize, true, false, GET_PREV_BIT((blk->header ^ MAGIC)));
            return pp;
        }
        else{
            split_block(blk, block_size, false);
            blk = create_alloc_block(blk, block_size, rsize, false, GET_PREV_BIT((blk->header ^ MAGIC)));
            return (void*)blk + 16;
        }
    }
    get_aggregate_payload();
    return pp;
}

double sf_internal_fragmentation() {; 
    double payload = 0.0; double block_size = payload;

    sf_block* blk = sf_mem_start() + 32; //Get to the first actual block
    if ((void*)blk >= sf_mem_end()) return 0.0;
    //Loop through until the epilogue is reached
    while((void *) blk < sf_mem_end() - 16){
        sf_header header = blk->header ^ MAGIC;
        //Make sure allocated blocks are only considered:
        //Allocated blocks have alloc bit as 1 and qk bit as 0
        if (GET_ALLOC_BIT(header) && !GET_QK_BIT(header)){
            block_size += GET_BLOCK_SIZE(header);
            payload += header >> 32;
        }
        blk = (void *)blk + GET_BLOCK_SIZE(header);
    }
    return payload/block_size;
}

void get_aggregate_payload(){
    double payload = 0.0; 
    sf_block* blk = sf_mem_start() + 32; //Get to the first actual block
    if ((void*)blk >= sf_mem_end()) return;
    //Loop through until the epilogue is reached
    while((void *) blk < sf_mem_end() - 16){
        sf_header header = blk->header ^ MAGIC;
        //Make sure allocated blocks are only considered:
        //Allocated blocks have alloc bit as 1 and qk bit as 0
        if (GET_ALLOC_BIT(header) && !GET_QK_BIT(header)) payload += header >> 32;
        blk = (void *)blk + GET_BLOCK_SIZE(header);
    } 
    //Only change the maximum aggregate payload if the aggregate payload
    //of the current heap is greater 
    max_aggregate = (payload > max_aggregate ? payload : max_aggregate);
}

double sf_peak_utilization() {
    get_aggregate_payload();
    double heap_size = sf_mem_end()-sf_mem_start();
    return max_aggregate/heap_size;
}