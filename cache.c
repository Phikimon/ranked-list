#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "cache.h"
#include "util.h"
#include "io.h"

static int compare_elem_pos(const void* ptr_1, const void* ptr_2)
{
    const uint32_t val_1 = ((struct cache_entry*)ptr_1)->elem_pos;
    const uint32_t val_2 = ((struct cache_entry*)ptr_2)->elem_pos;

    if (__builtin_expect(val_1 == val_2, 0)) {
        return 0;
    } else
    if (val_1 > val_2) {
        return 1;
    } else
        return -1;
}

static int compare_depth(const void* ptr_1, const void* ptr_2)
{
    const uint32_t val_1 = ((struct cache_entry*)ptr_1)->depth;
    const uint32_t val_2 = ((struct cache_entry*)ptr_2)->depth;

    if (__builtin_expect(val_1 == val_2, 0)) {
        return 0;
    } else
    if (val_1 > val_2) {
        return 1;
    } else
        return -1;
}

int lookup_id_in_cache(struct cache_s* cache,
                       uint64_t        position,
                       uint32_t        depth,
                       uint64_t*       res)
{
    struct cache_entry key_entry = {.elem_pos = position};
    struct cache_entry* res_entry;
    uint32_t effective_depth = depth;
    if (depth > cache->max_depth) {
        if (cache->max_depth == DEPTH_NUM - 1) {
            effective_depth = cache->max_depth;
        } else
            return 0;
    }

    res_entry = bsearch(&key_entry, cache->depth_bufs[effective_depth],
                        cache->depth_buf_size[effective_depth],
                        sizeof(cache->buf[0]), compare_elem_pos);
    if (res_entry) {
        *res = res_entry->id;
        return 1;
    }
    return 0;
}

static uint32_t get_bin_tree_node(uint32_t left,
                                  uint32_t rght,
                                  uint64_t tree_depth,
                                  uint64_t where_to_go)
{
    uint32_t mid = (left + rght) / 2;

    for (int i = 0; i < tree_depth; i++) {
        if (where_to_go & (uint32_t)1)
            left = mid + 1;
        else
            rght = mid - 1;

        if (left > rght)
            return UINT32_MAX;
        mid = (left + rght) / 2;
        where_to_go >>= 1;
    }
    return mid;
}

static void fill_wanted_elements(struct cache_entry_meta* wanted, uint64_t file_size, uint64_t cache_cnt)
{
    const uint32_t list_size = file_size / sizeof(struct input_elem);
    uint64_t tree_depth = 1, nodes_on_depth = 2, nodes_left_on_depth = 2;

    for (uint64_t i = 0; i < cache_cnt - 1;) {
        nodes_left_on_depth--;
        wanted[i].elem_pos = get_bin_tree_node(0, list_size - 1,
                                               tree_depth, nodes_left_on_depth);
        wanted[i].depth = tree_depth;

        if (nodes_left_on_depth == 0) {
            tree_depth++;
            nodes_left_on_depth = nodes_on_depth *= 2;
        }

        if (wanted[i].elem_pos != UINT32_MAX)
            i++;
    }
    wanted[cache_cnt - 1].depth = 0;
    wanted[cache_cnt - 1].elem_pos = (list_size - 1) / 2;
}

static void fill_cache_file(int                      file,
                            int                      cache_file,
                            struct cache_entry_meta* wanted,
                            uint64_t                 cache_cnt)
{
    struct buf_io* s = calloc(1, sizeof(*s));
    s->file = cache_file;

    struct input_elem* buf = malloc(IO_BLOCK_SIZE);
    uint64_t j = 0;
    uint64_t list_offset = 0, list_size = 0;
    ssize_t r;

    for (uint64_t i = 0; i < cache_cnt; i++) {
        while (wanted[i].elem_pos >= list_offset + list_size) {
            r = pread(file, buf, IO_BLOCK_SIZE, j * IO_BLOCK_SIZE);
            assert(r > 0);
            j++;
            list_offset += list_size;
            list_size = r / sizeof(buf[0]);
        }

        /* Write cache entry to file */
        struct input_elem* input = &buf[wanted[i].elem_pos - list_offset];
        struct cache_entry cache_ent = {
            .depth = wanted[i].depth,
            .elem_pos = wanted[i].elem_pos,
            .id = input->id,
        };
        buffered_write(s, &cache_ent,
                       sizeof(cache_ent), NOFLUSH);
    }
    buffered_write(s, NULL, 0, FLUSH);

    free(buf); buf = NULL;
}

