#ifndef LIST_UNWRAP_H
#define LIST_UNWRAP_H

#include <time.h>
#include "cache.h"

void unwrap_list(struct cache_s* cache,
                 int             input_file,
                 int             output_file,
                 time_t          cur_time);

#endif
