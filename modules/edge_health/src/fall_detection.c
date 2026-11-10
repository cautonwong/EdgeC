#include "fall_detection.h"
#include <math.h>
#include <string.h>

void fall_detector_init(fall_detector_t *detector, double threshold_svm) {
    if (!detector) return;
    memset(detector, 0, sizeof(fall_detector_t));
    detector->threshold_svm = (threshold_svm > 1.0) ? threshold_svm : 2.8;
}

bool fall_detector_feed(fall_detector_t *detector, const imu_sensor_sample_t *sample, double *out_svm, double *out_fall_prob) {
    if (!detector || !sample) return false;

    detector->window[detector->head] = *sample;
    detector->head = (detector->head + 1) % 32;
    if (detector->count < 32) {
        detector->count++;
    }

    // 1. Calculate Signal Vector Magnitude (SVM)
    double svm = sqrt(sample->ax * sample->ax + sample->ay * sample->ay + sample->az * sample->az);
    if (out_svm) {
        *out_svm = svm;
    }

    // 2. Quantized C inference (< 5ms execution target): Sigmoid SVM classification
    double fall_prob = 1.0 / (1.0 + exp(-2.5 * (svm - detector->threshold_svm)));
    if (out_fall_prob) {
        *out_fall_prob = fall_prob;
    }

    // Trigger fall alert if probability > 0.85
    return fall_prob > 0.85;
}
