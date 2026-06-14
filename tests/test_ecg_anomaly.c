#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_engine.h"

// Forward declaration
pipeline_stage_t* stage_ecg_anomaly_create(size_t buffer_size, uint32_t level, double threshold,
                                           uint32_t alarm_tag);

static void test_ecg_anomaly_detection(void** state) {
    (void)state;

    // Create stage: Window 256, Level 2 DWT, Threshold 150.0, Alarm Tag 9999
    pipeline_stage_t* stage = stage_ecg_anomaly_create(256, 2, 150.0, 9999);
    assert_non_null(stage);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;
    ev.tag_id = 1001;

    int alarm_count = 0;

    // 1. Push clean baseline ECG approximation (sine wave low frequency)
    for (int i = 0; i < 256; i++) {
        ev.payload.val.f = 5.0 * sin(2.0 * M_PI * i / 64.0);
        if (stage->process(stage, &ev) == STAGE_OK) {
            if (ev.type == PIPEMSG_TYPE_EVENT) alarm_count++;
        }
    }
    // High-frequency detail of a smooth wave should be small
    assert_int_equal(alarm_count, 0);

    // 2. Push ECG with a sharp QRS-like transient (PVC simulation)
    // Window is 256. Analysis happens every window/4 = 64 samples.
    // We'll push another 256 samples, with a high frequency sharp spike.
    for (int i = 0; i < 256; i++) {
        ev.payload.val.f = 5.0 * sin(2.0 * M_PI * i / 64.0);
        // Sharp PVC spike at i=100..102
        if (i >= 100 && i <= 102) {
            ev.payload.val.f += (i == 101) ? 50.0 : -20.0;
        }

        if (stage->process(stage, &ev) == STAGE_OK) {
            if (ev.type == PIPEMSG_TYPE_EVENT) {
                alarm_count++;
                assert_int_equal(ev.tag_id, 9999);
                // Payload param is (energy)
                assert_true(ev.payload.event.param > 150);
            }
        }
    }

    // Should have detected the anomaly
    assert_true(alarm_count > 0);

    // Cleanup
    if (stage->cleanup_state) stage->cleanup_state(stage->priv_state);
    free(stage);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ecg_anomaly_detection),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