static void read_cache_file(struct cache_s* cache,
                     int cache_file,
                     time_t          cur_time)
{
    uint64_t current_depth = 0;
    uint64_t cache_size;

    printf("Reading to cache from cache file, %ld sec\n",
            time(NULL) - cur_time);

    pread_buffer(cache_file, cache->buf, &cache_size, 0);
    assert(cache_size == cache->size);

    printf("Cache read successfully. %llu entries. "
           "Now sorting cache by depth, %ld sec\n",
           cache->cnt, time(NULL) - cur_time);

    (void)qsort((void*)cache->buf, cache->cnt,
                sizeof(cache->buf[0]), compare_depth);

    printf("Cache sorted by depth. "
           "Now assigning pointers to subarrays, %ld sec\n",
           time(NULL) - cur_time);

    cache->depth_bufs[0] = &cache->buf[0];
    for (uint64_t i = 0; i < cache->size; i++) {
        if (cache->buf[i].depth == current_depth + 1) {
            if (current_depth == DEPTH_NUM - 1)
                break;
            current_depth++;
            cache->depth_bufs[current_depth] = &cache->buf[i];
        }
    }
    cache->max_depth = current_depth;
    cache->depth_bufs[cache->max_depth + 1] = &cache->buf[cache->size];

    printf("Pointers assigned. Max depth is %u. "
           "Now sorting each subarray by elem_pos, %ld sec\n",
           cache->max_depth, time(NULL) - cur_time);

    printf("Subarrays already sorted: \n");
    for (uint64_t i = 0; i < cache->max_depth; i++) {
        cache->depth_buf_size[i] = cache->depth_bufs[i + 1] - cache->depth_bufs[i];
        (void)qsort((void*)cache->depth_bufs[i], cache->depth_buf_size[i],
                    sizeof(cache->buf[0]), compare_elem_pos);
        printf("> %llu\n", i);
    }
    printf("\nSubarrays sorted, %ld sec\n\n", time(NULL) - cur_time);
}

void cache_ctor(struct cache_s* c, int file, time_t init_time)
{
    uint64_t file_size = get_file_size(file);

    const uint64_t cache_cnt_quota  = GB_2 / sizeof(struct input_elem);
    const uint64_t cache_cnt_needed = file_size / sizeof(struct input_elem);
    c->cnt = (cache_cnt_quota > cache_cnt_needed) ? cache_cnt_needed : cache_cnt_quota;
    c->size = c->cnt * sizeof(struct cache_entry);

    int cache_file = open("cache", O_RDWR|O_CREAT, 0666);
    assert(cache_file > 0);
    ssize_t r = ftruncate(cache_file, 0);
    assert(r >= 0);

    struct cache_entry_meta* wanted;
    wanted = (struct cache_entry_meta*)malloc(c->cnt * sizeof(*wanted));
    /* Find which elements we want to be stored in cache */
    printf("Finding elements we want to store in cache, %ld sec\n",
            time(NULL) - init_time);
    fill_wanted_elements(wanted, file_size, c->cnt);
    printf("Sorting them, %ld sec\n",
            time(NULL) - init_time);
    qsort((void*)wanted, c->cnt, sizeof(wanted[0]), compare_elem_pos);

    /* Go through file and store wanted elements in cache file */
    printf("Filling cache file, %ld sec\n",
            time(NULL) - init_time);
    fill_cache_file(file, cache_file, wanted, c->cnt);

    free(wanted); wanted = NULL;
    c->buf = malloc(c->size);
    /* Read cache file contents and sort cache */
    read_cache_file(c, cache_file, init_time);

    /* Remove temp file */
    close(cache_file);
    r = unlink("cache");
    assert(r >= 0);
}
