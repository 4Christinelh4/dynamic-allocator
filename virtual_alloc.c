#include "virtual_alloc.h"
#include "virtual_sbrk.h"
#include "node.h"
#include "node.c"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define NODE 1 // node is 8 bits = 1 byte
#define TOTAL_SIZE_POSITION 0
#define MIN_ALLOCATION_POSITION 1
#define NUMBER_NODE 2
#define ALL_OFFSET 12 // 12 bytes for all offsets at the very beginning of the heap
#define START 0

void init_allocator(void *heapstart, uint8_t initial_size, uint8_t min_size){

    if (NULL == heapstart){
        fprintf(stderr, "no heap\n");
        exit(1);
    }

    if (min_size > initial_size){
        fprintf(stderr, "min size %u is larger than initial size %u\n", min_size, initial_size);
        exit(1);
    }

    uint64_t offset = virtual_sbrk(0)-heapstart;

    if (offset != 0){
        virtual_sbrk(-offset);
    }

    uint64_t total_initial_size = pow(2, initial_size) + ALL_OFFSET + NODE + NODE;

    heapstart = virtual_sbrk(total_initial_size);

    if (heapstart == (void *) -1){
        fprintf(stderr, "fail to initialise...\n");
        exit(1);
    }

    uint32_t *tmp_cast = heapstart;

    *(tmp_cast + TOTAL_SIZE_POSITION) = initial_size;
    *(tmp_cast + MIN_ALLOCATION_POSITION) = min_size;
    *(tmp_cast + NUMBER_NODE) = 1; // one node initially

    void *data_start = heapstart + ALL_OFFSET + (uint64_t)pow(2, initial_size);

    uint8_t *cast_to_byte = data_start;

    uint8_t first_node = create_node(initial_size, EMPTY);
    *(cast_to_byte + 0) = first_node;

    return;
}

/*
 * return the offset of the required block
 * return 0 if not found a proper block
 */
int search_most_close_size(void *heapstart, uint64_t request_size){
    uint32_t *heap = heapstart;
    uint32_t initial_size = heap[0];

    void *data_start = heapstart + ALL_OFFSET + (uint64_t) pow(2, initial_size);

    uint8_t *structure = data_start;

    uint32_t start = START;
    uint8_t found_first=0;
    uint8_t best=0;

    int res=-1;

    while (start < heap[NUMBER_NODE]) {

        uint8_t this_node = structure[start];
        uint64_t size = pow(2, get_node_size(this_node));

//        uint64_t size = pow(2, structure[start]);

//        printf("size = %llu\n", size);

        if (size < request_size){
            // skip
            start += NODE;
            continue;
        }

        if (get_node_full_or_empty(this_node) == FULL){
            // skip
            start += NODE;
            continue;
        }

        if (found_first == 0){
            best = *(structure + start);
            res = start;
            found_first = 1;
        } else{

            if (*(structure + start) < best){
                best = *(structure + start);
                res = start;
            }
        }

        start += NODE;
    }

    // res is index. it must be 3, 6, 9...... where the size (exponent) is stored
//    printf("\nbest fit idx: %d\n", res);

    return res;
}

void *virtual_malloc(void *heapstart, uint32_t size){

    if (0 == size){
        fprintf(stderr, "incorrect: malloc 0\n");
        return NULL;
    }

    uint32_t *heap = heapstart;

    uint8_t min_alloc = heap[MIN_ALLOCATION_POSITION];

    uint64_t request_size = pow(2, min_alloc);

    while (request_size<size){
        request_size*=2;
    }

    int position = search_most_close_size(heapstart, request_size);

    if (-1 == position){
        fprintf(stderr, "cant allocate %u\n", size);
        return NULL;
    }

    return iter_allocation(heapstart, request_size, (uint32_t )position);
}

