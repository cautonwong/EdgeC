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
pipeline_stage_t* stage_wavelet_anomaly_create(uint32_t alarm_tag, double threshold, size_t size,
                                               uint32_t level);

static void test_wavelet_anomaly_detection(void** state) {
    (void)state;

    // Create stage: Alarm Tag 9999, Threshold 5.0, Window 256, Level 1
    pipeline_stage_t* stage = stage_wavelet_anomaly_create(9999, 5.0, 256, 1);
    assert_non_null(stage);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;
    ev.tag_id = 1001;

    int alarm_count = 0;

    // 1. Push clean sine wave (256 samples)
    for (int i = 0; i < 256; i++) {
        ev.payload.val.f = 10.0 * sin(2.0 * M_PI * i / 32.0);  // Sine wave amp 10
        if (stage->process(stage, &ev) == STAGE_OK) {
            if (ev.type == PIPEMSG_TYPE_EVENT) alarm_count++;
        }
    }
    // High-frequency detail of a smooth sine should be small
    assert_int_equal(alarm_count, 0);

    // 2. Push sine wave with a sharp transient
    // Window is 256. Analysis happens every window/4 = 64 samples.
    // We'll push another 256 samples, with a spike at i=100
    for (int i = 0; i < 256; i++) {
        ev.payload.val.f = 10.0 * sin(2.0 * M_PI * i / 32.0);
        if (i == 100) ev.payload.val.f += 50.0;  // Sharp spike!

        if (stage->process(stage, &ev) == STAGE_OK) {
            if (ev.type == PIPEMSG_TYPE_EVENT) {
                alarm_count++;
                assert_int_equal(ev.tag_id, 9999);
                // Payload param is (max_detail * 100)
                assert_true(ev.payload.event.param > 500);
            }
        }
    }

    // Should have detected the anomaly
    assert_true(alarm_count > 0);

    // Cleanup
    if (stage->cleanup_state) stage->cleanup_state(stage->priv_state);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_wavelet_anomaly_detection),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
