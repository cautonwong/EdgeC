/**
 * @file edge_motor.h
 * @brief 工业级电机控制算法 (FOC: Clark, Park, SVPWM)
 */

#ifndef EDGE_MOTOR_H
#define EDGE_MOTOR_H

#include <stdint.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double alpha;
    double beta;
} coord_alphabeta_t;

typedef struct {
    double d;
    double q;
} coord_dq_t;

/**
 * @brief Clark 变换 (三相 abc -> 两相 alpha-beta)
 * 等幅值变换
 */
static inline void motor_clark(double a, double b, double c, coord_alphabeta_t *out) {
    out->alpha = a;
    out->beta = (b - c) / sqrt(3.0);
}

/**
 * @brief Park 变换 (alpha-beta -> dq)
 */
static inline void motor_park(const coord_alphabeta_t *in, double theta, coord_dq_t *out) {
    double cos_t = cos(theta);
    double sin_t = sin(theta);
    out->d = in->alpha * cos_t + in->beta * sin_t;
    out->q = -in->alpha * sin_t + in->beta * cos_t;
}

/**
 * @brief 逆 Park 变换 (dq -> alpha-beta)
 */
static inline void motor_inv_park(const coord_dq_t *in, double theta, coord_alphabeta_t *out) {
    double cos_t = cos(theta);
    double sin_t = sin(theta);
    out->alpha = in->d * cos_t - in->q * sin_t;
    out->beta = in->d * sin_t + in->q * cos_t;
}

/**
 * @brief SVPWM 产生占空比 (Seven-segment mode)
 * @param v_in 逆 Park 变换后的 alpha-beta 电压
 * @param v_dc 母线电压
 * @param t_pwm 周期
 * @param out_abc 输出三相占空比 [0.0, 1.0]
 */
void motor_svpwm(const coord_alphabeta_t *v_in, double v_dc, double *out_abc);

#ifdef __cplusplus
}
#endif

#endif
