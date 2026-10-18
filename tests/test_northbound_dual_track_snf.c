/**
 * @file test_northbound_dual_track_snf.c
 * @brief Unit tests for Dual-Track Northbound Relay (IEC 104 & MQTT) with SnF Resuming (AEP-463 /
 * Issue #150)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "northbound_dual_track_snf.h"

static void test_dual_track_online_dispatch(void) {
    printf("[TEST] Dual-Track Online Dispatch (IEC 104 & MQTT)...\n");

    northbound_dual_track_engine_t engine;
    northbound_dual_track_init(&engine, 100);

    northbound_record_t rec = {.tag_id = 1001,
                               .device_id = 1,
                               .value = 220.5,
                               .timestamp_ms = 1700000000000ULL,
                               .quality = 0,
                               .is_soe_event = false};

    int rc = northbound_dual_track_dispatch(&engine, &rec);
    assert(rc == 0);
    assert(engine.total_realtime_sent == 2);  // 1 for IEC 104 + 1 for MQTT
    assert(northbound_dual_track_get_snf_count(&engine) == 0);

    printf("  -> PASSED: Normal online dual-track dispatch verified.\n");
}

static void test_dual_track_offline_snf_and_ordered_playback(void) {
    printf("[TEST] Offline Store-and-Forward & Rate-Limited Chronological Replay...\n");

    northbound_dual_track_engine_t engine;
    const uint32_t rate_limit = 50;
    northbound_dual_track_init(&engine, rate_limit);

    // 1. Simulate MQTT cloud disconnect (IEC 104 still online)
    northbound_dual_track_set_channel_status(&engine, TRACK_MQTT, false);

    const uint32_t num_records = 1000;
    for (uint32_t i = 0; i < num_records; i++) {
        northbound_record_t rec = {
            .tag_id = 1000 + (i % 10),
            .device_id = i % 5,
            .value = 220.0 + (double)i * 0.1,
            .timestamp_ms =
                1700000000000ULL + (uint64_t)i * 100,  // Strictly monotonically increasing
            .quality = 0,
            .is_soe_event = (i % 50 == 0)};
        int rc = northbound_dual_track_dispatch(&engine, &rec);
        assert(rc == 0);
    }

    assert(northbound_dual_track_get_snf_count(&engine) == num_records);
    assert(engine.total_snf_buffered == num_records);
    // Realtime channel still sent realtime records
    assert(engine.total_realtime_sent == num_records);

    // 2. Simulate MQTT reconnection
    northbound_dual_track_set_channel_status(&engine, TRACK_MQTT, true);

    uint64_t last_ts = 0;
    uint32_t total_replayed = 0;
    northbound_record_t replay_batch[100];

    while (northbound_dual_track_get_snf_count(&engine) > 0) {
        uint32_t batch = northbound_dual_track_step_replay(&engine, replay_batch, 100);
        assert(batch > 0 && batch <= rate_limit);

        for (uint32_t k = 0; k < batch; k++) {
            assert(replay_batch[k].timestamp_ms >= last_ts);  // Strict chronological ordering!
            last_ts = replay_batch[k].timestamp_ms;
            total_replayed++;
        }
    }

    assert(total_replayed == num_records);
    assert(northbound_dual_track_get_snf_count(&engine) == 0);
    assert(engine.total_snf_replayed == num_records);

    printf("  -> PASSED: %u records buffered during disconnection and 100%% replayed in order.\n",
           num_records);
}

int main(void) {
    printf("=================================================================\n");
    printf("   AEP-463: Dual-Track Northbound SnF & Resuming Test Suite      \n");
    printf("=================================================================\n");

    test_dual_track_online_dispatch();
    test_dual_track_offline_snf_and_ordered_playback();

    printf("\nAll AEP-463 Dual-Track SnF tests PASSED!\n");
    return 0;
}
