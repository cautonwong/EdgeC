/**
 * @file linalg_builtin.c
 * @brief 内置纯 C 实现的线性代数引擎 (无外部依赖，数值稳定)
 */

#include "edge_linalg.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct edge_matrix_s {
    uint32_t rows;
    uint32_t cols;
    double *data;
};

edge_matrix_t* edge_matrix_alloc(uint32_t rows, uint32_t cols) {
    edge_matrix_t *m = malloc(sizeof(edge_matrix_t));
    m->rows = rows;
    m->cols = cols;
    m->data = calloc(rows * cols, sizeof(double));
    return m;
}

void edge_matrix_free(edge_matrix_t *m) {
    if (m) {
        free(m->data);
        free(m);
    }
}

int edge_matrix_set(edge_matrix_t *m, uint32_t r, uint32_t c, double val) {
    if (!m || r >= m->rows || c >= m->cols) return -1;
    m->data[r * m->cols + c] = val;
    return 0;
}

double edge_matrix_get(const edge_matrix_t *m, uint32_t r, uint32_t c) {
    if (!m || r >= m->rows || c >= m->cols) return NAN;
    return m->data[r * m->cols + c];
}

uint32_t edge_matrix_rows(const edge_matrix_t *m) {
    return m ? m->rows : 0;
}

uint32_t edge_matrix_cols(const edge_matrix_t *m) {
    return m ? m->cols : 0;
}

int edge_matrix_add(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *res) {
    if (a->rows != b->rows || a->cols != b->cols || a->rows != res->rows || a->cols != res->cols)
        return -1;
    for (uint32_t i = 0; i < a->rows * a->cols; i++) {
        res->data[i] = a->data[i] + b->data[i];
    }
    return 0;
}

int edge_matrix_sub(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *res) {
    if (a->rows != b->rows || a->cols != b->cols || a->rows != res->rows || a->cols != res->cols)
        return -1;
    for (uint32_t i = 0; i < a->rows * a->cols; i++) {
        res->data[i] = a->data[i] - b->data[i];
    }
    return 0;
}

int edge_matrix_mul(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *res) {
    if (a->cols != b->rows || a->rows != res->rows || b->cols != res->cols)
        return -1;
    for (uint32_t i = 0; i < a->rows; i++) {
        for (uint32_t j = 0; j < b->cols; j++) {
            double sum = 0;
            for (uint32_t k = 0; k < a->cols; k++) {
                sum += a->data[i * a->cols + k] * b->data[k * b->cols + j];
            }
            res->data[i * res->cols + j] = sum;
        }
    }
    return 0;
}

int edge_matrix_mul_vec(const edge_matrix_t *a, const edge_matrix_t *v, edge_matrix_t *res) {
    return edge_matrix_mul(a, v, res);
}

int edge_matrix_identity(edge_matrix_t *m) {
    if (!m || m->rows != m->cols) return -1;
    memset(m->data, 0, m->rows * m->cols * sizeof(double));
    for (uint32_t i = 0; i < m->rows; i++) {
        m->data[i * m->cols + i] = 1.0;
    }
    return 0;
}

int edge_matrix_copy(edge_matrix_t *dst, const edge_matrix_t *src) {
    if (!dst || !src || dst->rows != src->rows || dst->cols != src->cols) return -1;
    memcpy(dst->data, src->data, src->rows * src->cols * sizeof(double));
    return 0;
}

int edge_matrix_transpose(const edge_matrix_t *m, edge_matrix_t *res) {
    if (m->rows != res->cols || m->cols != res->rows) return -1;
    for (uint32_t i = 0; i < m->rows; i++) {
        for (uint32_t j = 0; j < m->cols; j++) {
            res->data[j * res->cols + i] = m->data[i * m->cols + j];
        }
    }
    return 0;
}

int edge_matrix_inv(const edge_matrix_t *m, edge_matrix_t *res) {
    if (m->rows != m->cols || m->rows != res->rows || m->cols != res->cols) return -1;
    uint32_t n = m->rows;
    double *a = malloc(n * n * sizeof(double));
    memcpy(a, m->data, n * n * sizeof(double));
    edge_matrix_identity(res);
    for (uint32_t i = 0; i < n; i++) {
        uint32_t pivot = i;
        for (uint32_t j = i + 1; j < n; j++) {
            if (fabs(a[j * n + i]) > fabs(a[pivot * n + i])) pivot = j;
        }
        if (pivot != i) {
            for (uint32_t j = 0; j < n; j++) {
                double tmp = a[i * n + j]; a[i * n + j] = a[pivot * n + j]; a[pivot * n + j] = tmp;
                tmp = res->data[i * n + j]; res->data[i * n + j] = res->data[pivot * n + j]; res->data[pivot * n + j] = tmp;
            }
        }
        double div = a[i * n + i];
        if (fabs(div) < 1e-12) { free(a); return -2; }
        for (uint32_t j = 0; j < n; j++) {
            a[i * n + j] /= div; res->data[i * n + j] /= div;
        }
        for (uint32_t k = 0; k < n; k++) {
            if (k != i) {
                double factor = a[k * n + i];
                for (uint32_t j = 0; j < n; j++) {
                    a[k * n + j] -= factor * a[i * n + j];
                    res->data[k * n + j] -= factor * res->data[i * n + j];
                }
            }
        }
    }
    free(a);
    return 0;
}

/**
 * @brief QR 分解 (使用 Gram-Schmidt 过程，工业界通常首选 Householder，此处 GS 较简洁)
 */
