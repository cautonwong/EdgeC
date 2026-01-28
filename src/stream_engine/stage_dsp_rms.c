#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "edge_log.h"
#include "stream_engine.h"
#include "stream_engine_priv.h"

typedef struct {
    uint32_t output_tag_id;
    double last_rms;
} dsp_rms_ctx_t;

/**
 * @brief RMS (均方根) 算法算子
 * 处理 PIPEMSG_TYPE_REF 类型的高频波形数据
 */
static stage_ret_t _dsp_rms_proc(pipeline_stage_t *s, pipe_msg_t *msg) {
    dsp_rms_ctx_t *ctx = (dsp_rms_ctx_t *)s->priv_state;

    if (msg->type != PIPEMSG_TYPE_REF) {
        return STAGE_OK;  // 忽略非引用类型数据
    }

    printf("\033[33m[DSP-DEBUG] RMS Proc Triggered for Tag %u\033[0m\n", msg->tag_id);

    float *samples = (float *)msg->payload.ref.ptr;
    size_t byte_count = msg->payload.ref.len;
    size_t count = byte_count / sizeof(float);

    if (!samples || count == 0) return STAGE_DROP;

    // 算法核心：RMS = sqrt(sum(x^2) / N)
    double sum_sq = 0.0;
    for (size_t i = 0; i < count; i++) {
        float val = samples[i];
        sum_sq += (double)val * val;
    }

    double rms = sqrt(sum_sq / (double)count);
    ctx->last_rms = rms;

    // 将计算结果封装为新的消息，写回 DataHub 业务点位
    pipe_msg_t out_msg;
    pipe_msg_init_scalar(&out_msg, ctx->output_tag_id, rms, msg->ts);

    // [Trace] 高频算法日志（采样输出）
    static uint64_t counter = 0;
    if (++counter % 50 == 0) {
        printf("\033[36m[DSP-RMS] Computed RMS from %zu samples: %.2f V\033[0m\n", count, rms);
    }

    datahub_publish(s->pipeline->engine->hub, &out_msg);

    return STAGE_OK;
}

static void _dsp_rms_cleanup(void *p) {
    free(p);
}

pipeline_stage_t *stage_dsp_rms_create(uint32_t output_tag_id) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "dsp_rms", 31);
    s->process = _dsp_rms_proc;

    dsp_rms_ctx_t *ctx = calloc(1, sizeof(dsp_rms_ctx_t));
    ctx->output_tag_id = output_tag_id;

    s->priv_state = ctx;
    s->cleanup_state = _dsp_rms_cleanup;
    return s;
}

pipeline_stage_t *stage_dsp_rms_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    return stage_dsp_rms_create((uint32_t)def->params.u1);
}