void *iter_allocation(void *heapstart, uint64_t request_size, uint32_t position) {

    uint32_t *heap = heapstart;

    uint8_t initial_size = heap[0];

    void *data_start = heapstart + ALL_OFFSET + (uint64_t) pow(2, initial_size);

    uint8_t *structure = data_start;

    uint32_t node_num = heap[NUMBER_NODE];

    request_size = (uint8_t) log2(request_size); // like 3 for 8, 4 for 16

    uint32_t ending;
    uint32_t cursor;

    while (get_node_size(structure[position]) > request_size
           && get_node_full_or_empty(structure[position]) == EMPTY) {
        ending = position;

        while (ending < node_num) {
            ending++;
        }

        // now move ending to the last one

        if (ending - position == NODE) {
            // divide the last block

            virtual_sbrk(NODE);

            uint8_t current_node = *(structure + position);

            uint8_t new_buddy = get_node_size(current_node) - 1;

            *(structure + position) = create_node(new_buddy, EMPTY);
            *(structure + position + NODE) = create_node(new_buddy, EMPTY);

            heap[NUMBER_NODE] += NODE;
            node_num = heap[NUMBER_NODE];
            continue;
        }

        virtual_sbrk(NODE);

        cursor = ending;

        while (cursor >= position + 2) {

            *(structure + cursor) = *(structure + cursor - 1);
            cursor--;
        }

//        printf("cursor = %u, position = %u\n", cursor, position);

        uint8_t this_node = *(structure + position);
        uint8_t new_buddy = get_node_size(this_node) - 1;

        *(structure + position) = create_node(new_buddy, EMPTY);
        *(structure + position + 1) = create_node(new_buddy, EMPTY);

        heap[NUMBER_NODE] += NODE;

        node_num = heap[NUMBER_NODE];
    }

    uint8_t allocate_this_node = *(structure + position);
    uint8_t allocated_node = create_node(get_node_size(allocate_this_node), FULL);
    *(structure + position) = allocated_node;

    uint32_t starting = START;
    uint64_t total_offset = 0;

    while (starting < position) {

        uint8_t current_node = structure[starting];
        uint8_t size = get_node_size(current_node);
        total_offset += (uint64_t) pow(2, size);
        starting += NODE;
    }

    void *allocated_addr = heapstart + ALL_OFFSET + total_offset;

    return allocated_addr;
}

int virtual_free(void *heapstart, void *ptr){

    if (NULL == ptr){
//        fprintf(stderr, "cant free a null pointer\n");
        return -1;
    }

    if (-1 == get_node_offset(heapstart, ptr)){
//        fprintf(stderr, "this node does not exist\n");
        return -1;
    }

    uint32_t offset = (uint32_t) get_node_offset(heapstart, ptr);

    uint32_t *heap = heapstart;

    void *data = heapstart + ALL_OFFSET + (uint64_t )pow(2, heap[0]);

    uint8_t *structure = data;
    uint8_t free_this_node = structure[offset];

    if (get_node_full_or_empty(free_this_node) == EMPTY){
//        fprintf(stderr, "cant free a node that has not been allocated\n");
        return -1;
    }

    uint8_t free_size = get_node_size(free_this_node);
    structure[offset] = create_node(free_size, EMPTY);

    if (NODE == heap[NUMBER_NODE]){
        // only 1 node
        // no need to merge
        return 0;
    }


    dfs_merge(heapstart, offset);
    ptr = NULL;
    return 0;
}

/*
 * helper for virtual_free, get the info from data structure through the given ptr
 */
int get_node_offset(void *heapstart, void *ptr){

    void *real_start = heapstart + ALL_OFFSET;
    uint64_t distance = ptr-real_start;

    void *data_start = heapstart + ALL_OFFSET + (uint64_t )pow(2, ((uint8_t *)heapstart)[0]);
    uint8_t *data_structure = data_start;

    uint32_t i=START;
    uint64_t total_sum=0;

    uint32_t data_len = *((uint32_t *)heapstart + NUMBER_NODE);

    while (i<data_len){

        if (total_sum == distance){
            return i; // i = 0, 2, 4, 6, 8.....
        }

        uint8_t current_node = data_structure[i];
        uint64_t size = pow(2, get_node_size(current_node));

        total_sum += size;
        i+=NODE;
    }

    return -1; // this ptr is invalid, it doesnt points to any start
}

/*
 * if it doesnt have a buddy return -1 (buddy: same size)
 * else return the buddy's index
 * whether the buddy is free, is checked in dfs_merge
 */
int get_buddy_index(void *heapstart, uint32_t target_offset, uint32_t start, uint32_t end, uint32_t total){

    uint32_t *heap = heapstart;

    void *data = heapstart + ALL_OFFSET + (uint64_t )pow(2, heap[0]);

    uint8_t *structure = data;

    if (start == end){
        // start == end ==  target_offset, so it is only one
        return -1;
    }

    if (start == target_offset){

        uint8_t node_1 = structure[target_offset + NODE];
        uint8_t node_2 = structure[target_offset];

        if (get_node_size(node_1) != get_node_size(node_2)){
            return -1;
        } else{
//            printf("case 1: buddy: %u target: %u", target_offset+1, target_offset);
            return target_offset + NODE;
        }
    }

    if (end == target_offset){
        uint8_t node_1 = structure[target_offset - NODE];
        uint8_t node_2 = structure[target_offset];

        if (get_node_size(node_1) != get_node_size(node_2)){
            return -1;
        } else{
//            printf("case 2: buddy: %u target: %u", target_offset-1, target_offset);
            return target_offset - NODE;
        }
    }

    uint32_t cursor = start;
    uint64_t half_sum = pow(2, get_node_size(structure[start]));

    while (half_sum < total/2){
        cursor += NODE;
        uint8_t cursored_node = structure[cursor];
        half_sum += pow(2, get_node_size(cursored_node));
    }

    if (cursor >= target_offset){
        // search within left half
        return get_buddy_index(heapstart, target_offset, start, cursor, total/2);
    } else {
        return get_buddy_index(heapstart, target_offset, cursor+NODE, end, total/2);
    }
}


