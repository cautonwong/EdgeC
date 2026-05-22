/**
 * @file stage_anomaly_baseline.c
 * @brief 工业级统计基准异常检测算子 (Moving Baseline)
 */

#include "stream_engine.h"
#include "stream_engine_priv.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    uint32_t target_tag;
    double mean;
    double var;
    uint32_t count;
    double threshold_sigma;
    uint32_t alarm_tag;
    bool is_learning;
    uint32_t learning_samples;
} baseline_ctx_t;

static stage_ret_t _proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    baseline_ctx_t *ctx = (baseline_ctx_t *)stage->priv_state;
    
    if (ev->type == PIPEMSG_TYPE_SCALAR && ev->tag_id == ctx->target_tag) {
        double val = ev->payload.val.f;
        
        if (ctx->is_learning) {
            // Welford's algorithm for online mean/variance
            ctx->count++;
            double delta = val - ctx->mean;
            ctx->mean += delta / ctx->count;
            double delta2 = val - ctx->mean;
            ctx->var += delta * delta2;
            
            if (ctx->count >= ctx->learning_samples) {
                ctx->is_learning = false;
                ctx->var /= (ctx->count - 1);
                LOG_INFO(LOG_CAT_CORE, "Baseline Learnt for Tag %u: Mean=%.2f, Std=%.2f", 
                         ctx->target_tag, ctx->mean, sqrt(ctx->var));
            }
            return STAGE_DROP; // Still learning
        } else {
            // Inference: Check for Z-score anomaly
            double std = sqrt(ctx->var);
            if (std > 1e-9) {
                double z = fabs(val - ctx->mean) / std;
                if (z > ctx->threshold_sigma) {
                    LOG_WARN(LOG_CAT_CORE, "[SECURITY] Baseline Anomaly on Tag %u: Val=%.2f (Z=%.2f)", 
                             ctx->target_tag, val, z);
                    
                    pipe_msg_t alarm;
                    pipe_msg_init_event(&alarm, 0xE003, (uint32_t)(z * 100), ev->ts);
                    alarm.tag_id = ctx->alarm_tag;
                    stream_engine_push(stage->pipeline->engine, &alarm);
                }
            }
            return STAGE_OK;
        }
    }
    
    return STAGE_DROP;
}

static void _cleanup(void *priv) {
    if (priv) free(priv);
}

pipeline_stage_t *stage_anomaly_baseline_create(uint32_t target_tag, uint32_t learning_samples, double threshold_sigma, uint32_t alarm_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    baseline_ctx_t *ctx = calloc(1, sizeof(baseline_ctx_t));
    
    ctx->target_tag = target_tag;
    ctx->learning_samples = learning_samples > 0 ? learning_samples : 100;
    ctx->threshold_sigma = threshold_sigma > 0 ? threshold_sigma : 3.0;
    ctx->alarm_tag = alarm_tag;
    ctx->is_learning = true;
    
    strncpy(s->name, "BASELINE", 31);
    s->process = _proc;
    s->priv_state = ctx;
    s->cleanup_state = _cleanup;
    
    return s;
}

pipeline_stage_t *stage_anomaly_baseline_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    // f1 = target_tag, f2 = samples, f3 = sigma, u1 = alarm_tag
    return stage_anomaly_baseline_create((uint32_t)def->params.f1, (uint32_t)def->params.f2, def->params.f3, (uint32_t)def->params.u1);
}
