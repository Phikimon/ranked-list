#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "util.h"
#include "sort_file.h"
#include "cache.h"
#include "list_unwrap.h"

int main(int argc, char* argv[])
{
    const char* input_name   = "input";
    const char output_name[] = "output";
    printf("Input file name is '%s', output file name is '%s'\n",
            input_name, output_name);

    int r;

    int input_file = open(input_name, O_RDWR);
    assert(input_file > 0);

    int output_file = open(output_name, O_WRONLY|O_CREAT, 0666);
    assert(output_file > 0);
    r = ftruncate(output_file, 0);
    assert(r >= 0);

    time_t init_time = time(NULL);
    struct cache_s c = {};

    sort_file(input_file, init_time);
    cache_ctor(&c, input_file, init_time);
    unwrap_list(&c, input_file, output_file, init_time);

    putchar('\n');

    fsync(input_file);
    fsync(output_file);
    close(input_file);
    close(output_file);
}