int edge_matrix_qr(const edge_matrix_t *a, edge_matrix_t *q, edge_matrix_t *r) {
    uint32_t m = a->rows;
    uint32_t n = a->cols;
    if (q->rows != m || q->cols != n || r->rows != n || r->cols != n) return -1;

    edge_matrix_identity(q); // Q starts as zero or identity
    memset(r->data, 0, n * n * sizeof(double));

    for (uint32_t j = 0; j < n; j++) {
        // v_j = a_j
        for (uint32_t i = 0; i < m; i++) edge_matrix_set(q, i, j, edge_matrix_get(a, i, j));

        for (uint32_t i = 0; i < j; i++) {
            // r_ij = q_i^T * a_j
            double dot = 0;
            for (uint32_t k = 0; k < m; k++) dot += edge_matrix_get(q, k, i) * edge_matrix_get(a, k, j);
            edge_matrix_set(r, i, j, dot);
            // v_j = v_j - r_ij * q_i
            for (uint32_t k = 0; k < m; k++) {
                double val = edge_matrix_get(q, k, j) - dot * edge_matrix_get(q, k, i);
                edge_matrix_set(q, k, j, val);
            }
        }
        // r_jj = ||v_j||
        double norm = 0;
        for (uint32_t k = 0; k < m; k++) norm += pow(edge_matrix_get(q, k, j), 2);
        norm = sqrt(norm);
        edge_matrix_set(r, j, j, norm);
        // q_j = v_j / r_jj
        if (norm > 1e-12) {
            for (uint32_t k = 0; k < m; k++) edge_matrix_set(q, k, j, edge_matrix_get(q, k, j) / norm);
        }
    }
    return 0;
}

int edge_matrix_solve_ls(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *x) {
    // Ax = b -> QRx = b -> Rx = Q^T b
    uint32_t m = a->rows;
    uint32_t n = a->cols;
    edge_matrix_t *q = edge_matrix_alloc(m, n);
    edge_matrix_t *r = edge_matrix_alloc(n, n);
    edge_matrix_qr(a, q, r);

    edge_matrix_t *qt = edge_matrix_alloc(n, m);
    edge_matrix_transpose(q, qt);
    edge_matrix_t *qtb = edge_matrix_alloc(n, 1);
    edge_matrix_mul(qt, b, qtb);

    // 回代法解 Rx = qtb (R 是上三角矩阵)
    for (int i = (int)n - 1; i >= 0; i--) {
        double sum = 0;
        for (uint32_t j = (uint32_t)i + 1; j < n; j++) {
            sum += edge_matrix_get(r, (uint32_t)i, j) * edge_matrix_get(x, j, 0);
        }
        double val = (edge_matrix_get(qtb, (uint32_t)i, 0) - sum) / edge_matrix_get(r, (uint32_t)i, (uint32_t)i);
        edge_matrix_set(x, (uint32_t)i, 0, val);
    }

    edge_matrix_free(q); edge_matrix_free(r); edge_matrix_free(qt); edge_matrix_free(qtb);
    return 0;
}

int edge_matrix_svd(const edge_matrix_t *a, edge_matrix_t *u, edge_matrix_t *s, edge_matrix_t *v) {
    // 简易 Jacobi SVD 实现 (针对小型矩阵)
    uint32_t m = a->rows;
    uint32_t n = a->cols;
    edge_matrix_copy(u, a);
    edge_matrix_identity(v);
    
    for (int iter = 0; iter < 30; iter++) {
        for (uint32_t i = 0; i < n; i++) {
            for (uint32_t j = i + 1; j < n; j++) {
                double aii = 0, ajj = 0, aij = 0;
                for (uint32_t k = 0; k < m; k++) {
                    aii += pow(edge_matrix_get(u, k, i), 2);
                    ajj += pow(edge_matrix_get(u, k, j), 2);
                    aij += edge_matrix_get(u, k, i) * edge_matrix_get(u, k, j);
                }
                
                if (fabs(aij) < 1e-15) continue;

                double tau = (ajj - aii) / (2.0 * aij);
                double t = 1.0 / (fabs(tau) + sqrt(1.0 + tau * tau));
                if (tau < 0) t = -t;
                double c = 1.0 / sqrt(1.0 + t * t);
                double s_rot = c * t;

                for (uint32_t k = 0; k < m; k++) {
                    double uki = edge_matrix_get(u, k, i);
                    double ukj = edge_matrix_get(u, k, j);
                    edge_matrix_set(u, k, i, c * uki - s_rot * ukj);
                    edge_matrix_set(u, k, j, s_rot * uki + c * ukj);
                }
                for (uint32_t k = 0; k < n; k++) {
                    double vki = edge_matrix_get(v, k, i);
                    double vkj = edge_matrix_get(v, k, j);
                    edge_matrix_set(v, k, i, c * vki - s_rot * vkj);
                    edge_matrix_set(v, k, j, s_rot * vki + c * vkj);
                }
            }
        }
    }

    edge_matrix_identity(s);
    for (uint32_t i = 0; i < n; i++) {
        double norm = 0;
        for (uint32_t k = 0; k < m; k++) norm += pow(edge_matrix_get(u, k, i), 2);
        norm = sqrt(norm);
        edge_matrix_set(s, i, i, norm);
        if (norm > 1e-12) {
            for (uint32_t k = 0; k < m; k++) edge_matrix_set(u, k, i, edge_matrix_get(u, k, i) / norm);
        }
    }
    return 0;
}

int edge_matrix_scale(edge_matrix_t *m, double scale) {
    if (!m) return -1;
    for (uint32_t i = 0; i < m->rows * m->cols; i++) {
        m->data[i] *= scale;
    }
    return 0;
}
