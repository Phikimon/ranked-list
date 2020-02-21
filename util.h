#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#define KB   ((uint64_t)1024)
#define MB   ((uint64_t)1024 * 1024)
#define GB   ((uint64_t)1024 * 1024 * 1024)

#define GB_2 (GB * 2)

#define PART_SIZE GB_2
#define ELEMS_IN_PART ((uint64_t)PART_SIZE / (2 * sizeof(uint64_t)))

struct input_elem {
    uint64_t id;
    uint64_t next_id;
};

struct output_elem {
    uint64_t id;
    uint64_t rank;
};

const char* size_all_units(uint64_t size);

#endif
