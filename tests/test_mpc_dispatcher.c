#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "stream_engine.h"

// Forward declaration
pipeline_stage_t *stage_mpc_dispatcher_create(uint32_t soc_tag, uint32_t out_pcs_tag, double dt, double capacity_ah, double target_soc);

static void test_mpc_dispatcher_logic(void **state) {
    (void)state;
    // SOC Tag: 1001, PCS Out Tag: 2001
    // dt=1s, Capacity=100Ah, Target SOC=50%
    pipeline_stage_t *mpc = stage_mpc_dispatcher_create(1001, 2001, 1.0, 100.0, 50.0);
    assert_non_null(mpc);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;

    // 1. Current SOC = 40% (below target 50%)
    ev.tag_id = 1001;
    ev.payload.val.f = 40.0;
    assert_int_equal(mpc->process(mpc, &ev), STAGE_OK);
    assert_int_equal(ev.tag_id, 2001);
    // Should result in charging (positive power/current)
    printf("MPC Out (SOC 40%%): %.2f A\n", ev.payload.val.f);
    assert_true(ev.payload.val.f > 0);

    // 2. Current SOC = 60% (above target 50%)
    ev.tag_id = 1001;
    ev.payload.val.f = 60.0;
    assert_int_equal(mpc->process(mpc, &ev), STAGE_OK);
    assert_int_equal(ev.tag_id, 2001);
    // Should result in discharging (negative power/current)
    printf("MPC Out (SOC 60%%): %.2f A\n", ev.payload.val.f);
    assert_true(ev.payload.val.f < 0);

    // 3. Current SOC = 50% (at target)
    ev.tag_id = 1001;
    ev.payload.val.f = 50.0;
    assert_int_equal(mpc->process(mpc, &ev), STAGE_OK);
    assert_int_equal(ev.tag_id, 2001);
    // Should result in ~0 power
    printf("MPC Out (SOC 50%%): %.2f A\n", ev.payload.val.f);
    assert_true(fabs(ev.payload.val.f) < 0.1);

    // Cleanup
    if (mpc->cleanup_state) mpc->cleanup_state(mpc->priv_state);
    free(mpc);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mpc_dispatcher_logic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
