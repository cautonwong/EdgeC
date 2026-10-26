/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

#include "timesync.h"

int main(void) {
    printf("[TEST] Testing TimeSync Clock Offset Compensator...\n");

    timesync_clock_t clk;
    timesync_init(&clk);

    assert(!clk.is_synced);

    // Simulated 4-timestamp exchange:
    // Local sends t1=1000000000ns
    // Remote receives t2=1000005000ns (+5us offset)
    // Remote sends t3=1000010000ns
    // Local receives t4=1000015000ns
    int64_t t1 = 1000000000LL;
    int64_t t2 = 1000005000LL;
    int64_t t3 = 1000010000LL;
    int64_t t4 = 1000015000LL;

    timesync_update(&clk, t1, t2, t3, t4);

    assert(clk.is_synced);
    assert(clk.offset_ns == 0);  // Symmetric delay in this synthetic setup

    // Test non-zero offset
    // t2=1000010000 (+10us offset)
    int64_t t2_offset = 1000010000LL;
    timesync_update(&clk, t1, t2_offset, t3, t4);

    assert(clk.offset_ns == 2500LL);

    int64_t raw_local = 2000000000LL;
    int64_t adjusted = timesync_get_adjusted_time_ns(&clk, raw_local);
    assert(adjusted == raw_local + 2500LL);

    printf("[TEST] TimeSync Clock Offset Compensator PASSED!\n");
    return 0;
}
