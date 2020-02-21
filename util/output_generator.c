#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#define BUF_SIZE 65536
#define BUF_CNT (BUF_SIZE / sizeof(uint64_t))

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printf("list size expected\n");
		return 1;
	}
	uint64_t list_size = atoi(argv[1]);
	printf("list_size = %llu\n", list_size);
	if (list_size * 2 > BUF_CNT) {
		printf("Too big list requested\n");
		return 1;
	}

	int fd = open("solution_bin", O_WRONLY|O_CREAT, 0777);

	uint64_t buf[BUF_CNT];
	for (uint64_t i = 0; i < list_size; i++) {
		buf[i * 2] = i + 2; buf[i * 2 + 1] = i + 1;
	}

	write(fd, buf, list_size * 2 * sizeof(uint64_t));

	fsync(fd);
	close(fd);
}
