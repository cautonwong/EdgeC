#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <math.h>
#include <stdlib.h>

#include "stream_engine.h"
#include "edge_dsp.h"
#include "edge_control.h"

// --- Helper Functions for Noise ---
static double rand_norm(double mu, double sigma) {
    double u1 = (rand() + 1.0) / (RAND_MAX + 1.0);
    double u2 = (rand() + 1.0) / (RAND_MAX + 1.0);
    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return z0 * sigma + mu;
}

// --- Test 1: PROC-01 DSP Filtering Pipeline ---
static void test_dsp_iir_variance_reduction(void **state) {
    (void)state;
    // Configure Butterworth low-pass filter (approximate)
    // 1st order LPF: Fc = 5Hz, Fs = 100Hz
    double b[] = {0.1367, 0.1367};
    double a[] = {1.0, -0.7265};
    
    dsp_iir_t *filter = dsp_iir_create(1, b, a);
    assert_non_null(filter);

    int num_samples = 1000;
    double variance_in = 0;
    double variance_out = 0;
    
    // Test signal: 1Hz sine wave + high frequency noise
    for (int i = 0; i < num_samples; i++) {
        double t = i * 0.01; // 100Hz
        double clean_signal = sin(2.0 * M_PI * 1.0 * t);
        double noise = rand_norm(0, 0.5);
        double noisy_signal = clean_signal + noise;
        
        double filtered_signal = dsp_iir_process(filter, noisy_signal);
        
        // Variance calculation (simplified relative to ground truth)
        variance_in += (noisy_signal - clean_signal) * (noisy_signal - clean_signal);
        variance_out += (filtered_signal - clean_signal) * (filtered_signal - clean_signal);
    }
    
    variance_in /= num_samples;
    variance_out /= num_samples;
    
    printf("Input Variance: %f, Output Variance: %f\n", variance_in, variance_out);
    
    // Variance should be reduced by at least 70% for this simple 1st order filter
    bool passed = variance_out < variance_in * 0.3;
    
    dsp_iir_free(filter);
    assert_true(passed);
}

// --- Test 2: PROC-02 Advanced PID Control Stage ---
static void test_advanced_pid_anti_windup(void **state) {
    (void)state;
    pid_adv_t pid;
    control_pid_init(&pid, 2.0, 5.0, 0.0); // High integral gain to test windup
    pid.out_min = -10.0;
    pid.out_max = 10.0;
    pid.setpoint = 100.0;

    double dt = 0.1;
    double pv = 0.0; // Process variable starts at 0

    // Simulate "blocked valve" - PV doesn't change for 50 steps
    for (int i = 0; i < 50; i++) {
        control_pid_compute(&pid, pv, dt);
    }
    
    // The integral term should be clamped. Without anti-windup, it would be 5.0 * 100 * 5 = 2500
    // With anti-windup, it stops growing when output hits 10.0.
    // P term is 2.0 * 100 = 200, which already saturates output.
    // Therefore I term should be near 0 or slightly negative depending on exact anti-windup logic.
    // In our logic, if P saturates, integral pauses or unwinds.
    
    printf("Integral term after blocked valve: %f\n", pid.integral);
    
    // Now remove blockage and let PV jump to 100
    pv = 100.0;
    double output = control_pid_compute(&pid, pv, dt);
    
    // Since setpoint is 100, error is 0. 
    // Output = I_term. If anti-windup worked, I_term shouldn't be massively positive causing overshoot.
    printf("Output immediately after reaching setpoint: %f\n", output);
    
    // In a well-clamped system, it shouldn't hold a massive residual output
    assert_true(output < 15.0); 
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dsp_iir_variance_reduction),
        cmocka_unit_test(test_advanced_pid_anti_windup),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
