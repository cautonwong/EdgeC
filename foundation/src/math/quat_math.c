/**
 * @file quat_math.c
 * @brief 航空航天级四元数与姿态解算数学库
 */

#include "edge_linalg.h"
#include <math.h>

/**
 * @brief 四元数乘法: res = q1 * q2 
 * q = [w, x, y, z]
 */
int edge_matrix_quat_mul(const edge_matrix_t *q1, const edge_matrix_t *q2, edge_matrix_t *res) {
    if (!q1 || !q2 || !res) return -1;
    
    double w1 = edge_matrix_get(q1, 0, 0);
    double x1 = edge_matrix_get(q1, 1, 0);
    double y1 = edge_matrix_get(q1, 2, 0);
    double z1 = edge_matrix_get(q1, 3, 0);
    
    double w2 = edge_matrix_get(q2, 0, 0);
    double x2 = edge_matrix_get(q2, 1, 0);
    double y2 = edge_matrix_get(q2, 2, 0);
    double z2 = edge_matrix_get(q2, 3, 0);
    
    edge_matrix_set(res, 0, 0, w1*w2 - x1*x2 - y1*y2 - z1*z2);
    edge_matrix_set(res, 1, 0, w1*x2 + x1*w2 + y1*z2 - z1*y2);
    edge_matrix_set(res, 2, 0, w1*y2 - x1*z2 + y1*w2 + z1*x2);
    edge_matrix_set(res, 3, 0, w1*z2 + x1*y2 - y1*x2 + z1*w2);
    
    return 0;
}

/**
 * @brief 四元数归一化
 */
int edge_matrix_quat_normalize(edge_matrix_t *q) {
    if (!q) return -1;
    
    double w = edge_matrix_get(q, 0, 0);
    double x = edge_matrix_get(q, 1, 0);
    double y = edge_matrix_get(q, 2, 0);
    double z = edge_matrix_get(q, 3, 0);
    
    double norm = sqrt(w*w + x*x + y*y + z*z);
    if (norm < 1e-9) return -1;
    
    edge_matrix_set(q, 0, 0, w / norm);
    edge_matrix_set(q, 1, 0, x / norm);
    edge_matrix_set(q, 2, 0, y / norm);
    edge_matrix_set(q, 3, 0, z / norm);
    
    return 0;
}

/**
 * @brief 四元数转旋转矩阵 (3x3)
 */
int edge_matrix_quat_to_rot(const edge_matrix_t *q, edge_matrix_t *rot) {
    if (!q || !rot) return -1;
    
    double w = edge_matrix_get(q, 0, 0);
    double x = edge_matrix_get(q, 1, 0);
    double y = edge_matrix_get(q, 2, 0);
    double z = edge_matrix_get(q, 3, 0);
    
    edge_matrix_set(rot, 0, 0, 1.0 - 2.0*y*y - 2.0*z*z);
    edge_matrix_set(rot, 0, 1, 2.0*x*y - 2.0*z*w);
    edge_matrix_set(rot, 0, 2, 2.0*x*z + 2.0*y*w);
    
    edge_matrix_set(rot, 1, 0, 2.0*x*y + 2.0*z*w);
    edge_matrix_set(rot, 1, 1, 1.0 - 2.0*x*x - 2.0*z*z);
    edge_matrix_set(rot, 1, 2, 2.0*y*z - 2.0*x*w);
    
    edge_matrix_set(rot, 2, 0, 2.0*x*z - 2.0*y*w);
    edge_matrix_set(rot, 2, 1, 2.0*y*z + 2.0*x*w);
    edge_matrix_set(rot, 2, 2, 1.0 - 2.0*x*x - 2.0*y*y);
    
    return 0;
}
