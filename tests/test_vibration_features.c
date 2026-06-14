#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "stream_engine.h"

// Forward declaration
pipeline_stage_t* stage_vibration_features_create(size_t window_size, uint32_t rms_tag,
                                                  uint32_t kurt_tag, uint32_t skew_tag);

static void test_vibration_features_logic(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "feat_test", 64, 10000, NULL, 0, NULL, NULL, NULL);

    stream_engine_t* se = stream_engine_create(&hub, loop, NULL, NULL);
    stream_engine_start(se);

    // Create stage: 128 sample window
    pipeline_stage_t* stage = stage_vibration_features_create(128, 2001, 2002, 2003);
    assert_non_null(stage);

    pipeline_t* p = stream_engine_add_pipeline(se, "test_p", 1001);
    pipeline_add_stage(p, stage->name, stage->process, stage->priv_state, stage->cleanup_state,
                       NULL, NULL);
    free(stage);  // Pipeline takes ownership of parts, stage helper was temporary

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;
    ev.tag_id = 1001;

    // Push 128 samples of a constant 10.0
    // RMS should be 10.0
    // Kurtosis/Skew should be 0.0 (constant signal)
    for (int i = 0; i < 128; i++) {
        ev.payload.val.f = 10.0;
        ev.ts = uv_hrtime();
        stream_engine_push(se, &ev);
    }

    // Verify stats from shadow table (Polling for async result)
    pipe_msg_t m_rms;
    int found = -1;
    for (int retry = 0; retry < 50; retry++) {
        if (datahub_read_shadow(&hub, 2001, &m_rms) == 0) {
            found = 0;
            break;
        }
        usleep(10000);  // 10ms
    }

    assert_int_equal(found, 0);
    assert_float_equal(m_rms.payload.val.f, 10.0, 0.01);
    printf("Calculated RMS: %.2f\n", m_rms.payload.val.f);

    // Cleanup
    stream_engine_destroy(se);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_vibration_features_logic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
