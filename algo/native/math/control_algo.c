/**
 * @file control_algo.c
 * @brief 现代控制理论实现 (PID, LQR, Observer, QP)
 */

#include "edge_control.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_blas.h>

void control_pid_init(pid_adv_t *pid, double p, double i, double d) {
    memset(pid, 0, sizeof(pid_adv_t));
    pid->Kp = p; pid->Ki = i; pid->Kd = d;
    pid->out_min = -100.0; pid->out_max = 100.0;
}

double control_pid_compute(pid_adv_t *pid, double measurement, double dt) {
    if (dt <= 0) return 0;
    double error = pid->setpoint - measurement;
    
    // 比例
    double p_term = pid->Kp * error;
    
    // 积分 (Anti-windup)
    pid->integral += error * dt;
    double i_term = pid->Ki * pid->integral;
    
    // 微分 (针对测量值微分，减少冲击)
    double derivative = (measurement - pid->last_input) / dt;
    double d_term = -pid->Kd * derivative;
    
    double output = p_term + i_term + d_term;
    
    // 抗饱和限幅
    if (output > pid->out_max) {
        output = pid->out_max;
        pid->integral -= error * dt; 
    } else if (output < pid->out_min) {
        output = pid->out_min;
        pid->integral -= error * dt;
    }
    
    pid->last_input = measurement;
    pid->last_error = error;
    return output;
}

void control_observer_update(control_observer_t *obs, const edge_matrix_t *u, double y, double dt) {
    uint32_t dim = edge_matrix_rows(obs->x_hat);
    
    // y_hat = C * x_hat
    edge_matrix_t *cx_mat = edge_matrix_alloc(1, 1);
    edge_matrix_mul(obs->C, obs->x_hat, cx_mat);
    double y_hat = edge_matrix_get(cx_mat, 0, 0);
    edge_matrix_free(cx_mat);
    
    double error = y - y_hat;
    
    // x_dot = A*x + B*u + L*(y - y_hat)
    edge_matrix_t *ax = edge_matrix_alloc(dim, 1);
    edge_matrix_t *bu = edge_matrix_alloc(dim, 1);
    edge_matrix_t *le = edge_matrix_alloc(dim, 1);
    
    edge_matrix_mul(obs->A, obs->x_hat, ax);
    edge_matrix_mul(obs->B, u, bu);
    
    edge_matrix_copy(le, obs->L);
    edge_matrix_scale(le, error);
    
    // x = x + x_dot * dt
    edge_matrix_add(ax, bu, ax);
    edge_matrix_add(ax, le, ax);
    edge_matrix_scale(ax, dt);
    edge_matrix_add(obs->x_hat, ax, obs->x_hat);
    
    edge_matrix_free(ax); edge_matrix_free(bu); edge_matrix_free(le);
}

