#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "sort_file.h"
#include "io.h"
#include "util.h"
#include "minheap.h"

static int compare_id(const void* ptr_1, const void* ptr_2)
{
    const uint64_t val_1 = ((struct input_elem*)ptr_1)->id;
    const uint64_t val_2 = ((struct input_elem*)ptr_2)->id;

    if (__builtin_expect(val_1 == val_2, 0)) {
        return 0;
    } else
    if (val_1 > val_2) {
        return 1;
    } else
        return -1;
}

static void sort_file_parts(int input_file,
                            int output_file,
                            uint64_t file_size,
                            uint64_t parts_number,
                            time_t init_time)
{
    struct input_elem* list = malloc(GB_2);
    assert(list);
    uint64_t list_size;

    for (int i = 0; i < parts_number; i++) {
        printf("Filling buffer #%d, %ld sec\n", i, time(NULL) - init_time);
        pread_buffer(input_file, list, &list_size, i * PART_SIZE);

        printf("Sorting buffer #%d of size %s, %ld sec\n",
                i, size_all_units(list_size),
                time(NULL) - init_time);

        (void)qsort((void*)list, list_size / sizeof(list[0]),
                    sizeof(list[0]), compare_id);

        printf("Writing buffer #%d, %ld sec\n", i, time(NULL) - init_time);
        pwrite_buffer(output_file, list, list_size, i * PART_SIZE);

        printf("Buffer #%d written, %ld sec\n\n", i, time(NULL) - init_time);
    }

    free(list); list = NULL;
}

static void merge_sort(int input_file,
                       int output_file,
                       uint64_t file_size,
                       uint64_t parts_num,
                       time_t init_time)
{
    struct buf_io* read_buf  = (struct buf_io*)calloc(parts_num, sizeof(*read_buf));
    struct buf_io* write_buf = (struct buf_io*)calloc(1,         sizeof(*write_buf));
    write_buf->file = output_file;
    struct minheap_node root;
    struct minheap hp;

    /* Initialize heap with single element from each file part */
    minheap_ctor(&hp, parts_num, PART_SIZE, input_file);

    /* Initialize offsets to start reading from each file part */
    for (int i = 0; i < parts_num; i++) {
        read_buf[i].offset = i * PART_SIZE + sizeof(root.element);
        read_buf[i].file = input_file;
    }

    uint64_t counter = 0;
    for (int i = 0; i < parts_num; ) {
        counter += sizeof(root.element);
        /* Write min value in heap to the output file */
        minheap_get_min(&hp, &root);

        buffered_write(write_buf, &root.element, sizeof(root.element), NOFLUSH);

        uint64_t part_end = (root.i == parts_num - 1) ?
                             file_size                :
                            (root.i + 1) * PART_SIZE  ;

        if (read_buf[root.i].offset != part_end) {
            buffered_read(&read_buf[root.i], &root.element,
                          sizeof(root.element));
        } else {
            root.element.id = UINT64_MAX;
            i++;
        }

        minheap_replace_min(&hp, &root);
        if (counter % (sizeof(root.element) * 1000) == 0)
            printf("\rMerge sort: %3lg%%, %ld sec    ",
                    100. * counter / file_size, time(NULL) - init_time);
    }
    printf("\rMerge sort: 100%%    \n");
    buffered_write(write_buf, NULL, 0, FLUSH);

    free(read_buf); read_buf = NULL;
}

void sort_file(int file, time_t init_time)
{
    const uint64_t file_size = get_file_size(file);
    printf("\nFile size = %s\n", size_all_units(file_size));
    uint64_t parts_number = ( file_size % (PART_SIZE) == 0 ) ?
                            (  file_size / (PART_SIZE)     ) :
                            (  file_size / (PART_SIZE) + 1 ) ;

    int temp_file = open("temp", O_RDWR|O_CREAT, 0666);
    assert(temp_file > 0);
    ssize_t r = ftruncate(temp_file, 0);
    assert(r >= 0);

    sort_file_parts(file, temp_file, file_size, parts_number, init_time);

    merge_sort(temp_file, file, file_size, parts_number, init_time);

    close(temp_file);
    r = unlink("temp");
    assert(r >= 0);
}
