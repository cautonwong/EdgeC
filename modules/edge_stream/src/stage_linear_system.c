/**
 * @file stage_linear_system.c
 * @brief 线性代数算子：实现状态空间观测器 (x_next = Ax + Bu)
 */

#include "stream_engine.h"
#include "edge_linalg.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    edge_matrix_t *A;
    edge_matrix_t *B;
    edge_matrix_t *x;
    uint32_t output_tag;
} linear_system_ctx_t;

static stage_ret_t _linear_system_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    linear_system_ctx_t *ctx = (linear_system_ctx_t *)stage->priv_state;
    if (!ctx || !ev) return STAGE_ERROR;

    double u_val = ev->payload.val.f;
    uint32_t rows = edge_matrix_rows(ctx->x);
    
    // x = A*x + B*u
    edge_matrix_t *ax = edge_matrix_alloc(rows, 1);
    edge_matrix_t *bu = edge_matrix_alloc(rows, 1);
    
    edge_matrix_mul(ctx->A, ctx->x, ax);
    
    for(uint32_t i=0; i<rows; i++) {
        edge_matrix_set(bu, i, 0, edge_matrix_get(ctx->B, i, 0) * u_val);
    }
    
    edge_matrix_add(ax, bu, ctx->x);
    
    ev->tag_id = ctx->output_tag;
    ev->payload.val.f = edge_matrix_get(ctx->x, 0, 0);
    
    edge_matrix_free(ax);
    edge_matrix_free(bu);
    
    return STAGE_OK;
}

static void _linear_system_cleanup(void *priv) {
    linear_system_ctx_t *ctx = (linear_system_ctx_t *)priv;
    if (ctx) {
        edge_matrix_free(ctx->A);
        edge_matrix_free(ctx->B);
        edge_matrix_free(ctx->x);
        free(ctx);
    }
}

pipeline_stage_t *stage_linear_system_create(uint32_t dim, uint32_t out_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    linear_system_ctx_t *ctx = calloc(1, sizeof(linear_system_ctx_t));
    
    ctx->A = edge_matrix_alloc(dim, dim);
    ctx->B = edge_matrix_alloc(dim, 1);
    ctx->x = edge_matrix_alloc(dim, 1);
    ctx->output_tag = out_tag;
    
    for(uint32_t i=0; i<dim; i++) edge_matrix_set(ctx->A, i, i, 1.0);
    
    snprintf(s->name, 31, "LinearSys_%u", dim);
    s->process = _linear_system_proc;
    s->priv_state = ctx;
    s->cleanup_state = _linear_system_cleanup;
    
    return s;
}
