#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>

#define DEPTH_NUM 12

struct cache_s {
    struct cache_entry* buf;
    struct cache_entry* depth_bufs[DEPTH_NUM + 1];
    uint64_t depth_buf_size[DEPTH_NUM];
    uint64_t size;
    uint64_t cnt;
    uint32_t max_depth;
};

struct cache_entry_meta {
    uint32_t depth;
    uint32_t elem_pos;
};

struct cache_entry {
    uint32_t depth;
    uint32_t elem_pos;
    uint64_t id;
};

void cache_ctor(struct cache_s* c, int file, time_t init_time);

int lookup_id_in_cache(struct cache_s* cache,
                       uint64_t        position,
                       uint32_t        depth,
                       uint64_t*       res);

#endif
