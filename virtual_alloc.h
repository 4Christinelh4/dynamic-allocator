#ifndef VIRTUAL_ALLOC_H
#define VIRTUAL_ALLOC_H

#include <stddef.h>
#include <stdint.h>

void init_allocator(void *heapstart, uint8_t initial_size, uint8_t min_size);
void *virtual_malloc(void *heapstart, uint32_t size);
int virtual_free(void *heapstart, void *ptr);
void *virtual_realloc(void *heapstart, void * ptr, uint32_t size);
void virtual_info(void *heapstart);

int search_most_close_size(void *heapstart, uint64_t request_size);
void *iter_allocation(void *heapstart, uint64_t request_size, uint32_t position);
int get_node_offset(void *heapstart, void *ptr);
void dfs_merge(void *heapstart, uint32_t target_offset);
int get_buddy_index(void *heapstart, uint32_t target_offset, uint32_t start, uint32_t end, uint32_t total);
uint8_t get_max_block_after_free(void *heapstart, uint8_t required_size, uint8_t offset);
int get_block_neighbor(void *heapstart, uint32_t this_offset, uint32_t num_blocks
        , uint32_t start, uint32_t end, uint64_t total);


#endif