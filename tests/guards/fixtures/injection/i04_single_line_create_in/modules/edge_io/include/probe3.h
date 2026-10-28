#ifndef PROBE3_H
#define PROBE3_H

#include <stdalign.h>
#include <stdint.h>

typedef struct probe3_s probe3_t;

typedef struct {
    alignas(max_align_t) uint8_t raw[64];
} probe3_storage_t;

probe3_t *probe3_create_in(probe3_storage_t *store);

#endif
