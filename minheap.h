#ifndef MINHEAP_H
#define MINHEAP_H

#include <stdint.h>
#include "util.h"

struct minheap_node {
    struct input_elem element;
    int i;
};

struct minheap {
    struct minheap_node* nodes;
    int size;
};

void minheap_get_min(const struct minheap* heap, struct minheap_node* node);

void minheap_replace_min(struct minheap* heap, const struct minheap_node* x);

void minheap_ctor(struct minheap* heap, int size, uint64_t part_size, int file);
void minheap_dtor(struct minheap* heap);

#endif
