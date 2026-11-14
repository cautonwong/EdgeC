/**
 * @file stage_iir_filter.c
 * @brief 通用 IIR 数字滤波器算子 (直接 II 型实现)
 * 方程: y[n] = b0*x[n] + b1*x[n-1] + ... - a1*y[n-1] - a2*y[n-2]...
 */

#include "stream_engine.h"
#include <stdlib.h>
#include <string.h>

#define IIR_MAX_ORDER 32

typedef struct {
    double b[IIR_MAX_ORDER + 1];      // 分子系数
    double a[IIR_MAX_ORDER + 1];      // 分母系数 (a0 默认为 1.0)
    double x_hist[IIR_MAX_ORDER];     // 输入历史
    double y_hist[IIR_MAX_ORDER];     // 输出历史
    uint32_t order;                   // 阶数
} iir_filter_ctx_t;

_Static_assert(sizeof(iir_filter_ctx_t) <= 2048, "iir ctx exceeds stage state storage");

static stage_ret_t _iir_filter_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    iir_filter_ctx_t *ctx = (iir_filter_ctx_t *)stage->priv_state;
    if (!ctx || !ev) return STAGE_ERROR;

    double xn = ev->payload.val.f;
    double yn = ctx->b[0] * xn;

    // 计算 IIR 差分方程
    for (uint32_t i = 1; i <= ctx->order; i++) {
        yn += ctx->b[i] * ctx->x_hist[i-1];
        yn -= ctx->a[i] * ctx->y_hist[i-1];
    }

    // 更新历史记录
    for (uint32_t i = ctx->order - 1; i > 0; i--) {
        ctx->x_hist[i] = ctx->x_hist[i-1];
        ctx->y_hist[i] = ctx->y_hist[i-1];
    }
    ctx->x_hist[0] = xn;
    ctx->y_hist[0] = yn;

    ev->payload.val.f = yn;
    return STAGE_OK;
}

static void _iir_filter_cleanup(void *priv) {
    (void)priv;
}

/**
 * @brief 创建 IIR 滤波器算子
 * @param order 阶数
 * @param b 分子系数数组 (长度 order + 1)
 * @param a 分母系数数组 (长度 order + 1, a[0] 忽略)
 */
pipeline_stage_t *stage_iir_filter_create(uint32_t order, const double *b, const double *a) {
    if (order == 0 || order > IIR_MAX_ORDER || !b || !a) return NULL;
    pipeline_stage_t *s = stream_stage_alloc_node();
    iir_filter_ctx_t *ctx = (iir_filter_ctx_t *)s->storage_state;
    memset(ctx, 0, sizeof(*ctx));

    ctx->order = order;
    memcpy(ctx->b, b, (order + 1) * sizeof(double));
    memcpy(ctx->a, a, (order + 1) * sizeof(double));

    snprintf(s->name, 31, "IIR_Filter_%u", order);
    s->process = _iir_filter_proc;
    s->priv_state = ctx;
    s->cleanup_state = NULL;

    return s;
}
