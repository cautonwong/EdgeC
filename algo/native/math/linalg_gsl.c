/**
 * @file linalg_gsl.c
 * @brief GSL 后端实现的线性代数引擎 (汇编加速，数值极度稳定)
 */

#include "edge_linalg.h"
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_eigen.h>
#include <stdlib.h>
#include <math.h>

struct edge_matrix_s {
    gsl_matrix *g;
};

edge_matrix_t* edge_matrix_alloc(uint32_t rows, uint32_t cols) {
    edge_matrix_t *m = malloc(sizeof(edge_matrix_t));
    m->g = gsl_matrix_calloc(rows, cols);
    return m;
}

void edge_matrix_free(edge_matrix_t *m) {
    if (m) {
        gsl_matrix_free(m->g);
        free(m);
    }
}

int edge_matrix_set(edge_matrix_t *m, uint32_t r, uint32_t c, double val) {
    if (!m || r >= m->g->size1 || c >= m->g->size2) return -1;
    gsl_matrix_set(m->g, r, c, val);
    return 0;
}

double edge_matrix_get(const edge_matrix_t *m, uint32_t r, uint32_t c) {
    if (!m || r >= m->g->size1 || c >= m->g->size2) return 0.0;
    return gsl_matrix_get(m->g, r, c);
}

uint32_t edge_matrix_rows(const edge_matrix_t *m) {
    return m ? (uint32_t)m->g->size1 : 0;
}

uint32_t edge_matrix_cols(const edge_matrix_t *m) {
    return m ? (uint32_t)m->g->size2 : 0;
}

int edge_matrix_add(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *res) {
    gsl_matrix_memcpy(res->g, a->g);
    return gsl_matrix_add(res->g, b->g);
}

int edge_matrix_sub(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *res) {
    gsl_matrix_memcpy(res->g, a->g);
    return gsl_matrix_sub(res->g, b->g);
}

int edge_matrix_mul(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *res) {
    return gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, a->g, b->g, 0.0, res->g);
}

int edge_matrix_mul_vec(const edge_matrix_t *a, const edge_matrix_t *v, edge_matrix_t *res) {
    gsl_vector_view v_view = gsl_matrix_column(v->g, 0);
    gsl_vector_view res_view = gsl_matrix_column(res->g, 0);
    return gsl_blas_dgemv(CblasNoTrans, 1.0, a->g, &v_view.vector, 0.0, &res_view.vector);
}

int edge_matrix_identity(edge_matrix_t *m) {
    gsl_matrix_set_identity(m->g);
    return 0;
}

int edge_matrix_copy(edge_matrix_t *dst, const edge_matrix_t *src) {
    return gsl_matrix_memcpy(dst->g, src->g);
}

int edge_matrix_transpose(const edge_matrix_t *m, edge_matrix_t *res) {
    return gsl_matrix_transpose_memcpy(res->g, m->g);
}

double edge_matrix_det(const edge_matrix_t *m) {
    if (m->g->size1 != m->g->size2) return NAN;
    size_t n = m->g->size1;
    gsl_matrix *tmp = gsl_matrix_alloc(n, n);
    gsl_matrix_memcpy(tmp, m->g);
    gsl_permutation *p = gsl_permutation_alloc(n);
    int signum;
    gsl_linalg_LU_decomp(tmp, p, &signum);
    double det = gsl_linalg_LU_det(tmp, signum);
    gsl_permutation_free(p);
    gsl_matrix_free(tmp);
    return det;
}

double edge_matrix_norm(const edge_matrix_t *m, int kind) {
    if (kind == 0) { // Frobenius
        double sum = 0;
        for(size_t i=0; i<m->g->size1; i++) {
            for(size_t j=0; j<m->g->size2; j++) {
                double val = gsl_matrix_get(m->g, i, j);
                sum += val * val;
            }
        }
        return sqrt(sum);
    }
    gsl_vector_view v = gsl_matrix_column(m->g, 0);
    if (kind == 1) return gsl_blas_dasum(&v.vector);
    if (kind == 2) return gsl_blas_dnrm2(&v.vector);
    return 0.0;
}

double edge_matrix_dot(const edge_matrix_t *a, const edge_matrix_t *b) {
    gsl_vector_view va = gsl_matrix_column(a->g, 0);
    gsl_vector_view vb = gsl_matrix_column(b->g, 0);
    double res;
    gsl_blas_ddot(&va.vector, &vb.vector, &res);
    return res;
}

