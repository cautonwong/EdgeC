#ifndef FALL_DETECTION_H
#define FALL_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double ax, ay, az; // 3-axis Acceleration (g)
    double gx, gy, gz; // 3-axis Gyroscope (deg/s)
} imu_sensor_sample_t;

typedef struct {
    imu_sensor_sample_t window[32];
    int head;
    int count;
    double threshold_svm; // Signal Vector Magnitude threshold (e.g., 2.8g)
} fall_detector_t;

void fall_detector_init(fall_detector_t *detector, double threshold_svm);
bool fall_detector_feed(fall_detector_t *detector, const imu_sensor_sample_t *sample, double *out_svm, double *out_fall_prob);

#ifdef __cplusplus
}
#endif

#endif // FALL_DETECTION_H
