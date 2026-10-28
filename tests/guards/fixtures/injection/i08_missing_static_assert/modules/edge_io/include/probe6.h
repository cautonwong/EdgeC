#ifndef PROBE6_H
#define PROBE6_H

#include <stdalign.h>
#include <stdint.h>

typedef struct probe6_s probe6_t;

typedef struct {
    alignas(max_align_t) uint8_t raw[32];
} probe6_storage_t;

#endif
