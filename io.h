#ifndef IO_H
#define IO_H

#include <stdint.h>
#include "util.h"

#define IO_BLOCK_SIZE (MB * 2)

#define FLUSH   1
#define NOFLUSH 0

struct buf_io {
    /* Public */
    uint64_t offset;
    int file;
    /* Private */
    char buf[IO_BLOCK_SIZE];
    uint64_t bytes_used;
    uint64_t offset_in_buf;
};

void pwrite_buffer(int file, const void* buf, uint64_t nbyte, uint64_t offset);
void pread_buffer(int file, void* buf, uint64_t* nbyte, uint64_t offset);

void buffered_write(struct buf_io* s, void* data, uint64_t size, int flush);
void buffered_read (struct buf_io* s, void* data, uint64_t size);

uint64_t get_file_size(int fd);

#endif
