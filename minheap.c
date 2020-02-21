#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "minheap.h"

static int get_left_child_index(int i)
{
    return 2 * i + 1;
}

static int get_rght_child_index(int i)
{
    return 2 * i + 2;
}

static void node_swap(struct minheap_node* x, struct minheap_node* y)
{
    assert(x);
    assert(y);

    struct minheap_node temp = *x;
    *x = *y;
    *y = temp;
}

static void heapify(struct minheap* heap, int i)
{
    assert(heap);

    int left = get_left_child_index(i);
    int rght = get_rght_child_index(i);
    int smallest = i;
    if (left < heap->size && heap->nodes[left].element.id < heap->nodes[i].element.id)
        smallest = left;
    if (rght < heap->size && heap->nodes[rght].element.id < heap->nodes[smallest].element.id)
        smallest = rght;
    if (smallest != i)
    {
        node_swap(&heap->nodes[i], &heap->nodes[smallest]);
        heapify(heap, smallest);
    }
}

/* Public */

void minheap_ctor(struct minheap* heap, int size, uint64_t part_size, int file)
{
    heap->size  = size;
    heap->nodes = (struct minheap_node*)malloc(size * sizeof(struct minheap_node));

    /* Read initial elements from file */
    ssize_t r;
    for (int i = 0; i < heap->size; i++) {
        r = pread(file, &heap->nodes[i].element, sizeof(heap->nodes[i].element),
                  i * part_size);
        assert(r == sizeof(heap->nodes[i].element));

        heap->nodes[i].i = i;
    }

    for (int i = (size - 1) / 2; i >= 0; i--)
        heapify(heap, i);
}

void minheap_dtor(struct minheap* heap)
{
    heap->size = 0;
    free(heap->nodes); heap->nodes = NULL;
}

void minheap_get_min(const struct minheap* heap, struct minheap_node* node)
{
    assert(heap);
    assert(node);

    /* Min value of heap is the root(first value in node array) */
    *node = heap->nodes[0];
}

void minheap_replace_min(struct minheap* heap, const struct minheap_node* x)
{
    assert(heap);

    heap->nodes[0] = *x;
    heapify(heap, 0);
}
