#ifndef TIMESYNC_H
#define TIMESYNC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int64_t offset_ns;        // Time offset in nanoseconds (Local - Remote)
    double frequency_ppm;     // Clock frequency drift in parts-per-million (PPM)
    uint64_t last_sync_time;  // Timestamp of last successful sync
    bool is_synced;           // Synchronized status
    uint32_t sync_count;      // Total sync attempts
} timesync_clock_t;

void timesync_init(timesync_clock_t *clk);

void timesync_update(timesync_clock_t *clk, int64_t t1_ns, int64_t t2_ns, int64_t t3_ns, int64_t t4_ns);

int64_t timesync_get_adjusted_time_ns(const timesync_clock_t *clk, int64_t local_raw_ns);

#ifdef __cplusplus
}
#endif

#endif // TIMESYNC_H
