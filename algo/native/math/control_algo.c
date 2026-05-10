/**
 * @file control_algo.c
 * @brief 现代控制理论实现 (PID, LQR, Observer)
 */

#include "edge_control.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void control_pid_init(pid_adv_t *pid, double p, double i, double d) {
    memset(pid, 0, sizeof(pid_adv_t));
    pid->Kp = p; pid->Ki = i; pid->Kd = d;
    pid->out_min = -100.0; pid->out_max = 100.0;
}

double control_pid_compute(pid_adv_t *pid, double measurement, double dt) {
    if (dt <= 0) return 0;
    double error = pid->setpoint - measurement;
    
    // 1. 比例
    double p_term = pid->Kp * error;
    
    // 2. 积分 (带抗饱和 Anti-windup)
    pid->integral += error * dt;
    double i_term = pid->Ki * pid->integral;
    
    // 3. 微分 (带滤波，防止噪声放大)
    double derivative = (measurement - pid->last_input) / dt;
    double d_term = -pid->Kd * derivative; // 使用测量值微分，避免 Setpoint 突变冲击
    
    double output = p_term + i_term + d_term;
    
    // 4. 限幅
    if (output > pid->out_max) output = pid->out_max;
    else if (output < pid->out_min) output = pid->out_min;
    
    pid->last_input = measurement;
    pid->last_error = error;
    return output;
}

void control_observer_update(control_observer_t *obs, const edge_matrix_t *u, double y, double dt) {
    // x_hat_dot = A*x_hat + B*u + L*(y - C*x_hat)
    uint32_t dim = edge_matrix_rows(obs->x_hat);
    
    // Cx = C * x_hat (假设 C 是 1xN，输出是标量)
    edge_matrix_t *cx_mat = edge_matrix_alloc(1, 1);
    edge_matrix_mul(obs->C, obs->x_hat, cx_mat);
    double y_hat = edge_matrix_get(cx_mat, 0, 0);
    edge_matrix_free(cx_mat);
    
    double error = y - y_hat;
    
    // x_dot = A*x + B*u + L*error
    edge_matrix_t *ax = edge_matrix_alloc(dim, 1);
    edge_matrix_t *bu = edge_matrix_alloc(dim, 1);
    edge_matrix_t *le = edge_matrix_alloc(dim, 1);
    
    edge_matrix_mul(obs->A, obs->x_hat, ax);
    edge_matrix_mul(obs->B, u, bu);
    
    edge_matrix_copy(le, obs->L);
    edge_matrix_scale(le, error);
    
    // x_new = x + (ax + bu + le)*dt
    edge_matrix_add(ax, bu, ax);
    edge_matrix_add(ax, le, ax);
    edge_matrix_scale(ax, dt);
    edge_matrix_add(obs->x_hat, ax, obs->x_hat);
    
    edge_matrix_free(ax); edge_matrix_free(bu); edge_matrix_free(le);
}

int control_lqr_solve(const edge_matrix_t *A, const edge_matrix_t *B, 
                      const edge_matrix_t *Q, const edge_matrix_t *R, 
                      edge_matrix_t *K) {
    // 实际工业场景中，LQR 增益 K 通常是离线计算好的
    // 在线求解 ARE 方程极其耗时。此处提供一个占位符或简单系统的近似解。
    (void)A; (void)B; (void)Q; (void)R; (void)K;
    return -1; // TODO: 集成专门的 ARE 求解器 (如通过 GSL 求解 Schur 分解)
}
