#ifndef NEW_A3_NODE_H
#define NEW_A3_NODE_H

#include <stdint.h>

#define EMPTY 1
#define EMPTY_MASK 128
#define FULL 0
#define FULL_MASK 127
#define ADJUST 127

uint8_t create_node(uint8_t size, uint8_t full_or_empty);
uint8_t get_node_size(uint8_t node);
uint8_t get_node_full_or_empty(uint8_t node);

#endif
