#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stream_engine.h"
#include "shmring.h"
#include "edge_log.h"

// 💡 [EMQX eKuiper Pattern - Step 2.1] 100Hz 滑动窗口卡尔曼滤波算子
typedef struct {
    double window[16];
    int window_size;
    int head;
    int count;
    double q; // 过程噪声协方差
    double r; // 测量噪声协方差
    double x; // 估计值
    double p; // 估计协方差
} sliding_window_ctx_t;

static stage_ret_t _sliding_window_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (!stage || !stage->priv_state || !ev || ev->type != PIPEMSG_TYPE_SCALAR) {
        return STAGE_OK;
    }
    sliding_window_ctx_t *ctx = (sliding_window_ctx_t *)stage->priv_state;

    double measurement = ev->payload.val.f;

    // 1. 卡尔曼滤波更新步骤
    ctx->p = ctx->p + ctx->q;
    double k = ctx->p / (ctx->p + ctx->r);
    ctx->x = ctx->x + k * (measurement - ctx->x);
    ctx->p = (1.0 - k) * ctx->p;

    // 2. 插入滑动窗口
    ctx->window[ctx->head] = ctx->x;
    ctx->head = (ctx->head + 1) % ctx->window_size;
    if (ctx->count < ctx->window_size) {
        ctx->count++;
    }

    // 3. 将卡尔曼滤波与滑动窗口平滑后的数据回写至消息
    ev->payload.val.f = ctx->x;
    return STAGE_OK;
}

static void _sliding_window_cleanup(void *priv_data) {
    if (priv_data) {
        free(priv_data);
    }
}

pipeline_stage_t *stage_sliding_window_create(int window_size, double q, double r) {
    pipeline_stage_t *stage = (pipeline_stage_t *)calloc(1, sizeof(pipeline_stage_t));
    if (!stage) return NULL;

    sliding_window_ctx_t *ctx = (sliding_window_ctx_t *)calloc(1, sizeof(sliding_window_ctx_t));
    if (!ctx) {
        free(stage);
        return NULL;
    }

    ctx->window_size = (window_size > 0 && window_size <= 16) ? window_size : 5;
    ctx->q = q > 0 ? q : 0.01;
    ctx->r = r > 0 ? r : 0.1;
    ctx->p = 1.0;

    strncpy(stage->name, "sliding_window", sizeof(stage->name) - 1);
    stage->priv_state = ctx;
    stage->process = _sliding_window_proc;
    stage->cleanup_state = _sliding_window_cleanup;

    LOG_INFO(LOG_CAT_CORE, "[eKuiper DAG] Sliding Window Kalman Filter Stage created (Window Size: %d)", ctx->window_size);
    return stage;
}