int edge_matrix_scale(edge_matrix_t *m, double scale) {
    return gsl_matrix_scale(m->g, scale);
}

int edge_matrix_inv(const edge_matrix_t *m, edge_matrix_t *res) {
    if (m->g->size1 != m->g->size2) return -1;
    size_t n = m->g->size1;
    gsl_matrix *tmp = gsl_matrix_alloc(n, n);
    gsl_matrix_memcpy(tmp, m->g);
    gsl_permutation *p = gsl_permutation_alloc(n);
    int signum;
    gsl_linalg_LU_decomp(tmp, p, &signum);
    int ret = gsl_linalg_LU_invert(tmp, p, res->g);
    gsl_permutation_free(p);
    gsl_matrix_free(tmp);
    return ret;
}

int edge_matrix_qr(const edge_matrix_t *a, edge_matrix_t *q, edge_matrix_t *r) {
    size_t m = a->g->size1;
    size_t n = a->g->size2;
    gsl_matrix *tmp = gsl_matrix_alloc(m, n);
    gsl_matrix_memcpy(tmp, a->g);
    gsl_vector *tau = gsl_vector_alloc(n < m ? n : m);
    gsl_linalg_QR_decomp(tmp, tau);
    gsl_linalg_QR_unpack(tmp, tau, q->g, r->g);
    gsl_vector_free(tau);
    gsl_matrix_free(tmp);
    return 0;
}

int edge_matrix_svd(const edge_matrix_t *a, edge_matrix_t *u, edge_matrix_t *s, edge_matrix_t *v) {
    size_t m = a->g->size1;
    size_t n = a->g->size2;
    // GSL computes thin SVD in-place: A(MxN) -> U(MxN), S(Nx1), V(NxN)
    // u must be MxN
    if (u->g->size1 != m || u->g->size2 != n) return -1;
    gsl_matrix_memcpy(u->g, a->g);
    gsl_vector *s_vec = gsl_vector_alloc(n);
    gsl_vector *work = gsl_vector_alloc(n);
    int ret = gsl_linalg_SV_decomp(u->g, v->g, s_vec, work);
    
    // Fill diagonal matrix S (NxN)
    gsl_matrix_set_zero(s->g);
    for(size_t i=0; i<n; i++) gsl_matrix_set(s->g, (uint32_t)i, (uint32_t)i, gsl_vector_get(s_vec, i));
    
    gsl_vector_free(s_vec);
    gsl_vector_free(work);
    return ret;
}

int edge_matrix_solve_ls(const edge_matrix_t *a, const edge_matrix_t *b, edge_matrix_t *x) {
    size_t m = a->g->size1;
    size_t n = a->g->size2;
    gsl_vector_view b_vec = gsl_matrix_column(b->g, 0);
    gsl_vector_view x_vec = gsl_matrix_column(x->g, 0);
    gsl_matrix *tmp_a = gsl_matrix_alloc(m, n);
    gsl_matrix_memcpy(tmp_a, a->g);
    gsl_vector *tau = gsl_vector_alloc(n < m ? n : m);
    gsl_vector *res = gsl_vector_alloc(m);
    gsl_linalg_QR_decomp(tmp_a, tau);
    int ret = gsl_linalg_QR_lssolve(tmp_a, tau, &b_vec.vector, &x_vec.vector, res);
    gsl_vector_free(tau);
    gsl_vector_free(res);
    gsl_matrix_free(tmp_a);
    return ret;
}

int edge_matrix_eigenvalues(const edge_matrix_t *m, double *res_re, double *res_im) {
    if (m->g->size1 != m->g->size2) return -1;
    size_t n = m->g->size1;
    gsl_matrix *tmp = gsl_matrix_alloc(n, n);
    gsl_matrix_memcpy(tmp, m->g);
    gsl_vector_complex *eval = gsl_vector_complex_alloc(n);
    gsl_eigen_nonsymm_workspace *w = gsl_eigen_nonsymm_alloc(n);
    gsl_eigen_nonsymm(tmp, eval, w);
    for (size_t i = 0; i < n; i++) {
        gsl_complex z = gsl_vector_complex_get(eval, i);
        res_re[i] = GSL_REAL(z);
        res_im[i] = GSL_IMAG(z);
    }
    gsl_eigen_nonsymm_free(w);
    gsl_vector_complex_free(eval);
    gsl_matrix_free(tmp);
    return 0;
}
