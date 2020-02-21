#include "util.h"
#include "stdio.h"

const char* size_all_units(uint64_t size)
{
    static char buf[64] = {};
    sprintf(buf, "%lluB = %lluKB = %lluMB = %lluGB",
            size,      size / KB,
            size / MB, size / GB);
    return buf;
}