void dfs_merge(void *heapstart, uint32_t target_offset){

    uint32_t *heap = heapstart;

    void *data = heapstart + ALL_OFFSET + (uint64_t )pow(2, heap[0]);

    uint8_t *structure = data;

    uint32_t end = heap[NUMBER_NODE] - NODE; // index and position..

    uint64_t total = pow(2, heap[TOTAL_SIZE_POSITION]);

    int buddy_index = get_buddy_index(heapstart, target_offset, START, end, total);

    if (buddy_index == -1){
        // no buddy cant merge
        return;
    }

    // check if empty, if not, cant merge
    uint8_t the_buddy = structure[buddy_index];

    if (get_node_full_or_empty(the_buddy) == FULL){
        return;
    }

//    printf("buddy index = %u, target offset = %u\n", buddy_index, target_offset);
    uint32_t merge_start=0; // be the left one. when merge, the right side will disappear

    if (buddy_index == target_offset - NODE){
        // buddy is left
        merge_start = buddy_index;
    } else if (buddy_index == target_offset + NODE){
        merge_start = target_offset;
    }

//    uint8_t current_size = *(structure + merge_start);
    uint8_t current_merge_start = *(structure + merge_start);

    uint8_t merged_size = 1 + get_node_size(current_merge_start);

    *(structure + merge_start) = create_node(merged_size, EMPTY); // ACTUAL MERGE HERE

//    *(structure + merge_start) = current_size + 1;

    uint32_t i = NODE + merge_start;

    while (i < end){

        *(structure + i) = *(structure + i + NODE);
//        *(structure + i + 1) = *(structure + i + 2 + 1);

        i += NODE;
    }

    heap[NUMBER_NODE] -= NODE;
    virtual_sbrk(-NODE);

    dfs_merge(heapstart, merge_start);
}

/*
 * num_blocks: the number of 'combined' blocks. should be 1 initially. regard all the blocks as one single
 * this_offset: the index of the first block among the blocks to be freed
 * regard the group of blocks as one single block and look for its buddy
 * start, end: first and last index of the current area I am looking for
 */
int get_block_neighbor(void *heapstart, uint32_t this_offset, uint32_t num_blocks
        , uint32_t start, uint32_t end, uint64_t total){

    uint32_t *heap = heapstart;

    void *data = heapstart + ALL_OFFSET + (uint64_t )pow(2, heap[0]);

    uint8_t *structure = data;

    if (start+(num_blocks-1)*NODE == end){
        // only 1 large single block
        return -1;
    }

    uint32_t current_sum = 0;
    // sum up the total size in this 'node group'
    uint32_t counter = 0;

    while (counter < num_blocks){
        uint8_t current_node = structure[this_offset + counter];
        current_sum += pow(2, get_node_size(current_node));
        counter++;
    }

    if (start == this_offset){
        uint8_t neighbor = structure[this_offset + num_blocks];

        if (current_sum != pow(2, get_node_size(neighbor))){
//            printf("case 1 fail, num_blocks = %d\n", num_blocks);
            return -1;
        } else{
            return this_offset + num_blocks;
        }
    }

    // this one single big node is the last one
    if (end == this_offset + (num_blocks-1)*NODE){
        uint8_t neighbor = this_offset - NODE;

        if (current_sum != pow(2, get_node_size(neighbor))){
//            printf("case 2 fail\n");
            return -1;
        } else{
            return this_offset - NODE;
        }
    }

    uint32_t cursor = start;
    uint8_t this_node = structure[cursor];
    uint64_t half_sum = pow(2, get_node_size(this_node));

    while (half_sum < total/2){
        cursor += NODE;

        this_node = structure[cursor];

        half_sum += pow(2, get_node_size(this_node));
    }

    if (cursor >= this_offset){
        return get_block_neighbor(heapstart, this_offset, num_blocks, start, cursor, total/2);
    } else{
        return get_block_neighbor(heapstart, this_offset, num_blocks, cursor + NODE, end, total/2);
    }

}

