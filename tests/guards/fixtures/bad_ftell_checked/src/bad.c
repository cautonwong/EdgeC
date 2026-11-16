#include <stdio.h>
#include <stdlib.h>

void *read_all(FILE *f, size_t *size) {
    fseek(f, 0, SEEK_END);
    *size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    return malloc(*size);
}

long unchecked_size(FILE *f) {
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    return size;
}
