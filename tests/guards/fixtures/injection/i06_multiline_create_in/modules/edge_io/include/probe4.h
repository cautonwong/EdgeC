#ifndef PROBE4_H
#define PROBE4_H

#include <stdalign.h>
#include <stdint.h>

typedef struct probe4_s probe4_t;

typedef struct {
    alignas(max_align_t) uint8_t raw[64];
} probe4_storage_t;

probe4_t *probe4_create_in(probe4_storage_t *store);

#endif
