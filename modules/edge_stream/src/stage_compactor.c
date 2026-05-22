/**
 * @file stage_compactor.c
 * @brief 工业级载荷压缩算子 (Delta Encoding & Batching)
 */

#include "stream_engine.h"
#include "stream_engine_priv.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t target_tag;
    uint32_t max_elements;
    uint32_t element_count;
    double *buffer;
    uint32_t output_tag;
} compactor_ctx_t;

static stage_ret_t _proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    compactor_ctx_t *ctx = (compactor_ctx_t *)stage->priv_state;
    
    if (ev->type == PIPEMSG_TYPE_SCALAR && ev->tag_id == ctx->target_tag) {
        ctx->buffer[ctx->element_count++] = ev->payload.val.f;
        
        if (ctx->element_count >= ctx->max_elements) {
            // [Nexus] Output as BATCH message
            pipe_msg_t batch;
            pipe_msg_init_batch(&batch, ctx->output_tag, ctx->buffer, ctx->element_count, ev->ts);
            
            // We use a clone or a new buffer because init_batch takes a pointer
            // In a real zero-copy system, we'd use net_buf
            // For now, we reuse the buffer but we MUST ensure it's not freed prematurely
            // Actually, stream_engine_push copies the msg but not the batch pointer deep
            // So we must allocate a new buffer for the batch
            double *out_buf = malloc(ctx->element_count * sizeof(double));
            memcpy(out_buf, ctx->buffer, ctx->element_count * sizeof(double));
            batch.payload.batch.ptr = out_buf;
            
            stream_engine_push(stage->pipeline->engine, &batch);
            ctx->element_count = 0;
            return STAGE_OK;
        }
        return STAGE_DROP; // Buffering
    }
    
    return STAGE_DROP;
}

static void _cleanup(void *priv) {
    compactor_ctx_t *ctx = (compactor_ctx_t *)priv;
    if (ctx) {
        if (ctx->buffer) free(ctx->buffer);
        free(ctx);
    }
}

pipeline_stage_t *stage_compactor_create(uint32_t target_tag, uint32_t max_elements, uint32_t output_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    compactor_ctx_t *ctx = calloc(1, sizeof(compactor_ctx_t));
    
    ctx->target_tag = target_tag;
    ctx->max_elements = max_elements > 0 ? max_elements : 10;
    ctx->buffer = calloc(ctx->max_elements, sizeof(double));
    ctx->output_tag = output_tag;
    
    strncpy(s->name, "COMPACTOR", 31);
    s->process = _proc;
    s->priv_state = ctx;
    s->cleanup_state = _cleanup;
    
    LOG_INFO(LOG_CAT_CORE, "Compactor Stage created for Tag %u (Limit: %u)", target_tag, ctx->max_elements);
    return s;
}

pipeline_stage_t *stage_compactor_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    // f1 = target_tag, f2 = max_elements, u1 = output_tag
    return stage_compactor_create((uint32_t)def->params.f1, (uint32_t)def->params.f2, (uint32_t)def->params.u1);
}
