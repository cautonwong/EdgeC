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
pipeline_stage_t* stage_ekf_soc_create(uint32_t current_tag, uint32_t voltage_tag,
                                       uint32_t soc_out_tag);

static void test_ekf_soc_convergence(void** state) {
    (void)state;
    // Current Tag: 1, Voltage Tag: 2, SOC Out Tag: 3
    pipeline_stage_t* soc_stage = stage_ekf_soc_create(1, 2, 3);
    assert_non_null(soc_stage);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;

    // Initial SOC is 50% (0.5) in code
    // OCV = 3.0 + 1.2 * 0.5 = 3.6V
    // With 0 current, Voltage should be 3.6V

    // 1. Push current 0A
    ev.tag_id = 1;
    ev.payload.val.f = 0.0;
    ev.ts = 1000;
    assert_int_equal(soc_stage->process(soc_stage, &ev), STAGE_DROP);

    // 2. Push voltage 3.6V
    ev.tag_id = 2;
    ev.payload.val.f = 3.6;
    ev.ts = 2000;  // dt = 1s
    assert_int_equal(soc_stage->process(soc_stage, &ev), STAGE_OK);
    assert_int_equal(ev.tag_id, 3);

    double soc = ev.payload.val.f;
    printf("Initial SOC: %.2f%%\n", soc);
    assert_true(soc >= 49.0 && soc <= 51.0);

    // 3. Discharge at 10A for some time
    // Cn = 100Ah, R0 = 0.05, R1 = 0.02, C1 = 1000.0
    double true_vp = 0.0;
    double r1 = 0.02;
    double c1 = 1000.0;
    double r0 = 0.05;

    for (int i = 0; i < 3600; i++) {
        double dt = 1.0;

        // Push current 10A
        ev.tag_id = 1;
        ev.payload.val.f = 10.0;
        ev.ts += (uint64_t)(dt * 1000);
        soc_stage->process(soc_stage, &ev);

        // Update Simulated Battery Model:
        double true_soc = 0.5 - (10.0 * (double)(i + 1)) / 360000.0;
        double rc_factor = exp(-dt / (r1 * c1));
        true_vp = true_vp * rc_factor + r1 * 10.0 * (1.0 - rc_factor);

        // V = OCV(SOC) - Vp - I*R0
        ev.tag_id = 2;
        ev.payload.val.f = (3.0 + 1.2 * true_soc) - true_vp - 10.0 * r0;
        ev.ts += (uint64_t)(dt * 1000);
        soc_stage->process(soc_stage, &ev);

        if (i % 600 == 0) {
            printf("Step %d: Est SOC=%.2f%%, True SOC=%.2f%%, V=%.3f, Vp=%.3f\n", i,
                   ev.payload.val.f, true_soc * 100.0, ev.payload.val.f, true_vp);
        }
    }

    soc = ev.payload.val.f;
    printf("Final SOC: %.2f%%\n", soc);

    bool pass = (soc >= 39.5 && soc <= 40.5);

    // Cleanup
    if (soc_stage->cleanup_state) soc_stage->cleanup_state(soc_stage->priv_state);
    free(soc_stage);

    assert_true(pass);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ekf_soc_convergence),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
