#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUF_SIZE 65536

uint64_t get_file_size(int fd)
{
    struct stat buf = {0};
    int r = fstat(fd, &buf);
    assert(r >= 0);
    return (uint64_t)buf.st_size;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("File name wanted\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd <= 0)
        return 1;

    const uint64_t BUF_CNT = BUF_SIZE / sizeof(uint64_t);
    uint64_t to_read_total = get_file_size(fd);
    uint64_t buf[BUF_CNT];
    uint64_t list_size = to_read_total / (2 * sizeof(uint64_t));
    uint64_t bytes_read = 0, bytes_left = to_read_total;
    uint64_t bytes_to_read_cur;
    ssize_t read_ret;
    uint64_t index = 0;

    while (bytes_left > 0) {
        bytes_to_read_cur = (bytes_left > BUF_SIZE) ? BUF_SIZE : bytes_left;

        read_ret = read(fd, buf, bytes_to_read_cur);
        if (read_ret != bytes_to_read_cur) {
            perror("read");
            printf("bytes_to_read_cur = %llu, read = %zd\n", bytes_to_read_cur, read_ret);
        }
        assert(read_ret == bytes_to_read_cur);

        bytes_left -= bytes_to_read_cur;
        bytes_read += bytes_to_read_cur;

        for (uint64_t i = 0; i < bytes_to_read_cur / (2 * sizeof(uint64_t)); i++) {
            if ((buf[i * 2] != index + 1) || (buf[i * 2 + 1] != index)) {
                printf("Wrong solution at line %llu: %llu %llu(%llu %llu expected)\n", index, buf[i * 2], buf[i * 2 + 1], index + 1, index);
                return 1;
            }
            index++;
        }
    }

    printf("Correct solution! List size = %llu\n", index);
    return 0;
}
