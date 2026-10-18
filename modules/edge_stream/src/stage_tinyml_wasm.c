/**
 * @file stage_tinyml_wasm.c
 * @brief 工业级 TinyML WASM 推理算子 (支持特征工程与模型推理)
 */

#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "stream_engine.h"
#include "wasm_mgr.h"
#include "wasm_export.h"

#if defined(CONFIG_EDGEC_WASM)

#define MAX_TINYML_WINDOW 256

typedef struct {
    char entry_func[64];
    wasm_app_t *app;
    float window[MAX_TINYML_WINDOW];
    size_t window_size;
    size_t cursor;
    uint32_t output_tag;
    uint32_t in_tensor_ptr;
    uint32_t out_tensor_ptr;
} tinyml_ctx_t;

static stage_ret_t tinyml_stage_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    tinyml_ctx_t *ctx = (tinyml_ctx_t *)stage->priv_cfg;
    if (!ctx || !ctx->app) return STAGE_DROP;

    if (ev->type == PIPEMSG_TYPE_SCALAR) {
        // 1. 窗口物理采样
        ctx->window[ctx->cursor++] = (float)ev->payload.val.f;
        
        if (ctx->cursor >= ctx->window_size) {
            ctx->cursor = 0; // 重置滑动窗口游标
            
            // 2. 真实将滑动窗口数据推送到 WASM 内存 (AEP-604 / Issue #219)
            if (ctx->in_tensor_ptr == 0) {
                wasm_app_tensor_alloc(ctx->app, (uint32_t)(ctx->window_size * sizeof(float)), &ctx->in_tensor_ptr);
            }
            if (ctx->out_tensor_ptr == 0) {
                wasm_app_tensor_alloc(ctx->app, 4 * sizeof(float), &ctx->out_tensor_ptr);
            }

            if (ctx->in_tensor_ptr > 0 && ctx->out_tensor_ptr > 0) {
                // 精准内存拷贝灌入，杜绝数据被丢弃
                wasm_app_tensor_write(ctx->app, ctx->in_tensor_ptr, ctx->window, (uint32_t)(ctx->window_size * sizeof(float)));

                uint32_t argv[4] = {
                    ctx->in_tensor_ptr,
                    (uint32_t)ctx->window_size,
                    ctx->out_tensor_ptr,
                    4
                };

                // 3. 执行模型多参推理
                if (wasm_app_exec_args(ctx->app, ctx->entry_func, 4, argv) == 0) {
                    float out_scores[4] = {0};
                    if (wasm_app_tensor_read(ctx->app, ctx->out_tensor_ptr, out_scores, sizeof(out_scores)) == 0) {
                        if (ctx->output_tag > 0) {
                            float max_val = out_scores[0];
                            int max_idx = 0;
                            for (int k = 1; k < 4; k++) {
                                if (out_scores[k] > max_val) {
                                    max_val = out_scores[k];
                                    max_idx = k;
                                }
                            }
                            ev->tag_id = ctx->output_tag;
                            ev->payload.val.f = (double)max_idx;
                            return STAGE_OK;
                        }
                    }
                }
            }
        }
    }

    return STAGE_DROP;
}

static void tinyml_stage_cleanup(void *ptr) {
    tinyml_ctx_t *ctx = (tinyml_ctx_t *)ptr;
    if (ctx && ctx->app) {
        if (ctx->in_tensor_ptr) {
            wasm_app_tensor_free(ctx->app, ctx->in_tensor_ptr);
            ctx->in_tensor_ptr = 0;
        }
        if (ctx->out_tensor_ptr) {
            wasm_app_tensor_free(ctx->app, ctx->out_tensor_ptr);
            ctx->out_tensor_ptr = 0;
        }
        wasm_app_destroy(ctx->app);
        ctx->app = NULL;
    }
}

/**
 * @brief 创建 TinyML WASM 算子
 * 参数: s1=wasm_path, s2=entry_func, f1=window_size, u1=output_tag
 */
pipeline_stage_t *stage_tinyml_wasm_create(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    if (!def->params.s1) return NULL;

    pipeline_stage_t *s = stream_stage_alloc_node();
    strncpy(s->name, "TINYML_WASM", 31);
    s->process = tinyml_stage_proc;

    tinyml_ctx_t *ctx = (tinyml_ctx_t *)s->storage_state;
    memset(ctx, 0, sizeof(*ctx));
    strncpy(ctx->entry_func, def->params.s2 ? def->params.s2 : "inference", 63);
    
    ctx->window_size = (size_t)def->params.f1;
    if (ctx->window_size == 0 || ctx->window_size > MAX_TINYML_WINDOW) ctx->window_size = 128;
    ctx->output_tag = (uint32_t)def->params.u1;

    // 创建安全容器 (128KB 堆，用于 TFLite)
    ctx->app = wasm_app_create(def->params.s1, 128 * 1024, 64 * 1024, 200);
    if (!ctx->app) {
        return NULL;
    }

    s->priv_cfg = ctx;
    s->cleanup_cfg = tinyml_stage_cleanup;

    LOG_INFO(LOG_CAT_CORE, "TinyML WASM Stage Created: %s (Window: %zu)", def->params.s1, ctx->window_size);
    return s;
}

#endif