int control_lqr_solve(const edge_matrix_t *A, const edge_matrix_t *B, 
                      const edge_matrix_t *Q, const edge_matrix_t *R, 
                      edge_matrix_t *K) {
    uint32_t n = edge_matrix_rows(A);
    uint32_t m = edge_matrix_cols(B);

    // 1. 计算 R_inv
    edge_matrix_t *R_inv = edge_matrix_alloc(m, m);
    if (edge_matrix_inv(R, R_inv) != 0) {
        edge_matrix_free(R_inv); return -1;
    }

    // 2. 构造 Hamiltonian 矩阵 H (2n x 2n)
    gsl_matrix *H = gsl_matrix_calloc(2 * n, 2 * n);
    
    // H11 = A
    for(uint32_t i=0; i<n; i++)
        for(uint32_t j=0; j<n; j++) gsl_matrix_set(H, i, j, edge_matrix_get(A, i, j));
    
    // H12 = -B * R_inv * B'
    edge_matrix_t *BT = edge_matrix_alloc(m, n);
    edge_matrix_transpose(B, BT);
    edge_matrix_t *BRB = edge_matrix_alloc(n, n);
    edge_matrix_t *tmp_BR = edge_matrix_alloc(m, n);
    edge_matrix_mul(R_inv, BT, tmp_BR);
    edge_matrix_mul(B, tmp_BR, BRB);
    for(uint32_t i=0; i<n; i++)
        for(uint32_t j=0; j<n; j++) gsl_matrix_set(H, i, j + n, -edge_matrix_get(BRB, i, j));
    
    // H21 = -Q
    for(uint32_t i=0; i<n; i++)
        for(uint32_t j=0; j<n; j++) gsl_matrix_set(H, i + n, j, -edge_matrix_get(Q, i, j));

    // H22 = -A'
    edge_matrix_t *AT = edge_matrix_alloc(n, n);
    edge_matrix_transpose(A, AT);
    for(uint32_t i=0; i<n; i++)
        for(uint32_t j=0; j<n; j++) gsl_matrix_set(H, i + n, j + n, -edge_matrix_get(AT, i, j));

    // 3. 特征值分解
    gsl_vector_complex *eval = gsl_vector_complex_alloc(2 * n);
    gsl_matrix_complex *evec = gsl_matrix_complex_alloc(2 * n, 2 * n);
    gsl_eigen_nonsymmv_workspace *w = gsl_eigen_nonsymmv_alloc(2 * n);
    gsl_eigen_nonsymmv(H, eval, evec, w);

    // 4. 提取稳定特征向量 (实部 < 0)
    gsl_matrix_complex *V_stable = gsl_matrix_complex_alloc(2 * n, n);
    uint32_t stable_count = 0;
    for (uint32_t i = 0; i < 2 * n && stable_count < n; i++) {
        gsl_complex z = gsl_vector_complex_get(eval, i);
        if (GSL_REAL(z) < 0) {
            gsl_vector_complex_view v = gsl_matrix_complex_column(evec, i);
            gsl_matrix_complex_set_col(V_stable, stable_count, &v.vector);
            stable_count++;
        }
    }

    if (stable_count < n) {
        gsl_eigen_nonsymmv_free(w); gsl_vector_complex_free(eval); gsl_matrix_complex_free(evec);
        gsl_matrix_complex_free(V_stable); gsl_matrix_free(H);
        edge_matrix_free(R_inv); edge_matrix_free(BT); edge_matrix_free(BRB); edge_matrix_free(tmp_BR); edge_matrix_free(AT);
        return -2;
    }

    // 5. 计算 P = V21 * V11_inv
    edge_matrix_t *V11 = edge_matrix_alloc(n, n);
    edge_matrix_t *V21 = edge_matrix_alloc(n, n);
    for(uint32_t i=0; i<n; i++) {
        for(uint32_t j=0; j<n; j++) {
            edge_matrix_set(V11, i, j, GSL_REAL(gsl_matrix_complex_get(V_stable, i, j)));
            edge_matrix_set(V21, i, j, GSL_REAL(gsl_matrix_complex_get(V_stable, i + n, j)));
        }
    }

    edge_matrix_t *V11_inv = edge_matrix_alloc(n, n);
    edge_matrix_inv(V11, V11_inv);
    edge_matrix_t *P = edge_matrix_alloc(n, n);
    edge_matrix_mul(V21, V11_inv, P);

    // 6. K = R_inv * B' * P
    edge_matrix_mul(tmp_BR, P, K);

    // 清理
    gsl_eigen_nonsymmv_free(w); gsl_vector_complex_free(eval); gsl_matrix_complex_free(evec);
    gsl_matrix_complex_free(V_stable); gsl_matrix_free(H);
    edge_matrix_free(R_inv); edge_matrix_free(BT); edge_matrix_free(BRB); edge_matrix_free(tmp_BR);
    edge_matrix_free(AT); edge_matrix_free(V11); edge_matrix_free(V21); edge_matrix_free(V11_inv); edge_matrix_free(P);

    return 0;
}

int control_qp_solve_hildreth(const edge_matrix_t *Q, const edge_matrix_t *c,
                               const edge_matrix_t *A, const edge_matrix_t *b,
                               edge_matrix_t *x) {
    uint32_t n = edge_matrix_rows(Q);
    uint32_t m = edge_matrix_rows(A);
    edge_matrix_t *Q_inv = edge_matrix_alloc(n, n);
    edge_matrix_inv(Q, Q_inv);

    edge_matrix_t *AT = edge_matrix_alloc(n, m);
    edge_matrix_transpose(A, AT);
    edge_matrix_t *tmp_AQ = edge_matrix_alloc(m, n);
    edge_matrix_mul(A, Q_inv, tmp_AQ);
    edge_matrix_t *H = edge_matrix_alloc(m, m);
    edge_matrix_mul(tmp_AQ, AT, H);

    edge_matrix_t *d = edge_matrix_alloc(m, 1);
    edge_matrix_copy(d, b);
    edge_matrix_t *tmp_AQc = edge_matrix_alloc(m, 1);
    edge_matrix_mul(tmp_AQ, c, tmp_AQc);
    edge_matrix_add(d, tmp_AQc, d);

    edge_matrix_t *lambda = edge_matrix_alloc(m, 1);
    for(uint32_t iter=0; iter<100; iter++) {
        for(uint32_t i=0; i<m; i++) {
            double w = edge_matrix_get(d, i, 0);
            for(uint32_t j=0; j<m; j++) {
                if(i != j) w += edge_matrix_get(H, i, j) * edge_matrix_get(lambda, j, 0);
            }
            double val = -w / edge_matrix_get(H, i, i);
            edge_matrix_set(lambda, i, 0, val > 0 ? val : 0);
        }
    }

    edge_matrix_t *ATl = edge_matrix_alloc(n, 1);
    edge_matrix_mul(AT, lambda, ATl);
    edge_matrix_add(ATl, c, ATl);
    edge_matrix_mul(Q_inv, ATl, x);
    edge_matrix_scale(x, -1.0);

    edge_matrix_free(Q_inv); edge_matrix_free(AT); edge_matrix_free(tmp_AQ);
    edge_matrix_free(H); edge_matrix_free(d); edge_matrix_free(tmp_AQc);
    edge_matrix_free(lambda); edge_matrix_free(ATl);
    return 0;
}
