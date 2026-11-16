/**
 * @file stage_compactor.c
 * @brief 工业级载荷压缩算子 (Delta Encoding & Batching)
 */

#include "stream_engine.h"
#include "stream_engine_priv.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>

#define COMPACTOR_MAX_ELEMENTS 4096

typedef struct {
    uint32_t target_tag;
    uint32_t max_elements;
    uint32_t element_count;
    double *buffer;
    double *out_buf;
    uint32_t output_tag;
} compactor_ctx_t;

static stage_ret_t _proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    compactor_ctx_t *ctx = (compactor_ctx_t *)stage->priv_state;
    if (!ctx) return STAGE_ERROR;

    if (ev->type == PIPEMSG_TYPE_SCALAR && ev->tag_id == ctx->target_tag) {
        ctx->buffer[ctx->element_count++] = ev->payload.val.f;

        if (ctx->element_count >= ctx->max_elements) {
            // BATCH 只在本次同步 push 内有效，复用 out_buf 不需要每批分配
            memcpy(ctx->out_buf, ctx->buffer, ctx->element_count * sizeof(double));
            pipe_msg_t batch;
            pipe_msg_init_batch(&batch, ctx->output_tag, ctx->out_buf, (int)ctx->element_count,
                                ev->ts);

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
        free(ctx->buffer);
        free(ctx->out_buf);
    }
}

pipeline_stage_t *stage_compactor_create(uint32_t target_tag, uint32_t max_elements, uint32_t output_tag) {
    if (max_elements > COMPACTOR_MAX_ELEMENTS) {
        LOG_ERROR(LOG_CAT_CORE, "Compactor: max_elements %u exceeds %d", max_elements,
                  COMPACTOR_MAX_ELEMENTS);
        return NULL;
    }

    pipeline_stage_t *s = stream_stage_alloc_node();
    compactor_ctx_t *ctx = (compactor_ctx_t *)s->storage_state;
    memset(ctx, 0, sizeof(*ctx));

    ctx->target_tag = target_tag;
    ctx->max_elements = max_elements > 0 ? max_elements : 10;
    ctx->buffer = calloc(ctx->max_elements, sizeof(double));
    ctx->out_buf = calloc(ctx->max_elements, sizeof(double));
    if (!ctx->buffer || !ctx->out_buf) {
        free(ctx->buffer);
        free(ctx->out_buf);
        return NULL;
    }
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
