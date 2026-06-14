#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_engine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declaration
pipeline_stage_t* stage_notch_filter_create(double f0, double fs, double q_factor,
                                            uint32_t output_tag);

static void test_notch_filter_60hz(void** state) {
    (void)state;

    // Create stage: 60Hz notch, 1000Hz sampling rate, Q=30, output tag 1002
    pipeline_stage_t* stage = stage_notch_filter_create(60.0, 1000.0, 30.0, 1002);
    assert_non_null(stage);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;
    ev.tag_id = 1001;

    double fs = 1000.0;
    int num_samples = 2000;

    double max_1hz = 0.0;
    double max_filtered = 0.0;

    // 1. Generate combined signal: 1Hz (Signal) + 60Hz (Noise)
    // Both with amplitude 1.0
    for (int i = 0; i < num_samples; i++) {
        double t = (double)i / fs;
        double sig_1hz = sin(2.0 * M_PI * 1.0 * t);
        double noise_60hz = sin(2.0 * M_PI * 60.0 * t);

        ev.payload.val.f = sig_1hz + noise_60hz;

        assert_int_equal(stage->process(stage, &ev), STAGE_OK);
        assert_int_equal(ev.tag_id, 1002);

        // Skip first 500 samples (0.5s) to allow IIR transient response to settle
        if (i > 500) {
            double err = fabs(ev.payload.val.f - sig_1hz);
            if (err > max_filtered) max_filtered = err;
            if (fabs(sig_1hz) > max_1hz) max_1hz = fabs(sig_1hz);
        }
    }

    printf("Max error between filtered and pure 1Hz signal: %.4f\n", max_filtered);

    // The pure 1Hz signal max is ~1.0.
    // The 60Hz noise amplitude was 1.0.
    // We expect the residual 60Hz noise (max error) to be < 0.05 (>26dB attenuation)
    // Actually, at the exact notch frequency, it should be heavily attenuated.
    assert_true(max_filtered < 0.05);

    // Cleanup
    if (stage->cleanup_state) stage->cleanup_state(stage->priv_state);
    free(stage);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_notch_filter_60hz),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
