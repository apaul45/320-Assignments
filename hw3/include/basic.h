#include "sfmm.h"
#include <stdbool.h>
#ifndef BASIC_H
#define BASIC_H
/**
 * This header file will be used to define macros to help carry out 
 * common processes (ie, getting the alloc, inqcklist, etc bits, 
 * getting the next block, etc)
 * 
 * It will also contain function prototypes of all the helper functions
 * made throughout the creation of the program
 */

#define HEADER_SIZE 8 //Header size in bytes--header - 64 bits
#define WORD_SIZE   2  //Word size in bytes (word in textbook refers to double word)
#define MIN_BLOCK_SIZE 32 //A block must be min 32 bytes, so that there is room for header, prev&next ptrs, and footer in a free block
#define ALIGN_SIZE 16 //Alignment must be 16 bytes because a payload starts after the prev footer and header 
#define GET_BLOCK_SIZE(h) (h & 0xFFFFFFF0) //Gets the value of the 4 ls-bytes in the header (minus 3 of the lsbs)
#define SET_BLOCK_SIZE(h, s) ((h & 0xFFFFFFFF0000000F) | s)
#define GET_QK_BIT(h) (h & IN_QUICK_LIST)
#define GET_PREV_BIT(h) (h & PREV_BLOCK_ALLOCATED)
#define GET_ALLOC_BIT(h) (h & THIS_BLOCK_ALLOCATED)
#define GET_QK_INDEX(size) (size-MIN_BLOCK_SIZE)/ALIGN_SIZE 
#define GET_ROUND_NUM(size) (16 - (size % 16)) //Gets the amount to add to round up to multiple of 16

int determine_freelist_index(double size);
sf_block* create_alloc_block(void* start_address, int block_size, int payload_size, bool qk, bool prevalloc);
sf_block* create_free_block(void* start_address, int block_size, int payload_size, bool alloc, bool qk, bool prevalloc);
void adjust_header(sf_block* new_block, int block_size, int payload_size, bool alloc, bool qk, bool prevalloc);
void add_free_block(void* block_address, int block_size, bool prevalloc, bool split);
void freeListAdder(void* block_address, int block_size, bool prevalloc, bool coalesced);
void quickListAdder(void* block_address, int block_size, bool prevalloc);
sf_block* coalesce(sf_block* curr);
void remove_from_list(sf_block* block);
void* grow_heap(int block_size, int size);
void split_block(sf_block* block, int block_size, bool no_coalesce);
sf_block* validate_pointer(void* pp);
void get_aggregate_payload();
#endif