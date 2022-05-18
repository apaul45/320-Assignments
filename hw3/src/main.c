#include <stdio.h>
#include "sfmm.h"
#include "basic.h"
#include <debug.h>
int main(int argc, char const *argv[]) {
    int i;
    //Make sure the next and prev pointers of each
    //dummy node in free list heads point back to them
    for (i=0; i<NUM_FREE_LISTS; i++){
        sf_free_list_heads[i].body.links.next = sf_free_list_heads[i].body.links.prev = 
        &sf_free_list_heads[i];
    }
    size_t size_a = 20; 
	void* blks[10];
	//Malloc 10 blocks, so that a quick list can properly be filled up
	for (i=0; i<10; i++){
		blks[i] = sf_malloc(size_a);
	}
	//Free blocks at even indices, so that they all can go into the same
	//quick list without coalescing
	for (i=0; i<10; i+=2){
		sf_free(blks[i]);
	}
    sf_malloc(size_a);
    sf_show_heap();
    return EXIT_SUCCESS;
}
