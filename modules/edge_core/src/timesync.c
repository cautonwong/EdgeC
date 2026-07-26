#include "timesync.h"
#include <string.h>

void timesync_init(timesync_clock_t *clk) {
    if (!clk) return;
    memset(clk, 0, sizeof(timesync_clock_t));
}

void timesync_update(timesync_clock_t *clk, int64_t t1_ns, int64_t t2_ns, int64_t t3_ns, int64_t t4_ns) {
    if (!clk) return;

    // PTP / NTP IEEE 1588 4-timestamp formula:
    // Offset = ((t2 - t1) + (t3 - t4)) / 2
    // Delay  = ((t4 - t1) - (t3 - t2)) / 2
    int64_t offset = ((t2_ns - t1_ns) + (t3_ns - t4_ns)) / 2;

    clk->offset_ns = offset;
    clk->last_sync_time = (uint64_t)t4_ns;
    clk->is_synced = true;
    clk->sync_count++;
}

int64_t timesync_get_adjusted_time_ns(const timesync_clock_t *clk, int64_t local_raw_ns) {
    if (!clk || !clk->is_synced) {
        return local_raw_ns;
    }
    return local_raw_ns + clk->offset_ns;
}
