/**
 * @file edge_linalg.h
 * @brief 归一化线性代数接口 (支持 GSL/CMSIS-DSP 后端可插拔)
 */

#ifndef EDGE_LINALG_H
#define EDGE_LINALG_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 矩阵对象 (不透明句柄)
 */
typedef struct edge_matrix_s edge_matrix_t;

/**
 * @brief 分配矩阵内存
 */
edge_matrix_t* edge_matrix_alloc(uint32_t rows, uint32_t cols);

/**
 * @brief 释放矩阵
 */
void edge_matrix_free(edge_matrix_t *m);

/**
 * @brief 设置矩阵元素值
 */
int edge_matrix_set(edge_matrix_t *m, uint32_t r, uint32_t c, double val);

/**
 * @brief 获取矩阵元素值
 */
double edge_matrix_get(const edge_matrix_t *m, uint32_t r, uint32_t c);

/**
 * @brief 获取矩阵行数
 */
uint32_t edge_matrix_rows(const edge_matrix_t *m);

/**
 * @brief 获取矩阵列数
 */
uint32_t edge_matrix_cols(const edge_matrix_t *m);

/**
 * @brief 矩阵加法: res = a + b
 */
int edge_matrix_add(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *res);

/**
 * @brief 矩阵减法: res = a - b
 */
int edge_matrix_sub(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *res);

/**
 * @brief 矩阵乘法: res = a * b
 */
int edge_matrix_mul(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *res);

/**
 * @brief 矩阵与向量乘法 (res 必须是列向量)
 */
int edge_matrix_mul_vec(const edge_matrix_t *a, const edge_matrix_t *v, edge_matrix_t *res);

/**
 * @brief 设为单位阵
 */
int edge_matrix_identity(edge_matrix_t *m);

/**
 * @brief 矩阵复制
 */
int edge_matrix_copy(edge_matrix_t *dst, const edge_matrix_t *src);

/**
 * @brief 矩阵求逆: res = inv(m)
 */
int edge_matrix_inv(const edge_matrix_t *m, edge_matrix_t *res);

/**
 * @brief Cholesky 分解: A = L * L^T (A 必须是正定矩阵)
 */
int edge_matrix_cholesky(const edge_matrix_t *a, edge_matrix_t *l);

/**
 * @brief QR 分解: A = Q * R
 */
int edge_matrix_qr(const edge_matrix_t *a, edge_matrix_t *q, edge_matrix_t *r);

/**
 * @brief 奇异值分解 (SVD): A = U * S * V^T
 */
int edge_matrix_svd(const edge_matrix_t *a, edge_matrix_t *u, edge_matrix_t *s, edge_matrix_t *v);

/**
 * @brief 最小二乘法求解: Ax = b
 */
int edge_matrix_solve_ls(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *x);

/**
 * @brief 矩阵转置: res = m^T
 */
int edge_matrix_transpose(const edge_matrix_t *m, edge_matrix_t *res);

/**
 * @brief 计算矩阵行列式
 */
double edge_matrix_det(const edge_matrix_t *m);

/**
 * @brief 计算向量/矩阵范数 (kind: 1:L1, 2:L2, 0:Frobenius)
 */
double edge_matrix_norm(const edge_matrix_t *m, int kind);

/**
 * @brief 向量点积 (a, b 必须是长度相同的向量)
 */
double edge_matrix_dot(const edge_matrix_t *a, const edge_matrix_t *b);

/**
 * @brief 计算特征值 (res_re, res_im 必须有 rows 个空间)
 */
int edge_matrix_eigenvalues(const edge_matrix_t *m, double *res_re, double *res_im);

/**
 * @brief 向量缩放: m = m * scale
 */
int edge_matrix_scale(edge_matrix_t *m, double scale);

/* --- 6. 四元数与姿态 (Quaternion Math) --- */

/**
 * @brief 四元数乘法: res = q1 * q2 (q1, q2 必须是 4x1 矩阵，格式 [w, x, y, z])
 */
int edge_matrix_quat_mul(const edge_matrix_t *q1, const edge_matrix_t *q2, edge_matrix_t *res);

/**
 * @brief 四元数归一化: 确保 q 的模长为 1
 */
int edge_matrix_quat_normalize(edge_matrix_t *q);

/**
 * @brief 四元数转旋转矩阵: q (4x1) -> rot (3x3)
 */
int edge_matrix_quat_to_rot(const edge_matrix_t *q, edge_matrix_t *rot);

#ifdef __cplusplus
}
#endif

#endif // EDGE_LINALG_H
