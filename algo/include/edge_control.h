/**
 * @file edge_control.h
 * @brief 现代控制理论算法接口 (LQR, Observers, Advanced PID)
 */

#ifndef EDGE_CONTROL_H
#define EDGE_CONTROL_H

#include <stdint.h>
#include <stddef.h>
#include "edge_linalg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- 1. 高级 PID --- */
typedef struct {
    double Kp, Ki, Kd;
    double setpoint;
    double out_min, out_max;
    double deadband;
    // 内部状态
    double integral;
    double last_error;
    double last_input;
} pid_adv_t;

void control_pid_init(pid_adv_t *pid, double p, double i, double d);
double control_pid_compute(pid_adv_t *pid, double measurement, double dt);

/* --- 2. 状态空间控制 --- */

/**
 * @brief 求解代数 Riccati 方程 (ARE) 并返回最优增益 K (LQR)
 * 方程: A'P + PA - PBR^-1B'P + Q = 0
 */
int control_lqr_solve(const edge_matrix_t *A, const edge_matrix_t *B, 
                      const edge_matrix_t *Q, const edge_matrix_t *R, 
                      edge_matrix_t *K);

/**
 * @brief Hildreth 过程求解二次规划 (QP) - 适用于小规模多约束 MPC
 * 极小化: 1/2 x' Q x + c' x  满足: A x <= b
 */
int control_qp_solve_hildreth(const edge_matrix_t *Q, const edge_matrix_t *c,
                               const edge_matrix_t *A, const edge_matrix_t *b,
                               edge_matrix_t *x);

/**
 * @brief 状态观测器 (Luenberger Observer)
 * x_hat_dot = A*x_hat + B*u + L*(y - C*x_hat)
 */
typedef struct {
    edge_matrix_t *A, *B, *C, *L;
    edge_matrix_t *x_hat;
} control_observer_t;

void control_observer_update(control_observer_t *obs, const edge_matrix_t *u, double y, double dt);

#ifdef __cplusplus
}
#endif

#endif
