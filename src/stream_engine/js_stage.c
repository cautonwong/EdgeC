#include <quickjs/quickjs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "js_engine.h"
#include "stream_engine.h"

// --- Main context for the JS stage ---
typedef struct {
    js_engine_t *engine;
    JSValue *persistent_state;
    char script_path[128];
    bool is_async;
} js_stage_ctx_t;

// --- Synchronous execution path ---
static stage_ret_t run_js_sync(pipeline_stage_t *stage, pipe_msg_t *ev) {
    js_stage_ctx_t *ctx = (js_stage_ctx_t *)stage->priv_cfg;
    JSContext *js_ctx = js_engine_get_context(ctx->engine);

    JSValue j_ev = JS_NewObject(js_ctx);
    JS_SetPropertyStr(js_ctx, j_ev, "tag", JS_NewUint32(js_ctx, ev->tag_id));
    JS_SetPropertyStr(js_ctx, j_ev, "val", JS_NewFloat64(js_ctx, ev->payload.val.f));
    JS_SetPropertyStr(js_ctx, j_ev, "ts", JS_NewInt64(js_ctx, (int64_t)ev->ts));

    JSValue global_obj = JS_GetGlobalObject(js_ctx);
    JSValue func = JS_GetPropertyStr(js_ctx, global_obj, "on_event");

    stage_ret_t status = STAGE_OK;
    if (JS_IsFunction(js_ctx, func)) {
        JSValue args[2] = {j_ev, *ctx->persistent_state};
        JSValue ret = JS_Call(js_ctx, func, JS_UNDEFINED, 2, args);

        if (JS_IsBool(ret) && !JS_ToBool(js_ctx, ret)) {
            status = STAGE_DROP;
        } else if (JS_IsObject(ret)) {
            JSValue next_val = JS_GetPropertyStr(js_ctx, ret, "val");
            if (JS_IsNumber(next_val)) {
                JS_ToFloat64(js_ctx, &ev->payload.val.f, next_val);
            }
            JS_FreeValue(js_ctx, next_val);
        }
        JS_FreeValue(js_ctx, ret);
    }

    JS_FreeValue(js_ctx, func);
    JS_FreeValue(js_ctx, global_obj);
    JS_FreeValue(js_ctx, j_ev);
    return status;
}

// --- Asynchronous execution path ---

typedef struct {
    uv_work_t req;
    pipeline_t *pipeline;
    char script_path[128];
    pipe_msg_t event;
    // No JSValue here, as it cannot cross threads
} js_work_ctx_t;

static void js_work_cb(uv_work_t *req) {
    js_work_ctx_t *work = req->data;

    // Create a temporary, isolated JS runtime and context for this thread
    js_engine_t *temp_engine = js_engine_create(NULL, 1024 * 1024);  // 1MB limit
    if (!temp_engine) return;

    // This is a simplified execution, it does not have the full context (e.g. persistent_state)
    // A more advanced version might serialize/deserialize state or use thread-safe JS values.
    js_engine_run_file(temp_engine, work->script_path);

    js_engine_destroy(temp_engine);
}

static void js_after_work_cb(uv_work_t *req, int status) {
    (void)status;
    js_work_ctx_t *work = req->data;
    stream_engine_resume(work->pipeline);
    free(work);
}

// --- Main stage process function ---
static stage_ret_t js_stage_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;

    js_stage_ctx_t *ctx = (js_stage_ctx_t *)stage->priv_cfg;

    if (!ctx->is_async) {
        return run_js_sync(stage, ev);
    } else {
        js_work_ctx_t *work_ctx = malloc(sizeof(js_work_ctx_t));
        work_ctx->req.data = work_ctx;
        work_ctx->pipeline = stage->pipeline;
        work_ctx->event = *ev;
        strncpy(work_ctx->script_path, ctx->script_path, 127);

        uv_queue_work(stage->loop, &work_ctx->req, js_work_cb, js_after_work_cb);
        return STAGE_ASYNC;
    }
}

static void js_stage_cleanup(void *ptr) {
    js_stage_ctx_t *ctx = (js_stage_ctx_t *)ptr;
    // This cleanup runs on the compute thread, so it's safe to free JSValue
    JSContext *js_ctx = js_engine_get_context(ctx->engine);
    JS_FreeValue(js_ctx, *ctx->persistent_state);
    free(ctx->persistent_state);
    free(ctx);
}

pipeline_stage_t *stage_js_rule_create(js_engine_t *engine, const char *script_path,
                                       bool is_async) {
    if (!engine) return NULL;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "js_rule", 31);
    s->process = js_stage_proc;

    js_stage_ctx_t *ctx = calloc(1, sizeof(js_stage_ctx_t));
    ctx->engine = engine;
    strncpy(ctx->script_path, script_path, 127);
    ctx->is_async = is_async;

    // The persistent state is only used for sync mode for now
    JSContext *js_ctx = js_engine_get_context(engine);
    ctx->persistent_state = malloc(sizeof(JSValue));
    *ctx->persistent_state = JS_NewObject(js_ctx);

    // Pre-load the script into the main engine context for sync execution
    js_engine_run_file(engine, script_path);

    s->priv_cfg = ctx;
    s->cleanup_cfg = js_stage_cleanup;
    return s;
}