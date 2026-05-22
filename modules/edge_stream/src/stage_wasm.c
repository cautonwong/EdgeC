/**
 * @file stage_wasm.c
 * @brief 安全的 WASM 容器算子 (支持资源隔离与超时中断)
 */

#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "stream_engine.h"
#include "wasm_mgr.h"

#if defined(CONFIG_EDGEC_WASM)

typedef struct {
    char entry_func[64];
    wasm_app_t *app;
} wasm_stage_ctx_t;

static stage_ret_t wasm_stage_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    wasm_stage_ctx_t *ctx = (wasm_stage_ctx_t *)stage->priv_cfg;
    if (!ctx->app) return STAGE_DROP;

    if (wasm_app_exec(ctx->app, ctx->entry_func, ev) != 0) {
        return STAGE_DROP;
    }

    return STAGE_OK;
}

static void wasm_stage_cleanup(void *ptr) {
    wasm_stage_ctx_t *ctx = (wasm_stage_ctx_t *)ptr;
    if (ctx->app) wasm_app_destroy(ctx->app);
    free(ctx);
}

/**
 * @brief 创建 WASM 算子
 * 参数: s1=wasm_path, s2=entry_func, f1=heap_size, f2=stack_size, f3=timeout_ms
 */
pipeline_stage_t *stage_wasm_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    if (!def->params.s1) return NULL;

    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "WASM_SECURE", 31);
    s->process = wasm_stage_proc;

    wasm_stage_ctx_t *ctx = calloc(1, sizeof(wasm_stage_ctx_t));
    strncpy(ctx->entry_func, def->params.s2 ? def->params.s2 : "on_event", 63);
    
    // [Nexus] 从契约读取安全参数
    uint32_t heap = (uint32_t)def->params.f1;
    uint32_t stack = (uint32_t)def->params.f2;
    uint32_t timeout = (uint32_t)def->params.f3;

    ctx->app = wasm_app_create(def->params.s1, heap, stack, timeout);
    if (!ctx->app) {
        free(ctx); free(s); return NULL;
    }

    s->priv_cfg = ctx;
    s->cleanup_cfg = wasm_stage_cleanup;

    LOG_INFO(LOG_CAT_CORE, "WASM Secure Stage Created: %s (Entry: %s)", def->params.s1, ctx->entry_func);
    return s;
}

// 兼容旧接口
pipeline_stage_t *stage_wasm_create(const char *wasm_path, const char *entry_func) {
    edge_stage_def_t def = {0};
    def.params.s1 = wasm_path;
    def.params.s2 = entry_func;
    // ...
    return stage_wasm_create_v2(&def, NULL);
}

#else
pipeline_stage_t *stage_wasm_create(const char *wasm_path, const char *entry_func) {
    (void)wasm_path; (void)entry_func; return NULL;
}
#endif
