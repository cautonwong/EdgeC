/**
 * @file motor_algo.c
 * @brief SVPWM 核心算法实现
 */

#include "edge_motor.h"
#include <math.h>

void motor_svpwm(const coord_alphabeta_t *v_in, double v_dc, double *out_abc) {
    if (v_dc <= 0.0) return;

    // 1. 归一化输入电压
    double v_alpha = v_in->alpha;
    double v_beta = v_in->beta;

    // 2. 扇区判断 (Sector Identification)
    // 根据 Ualpha, Ubeta 确定电压矢量所在扇区
    // 采用 U1, U2, U3 中间变量法
    double u1 = v_beta;
    double u2 = (sqrt(3.0) * v_alpha - v_beta) / 2.0;
    double u3 = (-sqrt(3.0) * v_alpha - v_beta) / 2.0;

    int sector = 0;
    if (u1 > 0) sector += 1;
    if (u2 > 0) sector += 2;
    if (u3 > 0) sector += 4;

    // 3. 计算矢量作用时间 T1, T2
    // 这里的 T1, T2 为相对于周期的百分比 (0.0~1.0)
    double ka = sqrt(3.0) / v_dc;
    double t1 = 0, t2 = 0;

    switch (sector) {
        case 3: // Sector 1
            t1 = ka * u2;
            t2 = ka * u1;
            break;
        case 1: // Sector 2
            t1 = -ka * u2;
            t2 = -ka * u3;
            break;
        case 5: // Sector 3
            t1 = ka * u1;
            t2 = ka * u3;
            break;
        case 4: // Sector 4
            t1 = -ka * u1;
            t2 = -ka * u2;
            break;
        case 6: // Sector 5
            t1 = ka * u3;
            t2 = ka * u2;
            break;
        case 2: // Sector 6
            t1 = -ka * u3;
            t2 = -ka * u1;
            break;
        default:
            t1 = 0; t2 = 0;
            break;
    }

    // 4. 饱和处理 (Over-modulation)
    double sum = t1 + t2;
    if (sum > 1.0) {
        t1 /= sum;
        t2 /= sum;
    }

    // 5. 计算三相占空比 (Seven-segment PWM)
    double t0 = (1.0 - t1 - t2) / 4.0;
    double ta, tb, tc;

    switch (sector) {
        case 3: ta = t0; tb = ta + t1/2.0; tc = tb + t2/2.0; break; // Sector 1
        case 1: tb = t0; ta = tb + t2/2.0; tc = ta + t1/2.0; break; // Sector 2
        case 5: tb = t0; tc = tb + t1/2.0; ta = tc + t2/2.0; break; // Sector 3
        case 4: tc = t0; tb = tc + t2/2.0; ta = tb + t1/2.0; break; // Sector 4
        case 6: tc = t0; ta = tc + t1/2.0; tb = ta + t2/2.0; break; // Sector 5
        case 2: ta = t0; tc = ta + t2/2.0; tb = tc + t1/2.0; break; // Sector 6
        default: ta = 0.5; tb = 0.5; tc = 0.5; break;
    }
    
    // 映射到三相比较值 (中点对称，转换为占空比 [0, 1])
    out_abc[0] = ta * 2.0;
    out_abc[1] = tb * 2.0;
    out_abc[2] = tc * 2.0;
}