/*
 * helper for reallocation. It is similar to dfs_merge, but
 * this function doesnt really free anything
 */
uint8_t get_max_block_after_free(void *heapstart, uint8_t required_size, uint8_t offset){

    uint32_t *heap = heapstart;
    void *data = heapstart + ALL_OFFSET + (uint64_t )pow(2, heap[0]);
    uint8_t *structure = data;

    uint8_t res = get_node_size(structure[offset]);

    uint32_t num_blocks = 1; // initially it is only one block

    uint32_t start = START;
    uint32_t end = heap[NUMBER_NODE] - NODE; // the last block's index
    uint32_t total_size = pow(2, heap[TOTAL_SIZE_POSITION]);

    // request size and res are both exponent
    while (res < required_size){

        int buddy = get_block_neighbor(heapstart, offset, num_blocks, start, end, total_size);

//        printf("buddy = %d\n", buddy);

        if (buddy == -1){
            break;
        }

        // stop allocate
        if (get_node_full_or_empty(structure[(uint32_t)buddy]) == FULL){
            break;
        }

        if (buddy == offset-NODE){
            offset = buddy;
        }

        res++; // act like merged
        num_blocks++;
    }

    return res;
}

void *virtual_realloc(void *heapstart, void * ptr, uint32_t new_size){
    // if new size (2^m) <= old size: first free, then malloc
    uint8_t *heap = heapstart;

    void *data = heapstart + ALL_OFFSET + (uint64_t )pow(2, heap[0]);

    uint8_t *structure = data;

    int offset = get_node_offset(heapstart, ptr);

    if (new_size == 0){
        int ret = virtual_free(heapstart, ptr);
        return (void *) ((long)ret);
    }

    if (ptr == NULL){
        // new size != 0 but ptr = null: virtual_malloc()
        void *res = virtual_malloc(heapstart, new_size);
        return res;
    }

    if (offset == -1){
        return NULL;
    }

    offset = (uint32_t )offset;

    // check if this is node is allocated!!!!!
    if (get_node_full_or_empty(structure[offset])== EMPTY){
        return NULL;
    }

    // get the request size, compare with the original size
    uint64_t new_allocation = pow(2, heap[MIN_ALLOCATION_POSITION]);

    while (new_allocation < new_size){
        new_allocation *= 2;
    }

    uint64_t prev_allocation = pow(2, get_node_size(structure[offset]));

    if (prev_allocation >= new_allocation){
        // reduce the data size... this allocation will surely successful
        void *save_ptr = ptr;
        virtual_free(heapstart, ptr);

        void *res = virtual_malloc(heapstart, new_size);
        // move part of the old values to the new position
        for(uint64_t i=0; i<new_size; i++){
            *((uint8_t *)res + i) = *((uint8_t *)save_ptr + i);
        }

        return res;
    }

    // new_allocation > prev_allocation
    // get max block size after free....
    uint8_t result_after_free = get_max_block_after_free(heapstart, (uint8_t )log2(new_allocation), offset);

//    printf("result = %u\n", result);

    int max_before_free = search_most_close_size(heapstart, new_allocation);

    if (result_after_free >= (uint8_t )log2(new_allocation) || max_before_free != -1){
        // is able to allocate
        void *save_ptr = ptr;

        virtual_free(heapstart, ptr);

        void *addr = virtual_malloc(heapstart, new_size);

        for(uint64_t i=0; i<prev_allocation; i++){
            *((uint8_t *)addr + i) = *((uint8_t *)save_ptr + i);
        }

//        printf("allocated: %p\n", addr);
        return addr;
    }
//    printf("cant allocate\n");
    return NULL;
}

void virtual_info(void *heapstart){

    uint32_t *heap = heapstart;
    uint8_t initial_size = heap[0];
    void *data_start = heapstart + ALL_OFFSET + (uint64_t) pow(2, initial_size);

    uint8_t *structure = data_start;
    uint64_t start = START;

//    printf("\n====memory====\n");

    while (start < heap[NUMBER_NODE]){

        uint8_t current_node = structure[start];

        uint32_t size = pow(2, get_node_size(current_node));

        if (get_node_full_or_empty(current_node) == EMPTY){
            printf("free %u\n", size);
        } else{
            printf("allocated %u\n", size);
        }

        start += NODE;
    }

//    printf("====ending====\n\n");

    return;
}
