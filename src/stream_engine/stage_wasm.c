#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "stream_engine.h"
#include "wasm_mgr.h"

#ifdef ENABLE_WASM

typedef struct {
    char wasm_path[128];
    char entry_func[64];
} wasm_stage_ctx_t;

static stage_ret_t wasm_stage_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    wasm_stage_ctx_t *ctx = (wasm_stage_ctx_t *)stage->priv_cfg;

    // 目前简单地同步调用 WASM 函数
    // TODO: 支持异步模式和更复杂的参数传递
    if (wasm_mgr_exec(ctx->wasm_path, ctx->entry_func, ev) != 0) {
        return STAGE_DROP;
    }

    return STAGE_OK;
}

static void wasm_stage_cleanup(void *ptr) {
    free(ptr);
}

pipeline_stage_t *stage_wasm_create(const char *wasm_path, const char *entry_func) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "wasm_executor", 31);
    s->process = wasm_stage_proc;

    wasm_stage_ctx_t *ctx = calloc(1, sizeof(wasm_stage_ctx_t));
    strncpy(ctx->wasm_path, wasm_path, 127);
    strncpy(ctx->entry_func, entry_func ? entry_func : "on_event", 63);

    s->priv_cfg = ctx;
    s->cleanup_cfg = wasm_stage_cleanup;

    LOG_INFO(LOG_CAT_CORE, "WASM Stage Created: %s (Entry: %s)", wasm_path, ctx->entry_func);
    return s;
}

#else

pipeline_stage_t *stage_wasm_create(const char *wasm_path, const char *entry_func) {
    (void)wasm_path;
    (void)entry_func;
    LOG_WARN(LOG_CAT_CORE, "WASM support is disabled in this build");
    return NULL;
}

#endif