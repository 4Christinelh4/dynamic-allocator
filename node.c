#include "node.h"
#include <stdint.h>
#include <stdio.h>

/*
 * for each node: the first bit implies if its full (allocated or empty)
 * empty: 1 full: 0
 * to allocate it : & with 01111111 = 127 (full_mask)
 * to empty it : | with 10000000 = 128 (empty_mask)
 * according to ed: size is less than 64
 */
uint8_t create_node(uint8_t size, uint8_t full_or_empty){

    uint8_t node = size;

    if (full_or_empty == FULL){
        node = node & FULL_MASK;
    } else{
        // full_or_empty = empty
        node = node | EMPTY_MASK;
    }

    return node;
}

uint8_t get_node_size(uint8_t node){
    uint8_t res = node & ADJUST;
    return res;
}

uint8_t get_node_full_or_empty(uint8_t node){
    uint8_t tmp = node>>7;

    if (tmp == 1){
        return EMPTY;
    } else{
        return FULL;
    }
}