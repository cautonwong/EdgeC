#include <quickjs/quickjs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "js_engine.h"
#include "stream_engine.h"

typedef struct {
    js_engine_t *engine;
    JSValue *persistent_state;
    char script_path[128];
} js_stage_ctx_t;

static stage_ret_t js_stage_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    js_stage_ctx_t *ctx = (js_stage_ctx_t *)stage->priv_cfg;
    JSContext *js_ctx = js_engine_get_context(ctx->engine);
    JSValue j_ev = JS_NewObject(js_ctx);
    JS_SetPropertyStr(js_ctx, j_ev, "tag", JS_NewUint32(js_ctx, ev->tag_id));
    JS_SetPropertyStr(js_ctx, j_ev, "val", JS_NewFloat64(js_ctx, ev->payload.val));
    JS_SetPropertyStr(js_ctx, j_ev, "ts", JS_NewInt64(js_ctx, (int64_t)ev->calendar_ms));
    JSValue global_obj = JS_GetGlobalObject(js_ctx);
    JSValue func = JS_GetPropertyStr(js_ctx, global_obj, "on_event");
    if (JS_IsFunction(js_ctx, func)) {
        JSValue args[2] = {j_ev, *ctx->persistent_state};
        JSValue ret = JS_Call(js_ctx, func, global_obj, 2, args);
        JS_FreeValue(js_ctx, ret);
    }
    JS_FreeValue(js_ctx, func);
    JS_FreeValue(js_ctx, global_obj);
    JS_FreeValue(js_ctx, j_ev);
    return STAGE_OK;
}

static void js_stage_cleanup(void *ptr) {
    js_stage_ctx_t *ctx = (js_stage_ctx_t *)ptr;
    free(ctx->persistent_state);
    free(ctx);
}

pipeline_stage_t *stage_js_rule_create(js_engine_t *engine, const char *script_path) {
    if (!engine) return NULL;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "js_rule", 31);
    s->process = js_stage_proc;
    js_stage_ctx_t *ctx = calloc(1, sizeof(js_stage_ctx_t));
    ctx->engine = engine;
    strncpy(ctx->script_path, script_path, 127);
    JSContext *js_ctx = js_engine_get_context(engine);
    ctx->persistent_state = malloc(sizeof(JSValue));
    *ctx->persistent_state = JS_NewObject(js_ctx);
    js_engine_run_file(engine, script_path);
    s->priv_cfg = ctx;
    s->cleanup_cfg = js_stage_cleanup;
    return s;
}
