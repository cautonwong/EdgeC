#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "fall_detection.h"

static void test_fall_detection_normal_and_fall(void** state) {
    (void)state;

    fall_detector_t detector;
    fall_detector_init(&detector, 2.8);

    // 1. Normal sitting/walking sample (1.0g gravity acceleration)
    imu_sensor_sample_t normal_sample = {
        .ax = 0.0, .ay = 0.0, .az = 1.0, .gx = 0.0, .gy = 0.0, .gz = 0.0};
    double svm = 0.0, fall_prob = 0.0;
    bool is_fall = fall_detector_feed(&detector, &normal_sample, &svm, &fall_prob);

    assert_float_equal(svm, 1.0, 0.001);
    assert_false(is_fall);
    assert_true(fall_prob < 0.2);

    // 2. High-impact fall sample (3.5g impact acceleration)
    imu_sensor_sample_t fall_sample = {
        .ax = 2.0, .ay = 2.0, .az = 2.2, .gx = 150.0, .gy = 80.0, .gz = 90.0};
    is_fall = fall_detector_feed(&detector, &fall_sample, &svm, &fall_prob);

    assert_true(svm > 3.0);
    assert_true(is_fall);
    assert_true(fall_prob > 0.85);

    printf("🔥 [SUCCESS] Fall detection algorithm test passed! (SVM=%.2fg, Prob=%.2f%%)\n", svm,
           fall_prob * 100.0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fall_detection_normal_and_fall),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
