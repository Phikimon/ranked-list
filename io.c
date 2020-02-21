#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "io.h"
#include "util.h"

uint64_t get_file_size(int fd)
{
    struct stat buf = {0};
    int r = fstat(fd, &buf);
    assert(r >= 0);
    return (uint64_t)buf.st_size;
}

void pwrite_buffer(int file, const void* buf, uint64_t nbyte, uint64_t offset)
{
    ssize_t r = 0;
    uint64_t to_write = 0;
    uint64_t sum = 0;

    do {
        to_write = (nbyte - sum > IO_BLOCK_SIZE) ?
                    IO_BLOCK_SIZE : (nbyte - sum);

        r = pwrite(file, (char*)buf + sum, to_write, offset + sum);
        assert(r == to_write);

        sum += to_write;
    } while (sum < nbyte);

    r = fsync(file);
    assert(r >= 0);
}

void pread_buffer(int file, void* buf, uint64_t* nbyte, uint64_t offset)
{
    ssize_t r = 0;
    uint64_t sum = 0;

    do {
        r = pread(file, (char*)buf + sum, IO_BLOCK_SIZE, offset + sum);
        assert(r >= 0);
        sum += r;
    } while ((r > 0) && (sum < PART_SIZE));

    *nbyte = (sum > PART_SIZE) ? PART_SIZE : sum;
}

void buffered_write(struct buf_io* s, void* data, uint64_t size, int flush)
{
    assert(s);
    assert(size <= sizeof(s->buf));
    assert(s->file >= 0);
    assert(s->bytes_used <= sizeof(s->buf));

    ssize_t r;

    if (size + s->bytes_used > sizeof(s->buf)) {
        r = write(s->file, s->buf, s->bytes_used);
        assert(r == s->bytes_used);
        s->bytes_used = 0;
    }

    if (data) {
        memcpy(s->buf + s->bytes_used, data, size);
        s->bytes_used += size;
    }

    if (flush) {
        r = write(s->file, s->buf, s->bytes_used);
        assert(r == s->bytes_used);
        s->bytes_used = 0;
    }
}

void buffered_read(struct buf_io* s, void* data, uint64_t size)
{
    assert(s);
    assert(size <= sizeof(s->buf));
    assert(s->file >= 0);
    assert(s->bytes_used <= sizeof(s->buf));
    ssize_t r;

    if (s->offset_in_buf == s->bytes_used) {
        r = pread(s->file, s->buf,
                  IO_BLOCK_SIZE, s->offset);
        assert(r > 0);
        s->bytes_used = r;
        s->offset_in_buf = 0;
    }

    memcpy(data, &s->buf[s->offset_in_buf], size);
    s->offset_in_buf += size;
    s->offset += size;
}
