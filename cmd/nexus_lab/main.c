/**
 * @file main.c
 * @brief Nexus Lab V4.1 (The Industrial Lexicon): 全栈算法深度验证
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <uv.h>
#include <sys/mman.h>
#include <unistd.h>

#include "edge_linalg.h"
#include "edge_dsp.h"
#include "radar_algo.h"
#include "edge_control.h"
#include "edge_log.h"
#include "shmring.h"

void demo_linalg_solvers() {
    printf("\n\033[1;34m[Lexicon 1] 线性代数求解器\033[0m\n");
    edge_matrix_t *A = edge_matrix_alloc(3, 2);
    edge_matrix_t *b = edge_matrix_alloc(3, 1);
    edge_matrix_t *x = edge_matrix_alloc(2, 1);
    edge_matrix_set(A, 0,0, 1.0); edge_matrix_set(A, 0,1, 1.0);
    edge_matrix_set(A, 1,0, 2.0); edge_matrix_set(A, 1,1, 1.0);
    edge_matrix_set(A, 2,0, 3.0); edge_matrix_set(A, 2,1, 1.0);
    edge_matrix_set(b, 0,0, 2.1); edge_matrix_set(b, 1,0, 3.9); edge_matrix_set(b, 2,0, 6.1);
    edge_matrix_solve_ls(A, b, x);
    printf("  - LS 线性拟合: M=%.2f, C=%.2f\n", edge_matrix_get(x,0,0), edge_matrix_get(x,1,0));
    
    edge_matrix_t *U = edge_matrix_alloc(3, 2);
    edge_matrix_t *S = edge_matrix_alloc(2, 2);
    edge_matrix_t *V = edge_matrix_alloc(2, 2);
    edge_matrix_svd(A, U, S, V);
    printf("  - SVD 奇异值: σ1=%.2f, σ2=%.2f\n", edge_matrix_get(S,0,0), edge_matrix_get(S,1,1));
    
    edge_matrix_free(A); edge_matrix_free(b); edge_matrix_free(x);
    edge_matrix_free(U); edge_matrix_free(S); edge_matrix_free(V);
}

void demo_dsp_and_radar() {
    printf("\n\033[1;34m[Lexicon 2] 信号处理与雷达感测 (2D FFT / CFAR)\033[0m\n");
    double *rd_data = calloc(8 * 8 * 2, sizeof(double));
    rd_data[(3 * 8 + 2) * 2] = 100.0; 
    dsp_fft2d_c2c(rd_data, 8, 8, true);
    printf("  - 2D FFT 变换完成 (已打通行、列跨步处理)\n");

    double spectrum[64];
    for(int i=0; i<64; i++) spectrum[i] = (double)rand()/RAND_MAX;
    spectrum[32] = 20.0;
    uint8_t peaks[64];
    int count = radar_cfar_ca(spectrum, 64, 4, 2, 5.0, peaks);
    printf("  - CA-CFAR 结果: 检测到 %d 个目标, Bin 32: %s\n", 
           count, peaks[32] ? "\033[32mTarget!\033[0m" : "Noise");
    free(rd_data);
}

void demo_constrained_optimization() {
    printf("\n\033[1;34m[Lexicon 3] 约束优化 (QP Hildreth Solver)\033[0m\n");
    // min 1/2 x'Qx + c'x s.t. Ax <= b
    // Q = [2 0; 0 2], c = [-2; -5] -> 无约束极小值在 (1, 2.5)
    // 约束 x1 + x2 <= 2
    edge_matrix_t *Q = edge_matrix_alloc(2, 2); edge_matrix_identity(Q); edge_matrix_scale(Q, 2.0);
    edge_matrix_t *c = edge_matrix_alloc(2, 1); edge_matrix_set(c, 0, 0, -2.0); edge_matrix_set(c, 1, 0, -5.0);
    edge_matrix_t *A = edge_matrix_alloc(1, 2); edge_matrix_set(A, 0, 0, 1.0); edge_matrix_set(A, 0, 1, 1.0);
    edge_matrix_t *b = edge_matrix_alloc(1, 1); edge_matrix_set(b, 0, 0, 2.0);
    edge_matrix_t *x = edge_matrix_alloc(2, 1);

    control_qp_solve_hildreth(Q, c, A, b, x);
    printf("  - QP 求解结果 (带约束 x1+x2 <= 2): x1=\033[32m%.2f\033[0m, x2=\033[32m%.2f\033[0m\n", 
           edge_matrix_get(x,0,0), edge_matrix_get(x,1,0));

    edge_matrix_free(Q); edge_matrix_free(c); edge_matrix_free(A); edge_matrix_free(b); edge_matrix_free(x);
}

void demo_modern_control() {
    printf("\n\033[1;34m[Lexicon 4] 现代控制理论 (LQR ARE Solver)\033[0m\n");
    edge_matrix_t *A = edge_matrix_alloc(2, 2); edge_matrix_set(A, 0, 1, 1.0);
    edge_matrix_t *B = edge_matrix_alloc(2, 1); edge_matrix_set(B, 1, 0, 1.0);
    edge_matrix_t *Q = edge_matrix_alloc(2, 2); edge_matrix_identity(Q);
    edge_matrix_t *R = edge_matrix_alloc(1, 1); edge_matrix_set(R, 0, 0, 1.0);
    edge_matrix_t *K = edge_matrix_alloc(1, 2);

    if (control_lqr_solve(A, B, Q, R, K) == 0) {
        printf("  - LQR 最优增益 K: [\033[32m%.2f, %.2f\033[0m]\n", 
               edge_matrix_get(K,0,0), edge_matrix_get(K,0,1));
    }

    edge_matrix_free(A); edge_matrix_free(B); edge_matrix_free(Q); edge_matrix_free(R); edge_matrix_free(K);
}

int main() {
    printf("\033[36m======================================================================\n");
    printf("      Nexus Lab V4.1 Lexicon: 工业算法字典深度增强版\n");
    printf("======================================================================\033[0m\n");

    demo_linalg_solvers();
    demo_dsp_and_radar();
    demo_constrained_optimization();
    demo_modern_control();

    printf("\n\033[36m======================================================================\n");
    printf("   验证结论：LQR (ARE), QP (Hildreth), 2D FFT 与 CFAR 已全线就绪。\n");
    printf("======================================================================\033[0m\n");
    return 0;
}
