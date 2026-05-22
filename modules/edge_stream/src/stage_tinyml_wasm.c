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

typedef struct {
    char entry_func[64];
    wasm_app_t *app;
    float *window;
    size_t window_size;
    size_t cursor;
    uint32_t output_tag;
} tinyml_ctx_t;

static stage_ret_t tinyml_stage_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    tinyml_ctx_t *ctx = (tinyml_ctx_t *)stage->priv_cfg;
    if (!ctx || !ctx->app) return STAGE_DROP;

    if (ev->type == PIPEMSG_TYPE_SCALAR) {
        // 1. 窗口采样
        ctx->window[ctx->cursor++] = (float)ev->payload.val.f;
        
        if (ctx->cursor >= ctx->window_size) {
            ctx->cursor = 0; // 重置窗口
            
            // 2. 将窗口数据推送到 WASM 内存 (此处简化：假设 WASM 通过 ABI 读取)
            // 生产环境可使用 wasm_runtime_module_malloc 在 WASM 堆上分配空间并拷贝
            
            // 3. 执行推理
            if (wasm_app_exec(ctx->app, ctx->entry_func, ev) == 0) {
                // 结果通常由 WASM 通过 ext_datahub_publish 发布
                return STAGE_DROP; 
            }
        }
    }

    return STAGE_DROP;
}

static void tinyml_stage_cleanup(void *ptr) {
    tinyml_ctx_t *ctx = (tinyml_ctx_t *)ptr;
    if (ctx->app) wasm_app_destroy(ctx->app);
    if (ctx->window) free(ctx->window);
    free(ctx);
}

/**
 * @brief 创建 TinyML WASM 算子
 * 参数: s1=wasm_path, s2=entry_func, f1=window_size, u1=output_tag
 */
pipeline_stage_t *stage_tinyml_wasm_create(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    if (!def->params.s1) return NULL;

    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "TINYML_WASM", 31);
    s->process = tinyml_stage_proc;

    tinyml_ctx_t *ctx = calloc(1, sizeof(tinyml_ctx_t));
    strncpy(ctx->entry_func, def->params.s2 ? def->params.s2 : "inference", 63);
    
    ctx->window_size = (size_t)def->params.f1;
    if (ctx->window_size == 0) ctx->window_size = 128;
    ctx->window = calloc(ctx->window_size, sizeof(float));
    ctx->output_tag = (uint32_t)def->params.u1;

    // 创建安全容器 (128KB 堆，用于 TFLite)
    ctx->app = wasm_app_create(def->params.s1, 128 * 1024, 64 * 1024, 200);
    if (!ctx->app) {
        free(ctx->window); free(ctx); free(s); return NULL;
    }

    s->priv_cfg = ctx;
    s->cleanup_cfg = tinyml_stage_cleanup;

    LOG_INFO(LOG_CAT_CORE, "TinyML WASM Stage Created: %s (Window: %zu)", def->params.s1, ctx->window_size);
    return s;
}

#endif
