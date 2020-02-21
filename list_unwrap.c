#include "list_unwrap.h"
#include "io.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define PRINT_PROGRESS

static void read_list_elem(int file, uint64_t num, struct input_elem* res)
{
    ssize_t r;
    r = pread(file, res, sizeof(*res),
              num * sizeof(struct input_elem));
    assert(r == sizeof(*res));
}

static uint64_t lookup_id(int             file,
                          struct cache_s* cache,
                          uint64_t        key,
                          uint64_t        left,
                          uint64_t        rght)
{
#ifdef PRINT_PROGRESS
    static uint64_t hits = 0, misses = 0, call_num = 0;
    if (call_num % 1000 == 0)
        printf("\r\t\t\t\t\t\t\tHits %llu/%llu(%5lg%%)    ",
                hits, misses + hits, (double)hits * 100 / (hits + misses));
    call_num++;
#endif

    struct input_elem mid_elem;
    uint64_t mid;
    int in_cache;
    uint32_t depth = 0;
#ifdef VERBOSE
    printf("\nLooking for key = %llu\n", key);
#endif

    while (rght >= left) {
        mid = (left + rght) / 2;

        in_cache = lookup_id_in_cache(cache, mid, depth, &mid_elem.id);
#ifdef VERBOSE
        printf("\tmid = %llu, %sin cache\n", mid, in_cache ? "" :"not ");
#endif
        if (!in_cache)
            read_list_elem(file, mid, &mid_elem);

#ifdef PRINT_PROGRESS
        if (!in_cache)
            hits++;
        else
            misses++;
#endif

        if (mid_elem.id == key) {
            if (in_cache)
                read_list_elem(file, mid, &mid_elem);
            return mid_elem.next_id;
        }

        if (mid_elem.id > key)
            rght = mid - 1;
        else
            left = mid + 1;

        depth++;
    }

    printf("\nKey %llu not found, aborting\n", key);
    exit(1);
    return 0;
}

/* Returns value that corresponds to 'key' in the 'file' */
void unwrap_list(struct cache_s*     cache,
                 int                 input_file,
                 int                 output_file,
                 time_t              cur_time)
{
    uint64_t file_size = get_file_size(input_file);

    struct buf_io* s = (struct buf_io*)calloc(1, sizeof(*s));
    s->file = output_file;

    struct output_elem temp = {};
    uint64_t prev_id = 1, next_id; /* First node number is 1 */

    const uint64_t list_size = file_size / sizeof(struct input_elem);
    for (uint64_t i = 0; i < list_size; i++) {
        /* Lookup the node */
        next_id = lookup_id(input_file, cache, prev_id, 0, list_size - 1);
        /* Write element id and rank to the output file */
        temp.id = prev_id; temp.rank = i;
        buffered_write(s, &temp, sizeof(temp), NOFLUSH);

        prev_id = next_id;

#ifdef PRINT_PROGRESS
        if (i % 1000 == 0)
            printf("\rUnwrapping progress = %4lg%%, %lu sec   ",
                    (double)i * 100 / list_size, time(NULL) - cur_time);
#endif
    }
    buffered_write(s, NULL, 0, FLUSH);
#ifdef PRINT_PROGRESS
    printf("\rUnwrapping progress = 100%%, %lu sec   ",
            time(NULL) - cur_time);
#endif

    /* Check that last element has next_id == 0 */
    assert(prev_id == 0);
}
