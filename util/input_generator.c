#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>

/* To create 1 Gb input file this program consumes only 8 Mb of memory */

const uint64_t BITS_IN_UINT64 = CHAR_BIT * sizeof(uint64_t) / sizeof(char);

struct options {
    uint64_t list_size;
    int want_binary;
    int want_text;
};

struct list_elem {
    uint64_t id;
    uint64_t next_id;
};

void get_options(struct options* opts, int argc, char* argv[])
{
    assert(opts);

    if (argc < 3) {
        printf("Usage: ./generator <file_size> (bin/text/both)\n");
        exit(1);
    }

    uint64_t file_size;
    sscanf(argv[1], "%llu", &file_size);
    opts->list_size = file_size / (2 * sizeof(uint64_t));
    printf("file_size = %lgK = %lgM = %lgGb, list_size = %llu\n",
            file_size / 1024., file_size / (1024. * 1024.), file_size / (1024. * 1024. * 1024.), opts->list_size);

    if (!strcmp(argv[2], "bin"))
        opts->want_binary = 1;
    else if (!strcmp(argv[2], "text"))
        opts->want_text = 1;
    else if (!strcmp(argv[2], "both"))
        opts->want_binary = opts->want_text = 1;
    else {
        printf("Unrecognized option. Abort\n");
        exit(1);
    }
}

void set_spot_busy(uint64_t* mask, uint64_t spot_num)
{
    mask[spot_num / BITS_IN_UINT64] |= ((uint64_t)1 << (spot_num % BITS_IN_UINT64));
}

int is_spot_busy(uint64_t* mask, uint64_t spot_num)
{
    return !!(mask[spot_num / BITS_IN_UINT64] & ((uint64_t)1 << (spot_num % BITS_IN_UINT64)));
}


uint64_t get_available_spot(uint64_t* mask, uint64_t list_size, uint64_t spot_num)
{
    //Step one: scan this 64-bit element
    while (is_spot_busy(mask, spot_num) && (spot_num % BITS_IN_UINT64 != 0))
        spot_num = (spot_num + 1) % list_size;
    //Step two: skip all 0xffffffffffffffff elements
    while (mask[spot_num / BITS_IN_UINT64] == (~(uint64_t)0))
        spot_num = (spot_num + BITS_IN_UINT64) % list_size;
    //Step three: scan next 64-bit element
    while (is_spot_busy(mask, spot_num))
        spot_num = (spot_num + 1) % list_size;

    return spot_num;
}

uint64_t get_random_val(uint64_t list_size, uint64_t last_available_spot)
{
    static uint64_t i = 0;
    i++;


    if ((i * 100) / list_size < 85) {
        if (i % 1000 == 0) {
            double progress = ((double)i) / list_size;
            printf("\r%8lg%%", progress * 100);
        }
        return ((uint64_t)rand()) % list_size;
    } else {
        if (i % 10 == 0) {
            double progress = ((double)i) / list_size;
            printf("\r%8lg%%, rand_val = %9llu / 402653184", progress * 100, last_available_spot);
        }
        return last_available_spot;
    }

}

uint64_t get_cur_elem(uint64_t list_size)
{
    static uint64_t* busy_spots_mask = NULL;
    if (busy_spots_mask == NULL) {
        busy_spots_mask = (uint64_t*)calloc(list_size / BITS_IN_UINT64 + 1, sizeof(*busy_spots_mask));
        assert(busy_spots_mask);
        set_spot_busy(busy_spots_mask, list_size - 1);
        set_spot_busy(busy_spots_mask, 0);
        return 1;
    }

    static uint64_t last_available_spot = 0;
    uint64_t cur_elem = get_random_val(list_size, last_available_spot);
    last_available_spot = cur_elem = get_available_spot(busy_spots_mask, list_size, cur_elem);
    set_spot_busy(busy_spots_mask, cur_elem);

    return cur_elem + 1;
}

void produce_list(struct options* opts)
{
    assert(opts);

    uint64_t buf[512];
    uint64_t cur_elem = 0;

    /* Open files */
    int r;
    int bin_file, text_file;
    if (opts->want_binary) {
        bin_file = open("bin", O_WRONLY|O_CREAT, 0666);
        assert(bin_file > 0);
        int r = ftruncate(bin_file, 0);
        assert(r >= 0);
    }
#ifdef WANT_TEXT
    if (opts->want_text) {
        text_file = open("text", O_WRONLY|O_CREAT, 0666);
        assert(text_file > 0);
        int r = ftruncate(text_file, 0);
        assert(r >= 0);
    }
#endif

    /* Initialize randomizer */
    srand(time(NULL));

    putchar('\n');
    for (uint64_t i = 0; i < opts->list_size - 1; i++) {
        cur_elem = get_cur_elem(opts->list_size);
#ifdef WANT_TEXT
        if (opts->want_binary) {
#endif
            buf[(i * 2) % 512] = cur_elem;
            buf[((i * 2) % 512) + 1] = cur_elem + 1;
            if (((i * 2) % 512 == 510) || (i == opts->list_size - 2)) {
                r = write(bin_file, buf, (((i * 2) % 512) + 2) * sizeof(buf[0]));
                assert(r == (((i * 2) % 512) + 2) * sizeof(buf[0]));
            }
#ifdef WANT_TEXT
        }
        if (opts->want_text)
            dprintf(text_file, "%llu %llu\n", cur_elem, cur_elem + 1);
#endif
    }
    putchar('\n');

    /* Write last line and close files */
    if (opts->want_binary) {
        buf[0] = opts->list_size;
        buf[1] = 0;
        r = write(bin_file, buf, 2 * sizeof(buf[0]));
        assert(r == 2 * sizeof(buf[0]));
        fsync(bin_file);
        close(bin_file);
    }
#ifdef WANT_TEXT
    if (opts->want_text) {
        dprintf(text_file, "%llu %llu\n", opts->list_size, (uint64_t)0);
        fsync(text_file);
        close(text_file);
    }
#endif
}

int main(int argc, char* argv[])
{
    struct options opts = {0};

    get_options(&opts, argc, argv);

    produce_list(&opts);

    return 0;
}
