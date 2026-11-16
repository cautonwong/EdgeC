#include <stdio.h>

int size_of_file(FILE *f, long *out) {
    if (fseek(f, 0, SEEK_END) != 0) return -1;
    long size = ftell(f);
    if (size < 0) return -1;
    *out = size;
    return 0;
}
