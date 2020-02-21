#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 65536

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printf("File name wanted\n");
		return 1;
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd <= 0)
		return 1;

	uint64_t buf[BUF_SIZE / sizeof(uint64_t)];
	int read_ret;
	do {
		read_ret = read(fd, buf, sizeof(buf));
		assert(read_ret % (sizeof(uint64_t) * 2) == 0);
		for (int i = 0; i < read_ret / (sizeof(uint64_t) * 2); i++) {
			printf("%llu %llu\n", buf[i * 2], buf[i * 2 + 1]);
		}
	} while (read_ret > 0);
	return 0;
}
