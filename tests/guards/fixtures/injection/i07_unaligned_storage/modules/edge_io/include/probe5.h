#ifndef PROBE5_H
#define PROBE5_H

#include <stdint.h>

typedef struct probe5_s probe5_t;

/* #411 probe #7: storage placeholder without alignas -- unsafe physical
 * alignment for placement construction. */
typedef struct {
    uint8_t raw[64];
} probe5_storage_t;

#endif
