#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "edge_control.h"

static void test_pid_anti_windup(void **state) {
    (void)state;
    pid_adv_t pid;
    
    // Kp=1.0, Ki=10.0, Kd=0.0
    // Limits: [-10, 10]
    control_pid_init(&pid, 1.0, 10.0, 0.0);
    pid.out_min = -10.0;
    pid.out_max = 10.0;
    pid.setpoint = 100.0; // Large setpoint to force saturation
    
    // Step 1: Multiple iterations with saturation
    for (int i = 0; i < 10; i++) {
        double out = control_pid_compute(&pid, 0.0, 1.0); // 1s steps
        assert_true(out <= 10.0);
    }
    
    // Without anti-windup, integral would be 1000.0 (100 * 1.0 * 10)
    // and i_term would be 10000.0.
    // With anti-windup, we freeze integral when output > max.
    // Initial error = 100.
    // Iter 1: p=100, i=1000 -> out=10100 -> Clamped to 10. Integral frozen at 0?
    // Wait, the logic I wrote was:
    /*
    if (output > pid->out_max) {
        output = pid->out_max;
        pid->integral -= error * dt; 
    }
    */
    // So it subtracts the increment.
    
    printf("Final PID Integral: %.2f\n", pid.integral);
    
    // Step 2: Reverse setpoint. It should respond immediately if not wound up.
    pid.setpoint = -100.0;
    double out_rev = control_pid_compute(&pid, 0.0, 1.0);
    
    // If it was wound up to 1000, error=-100 would just bring it down slightly.
    // If anti-windup worked, integral is small, and error=-100 should immediately drive output negative.
    printf("Output after setpoint reversal: %.2f\n", out_rev);
    assert_true(out_rev < 0.0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pid_anti_windup),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
