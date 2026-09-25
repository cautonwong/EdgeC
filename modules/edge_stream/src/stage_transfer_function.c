/**
 * @file stage_transfer_function.c
 * @brief 实时传递函数算子 (支持 S 域定义，自动双线性变换)
 */

#include "stream_engine.h"
#include "edge_dsp.h"
#include <stdlib.h>

typedef struct {
    dsp_iir_storage_t filter_store;
    dsp_iir_t *filter;
    double fs;
} transfer_fn_ctx_t;

static stage_ret_t _transfer_fn_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    transfer_fn_ctx_t *ctx = (transfer_fn_ctx_t *)stage->priv_state;
    if (!ctx || !ctx->filter || !ev) return STAGE_ERROR;

    ev->payload.val.f = dsp_iir_process(ctx->filter, ev->payload.val.f);
    return STAGE_OK;
}

/**
 * @brief 算子工厂实现
 * 参数映射: f1=fs, f2=s_num_b0, f3=s_num_b1, u1=s_den_a0 (a1固定为1.0)
 * 或通过更复杂的配置注入。
 */
pipeline_stage_t *stage_transfer_fn_create(const edge_stage_def_t *def) {
    pipeline_stage_t *s = stream_stage_alloc_node();
    transfer_fn_ctx_t *ctx = (transfer_fn_ctx_t *)s->storage_state;
    memset(ctx, 0, sizeof(*ctx));

    double fs = def->params.f1 > 0 ? def->params.f1 : 1000.0;
    double s_num[2] = {def->params.f2, def->params.f3};
    double s_den[2] = {(double)def->params.u1, 1.0}; // a1 默认为 1.0 (标准化)

    double z_num[2], z_den[2];
    dsp_s2z_bilinear(fs, s_num, s_den, z_num, z_den);

    // 创建 1 阶 IIR (由 S 域 [b0, b1]/[a0, 1] 生成)
    ctx->filter = dsp_iir_create_in(&ctx->filter_store, 1, z_num, z_den);
    ctx->fs = fs;

    snprintf(s->name, 31, "TF_S_to_Z");
    s->process = _transfer_fn_proc;
    s->priv_state = ctx;
    s->cleanup_state = NULL;

    return s;
}
