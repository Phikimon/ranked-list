Problem:

There is a file filled with following structures:
```
struct node {
    uint64_t id;
    uint64_t next_id;
}
```
The .id field of the first structure in the file is equal to 1, the .next_id field of the last structure is equal to 0.

Task is to create another file that with the structures of the following format:
```
struct node {
    uint64_t id;
    uint64_t rank;
}
```
In this file `rank` is the number(starting with the 0) of the node that we met while jumping to next_id of the nodes.

Additional restrictions:
1. Process shall use no more that 2 GB of RAM.
2. 6 GB file on SSD machine has to be processed in no more that 1 hour.

Example:
+---------+---------+
|  Input  |  Output |
+---------+---------+
| 1 5     | 1 0     |
| 3 4     | 5 1     |
| 5 3     | 3 2     |
| 4 0     | 4 3     |
+---------+---------+

Solution:
1. Read 2GB file parts and sort them via qsort.
2. Use minheap to merge sort the whole file.
2. Find the most frequently read elements that are located in the
   nodes of the binary search tree. Go through the whole file and
   store these elements in cache file.
4. Read contents of cache file and sort them first by depth(to allow
   more frequently used nodes to be found more quickly), and then by
   offset in file.
5. Unwrap the list using binary search to find next list element in file.
   Caching allows this process to complete faster.

util/ directory stores code to measure RAM usage of a process as well as to
create the file described in the problem specification.
