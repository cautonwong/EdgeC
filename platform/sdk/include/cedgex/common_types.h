#ifndef CEDGE_COMMON_TYPES_H
#define CEDGE_COMMON_TYPES_H

#include <stdint.h>
#include <time.h>

// --- Value Representation ---

typedef enum {
    CEDGEX_VAL_TYPE_FLOAT,
    CEDGEX_VAL_TYPE_INT,
    CEDGEX_VAL_TYPE_STRING
} cedgex_value_type_t;

typedef enum {
    CEDGEX_QUALITY_GOOD = 0,
    CEDGEX_QUALITY_BAD = 1,
    CEDGEX_QUALITY_OLD = 2,
} cedgex_quality_t;


// --- Data Point - The Unified Data Model ---

/**
 * @brief The unified data point structure, optimized for 32-byte alignment
 * to fit perfectly into CPU cache lines.
 */
typedef struct __attribute__((packed, aligned(8))) {
    uint64_t ts;      // 8 bytes: Unix millisecond timestamp
    uint32_t tag_id;  // 4 bytes: The unique integer ID for the data point
    uint8_t  type;    // 1 byte: cedgex_value_type_t
    uint8_t  quality; // 1 byte: cedgex_quality_t
    uint8_t  _pad[2]; // 2 bytes: Padding to align the union
    union {
        double   f_val;   // 8 bytes
        int64_t  i_val;   // 8 bytes
        char     s_val[16]; // 16 bytes for short strings, or a pointer to a net_buf
    } val;
} cedgex_datapoint_t; // Total: 8+4+1+1+2+16 = 32 bytes


// --- Helper function to get current time ---

static inline uint64_t current_ts_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

#endif // CEDGE_COMMON_TYPES_H
