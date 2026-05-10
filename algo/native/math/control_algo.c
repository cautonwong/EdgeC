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

void control_ekf_predict(control_ekf_t *ekf, const edge_matrix_t *F, const edge_matrix_t *B, const edge_matrix_t *u) {
    // x = f(x, u) -> 此处假设外部已更新 x，仅更新 P
    // P = F*P*F' + Q
    edge_matrix_t *FT = edge_matrix_alloc(ekf->n, ekf->n);
    edge_matrix_transpose(F, FT);
    edge_matrix_t *tmp = edge_matrix_alloc(ekf->n, ekf->n);
    edge_matrix_mul(F, ekf->P, tmp);
    edge_matrix_mul(tmp, FT, ekf->P);
    edge_matrix_add(ekf->P, ekf->Q, ekf->P);
    edge_matrix_free(FT); edge_matrix_free(tmp);
}

void control_ekf_correct(control_ekf_t *ekf, const edge_matrix_t *H, const edge_matrix_t *y) {
    // S = H*P*H' + R
    uint32_t m = edge_matrix_rows(H);
    edge_matrix_t *HT = edge_matrix_alloc(ekf->n, m);
    edge_matrix_transpose(H, HT);
    edge_matrix_t *S = edge_matrix_alloc(m, m);
    edge_matrix_t *tmpPH = edge_matrix_alloc(ekf->n, m);
    edge_matrix_mul(ekf->P, HT, tmpPH);
    edge_matrix_mul(H, tmpPH, S);
    edge_matrix_add(S, ekf->R, S);
    
    // K = P*H' * inv(S)
    edge_matrix_t *S_inv = edge_matrix_alloc(m, m);
    edge_matrix_inv(S, S_inv);
    edge_matrix_t *K = edge_matrix_alloc(ekf->n, m);
    edge_matrix_mul(tmpPH, S_inv, K);
    
    // x = x + K*y
    edge_matrix_t *Ky = edge_matrix_alloc(ekf->n, 1);
    edge_matrix_mul(K, y, Ky);
    edge_matrix_add(ekf->x, Ky, ekf->x);
    
    // P = (I - K*H)*P
    edge_matrix_t *I = edge_matrix_alloc(ekf->n, ekf->n);
    edge_matrix_identity(I);
    edge_matrix_t *KH = edge_matrix_alloc(ekf->n, ekf->n);
    edge_matrix_mul(K, H, KH);
    edge_matrix_sub(I, KH, I);
    edge_matrix_mul(I, ekf->P, KH); // reuse KH as temp
    edge_matrix_copy(ekf->P, KH);

    edge_matrix_free(HT); edge_matrix_free(S); edge_matrix_free(tmpPH);
    edge_matrix_free(S_inv); edge_matrix_free(K); edge_matrix_free(Ky);
    edge_matrix_free(I); edge_matrix_free(KH);
}

int control_mpc_formulate(const edge_matrix_t *A, const edge_matrix_t *B, 
                          const edge_matrix_t *Q, const edge_matrix_t *R,
                          uint32_t N, edge_matrix_t *H_out, edge_matrix_t *f_out) {
    uint32_t n = edge_matrix_rows(A);
    uint32_t m = edge_matrix_cols(B);

    // 1. 构造 Sx (nN x n) 和 Su (nN x mN)
    edge_matrix_t *Sx = edge_matrix_alloc(n * N, n);
    edge_matrix_t *Su = edge_matrix_alloc(n * N, m * N);
    
    edge_matrix_t *Ai = edge_matrix_alloc(n, n);
    edge_matrix_identity(Ai);
    
    for (uint32_t i = 0; i < N; i++) {
        // Ai = A^(i+1)
        edge_matrix_t *next_Ai = edge_matrix_alloc(n, n);
        edge_matrix_mul(Ai, A, next_Ai);
        edge_matrix_copy(Ai, next_Ai);
        edge_matrix_free(next_Ai);
        
        // Fill Sx block
        for(uint32_t r=0; r<n; r++)
            for(uint32_t c=0; c<n; c++) edge_matrix_set(Sx, i*n+r, c, edge_matrix_get(Ai, r, c));
        
        // Fill Su blocks
        for (uint32_t j = 0; j <= i; j++) {
            // Su(i,j) = A^(i-j) * B
            edge_matrix_t *Ap = edge_matrix_alloc(n, n);
            edge_matrix_identity(Ap);
            // 这里为了简单，重新计算 A^(i-j)，实际可优化
            for(uint32_t p=0; p < (i-j); p++) {
                edge_matrix_t *tmp = edge_matrix_alloc(n, n);
                edge_matrix_mul(Ap, A, tmp);
                edge_matrix_copy(Ap, tmp);
                edge_matrix_free(tmp);
            }
            edge_matrix_t *block = edge_matrix_alloc(n, m);
            edge_matrix_mul(Ap, B, block);
            for(uint32_t r=0; r<n; r++)
                for(uint32_t c=0; c<m; c++) edge_matrix_set(Su, i*n+r, j*m+c, edge_matrix_get(block, r, c));
            edge_matrix_free(Ap); edge_matrix_free(block);
        }
    }

    // 2. H = Su' * Qbar * Su + Rbar
    edge_matrix_t *SuT = edge_matrix_alloc(m * N, n * N);
    edge_matrix_transpose(Su, SuT);
    edge_matrix_mul(SuT, Su, H_out); 
    
    // 3. f = (Sx * x0)' * Qbar * Su
    edge_matrix_t *Sx_x0 = edge_matrix_alloc(n * N, 1);
    for(uint32_t i=0; i<n*N; i++) edge_matrix_set(Sx_x0, i, 0, 1.0);
    edge_matrix_t *Sx_x0_T = edge_matrix_alloc(1, n * N);
    edge_matrix_transpose(Sx_x0, Sx_x0_T);
    
    edge_matrix_t *f_row = edge_matrix_alloc(1, m * N);
    edge_matrix_mul(Sx_x0_T, Su, f_row);
    
    // Transpose f_row to f_out (mN x 1)
    edge_matrix_transpose(f_row, f_out);

    edge_matrix_free(Sx); edge_matrix_free(Su); edge_matrix_free(Ai);
    edge_matrix_free(SuT); edge_matrix_free(Sx_x0); edge_matrix_free(Sx_x0_T);
    edge_matrix_free(f_row);
    return 0;
}
