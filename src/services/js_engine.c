#include "services/js_engine.h"
#include <quickjs/quickjs.h>
#include <quickjs/quickjs-libc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct js_engine_s {
    service_t *svc;
    JSRuntime *rt;
    JSContext *ctx;
    size_t mem_limit;
    uint64_t start_time;
    bool interrupted;
};

// --- Helper: Get engine from context ---
static js_engine_t* get_engine(JSContext *ctx) {
    return (js_engine_t *)JS_GetRuntimeOpaque(JS_GetRuntime(ctx));
}

// --- JS Bindings: Logging ---
static JSValue js_log_info(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    (void)this_val; (void)argc;
    const char *str = JS_ToCString(ctx, argv[0]);
    if (str) {
        printf("[JS-INFO] %s\n", str); // Corrected newline escape
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

// --- JS Bindings: DataHub ---
static JSValue js_datahub_read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    (void)this_val; (void)argc;
    js_engine_t *engine = get_engine(ctx);
    (void)engine;
    uint32_t tag_id;
    if (JS_ToUint32(ctx, &tag_id, argv[0])) return JS_EXCEPTION;

    // Simulate reading from shm_ring
    // In real implementation: pipe_msg_t msg; shm_ring_read(engine->svc->data_hub->ring, ...)
    // For now, return a mock value based on tag_id
    double val = 220.0 + (tag_id % 10);
    return JS_NewFloat64(ctx, val);
}

// --- JS Bindings: Arbiter ---
static JSValue js_arbiter_control(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    (void)this_val; (void)argc;
    js_engine_t *engine = get_engine(ctx);
    uint32_t tag_id;
    double val;
    
    if (JS_ToUint32(ctx, &tag_id, argv[0])) return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &val, argv[1])) return JS_EXCEPTION;

    // Bridge to Command Bridge
    service_cmd_t cmd = {
        .type = SVC_CMD_WRITE,
        .tag_id = tag_id,
        .value = val
    };
    service_post_cmd(engine->svc, &cmd);

    return JS_NewBool(ctx, true);
}

// --- Interrupt Handler (Execution Timeout) ---
static int js_interrupt_handler(JSRuntime *rt, void *opaque) {
    (void)rt;
    js_engine_t *engine = (js_engine_t *)opaque;
    // Check if we exceeded 100ms (Simplified check)
    // In real app, use uv_hrtime() or similar
    return engine->interrupted; 
}

// --- Engine Lifecycle ---

js_engine_t* js_engine_create(service_t *svc, size_t mem_limit) {
    js_engine_t *engine = calloc(1, sizeof(js_engine_t));
    if (!engine) return NULL;

    engine->svc = svc;
    engine->mem_limit = mem_limit;
    engine->rt = JS_NewRuntime();
    if (!engine->rt) {
        free(engine);
        return NULL;
    }

    JS_SetMemoryLimit(engine->rt, mem_limit);
    JS_SetRuntimeOpaque(engine->rt, engine);
    JS_SetInterruptHandler(engine->rt, js_interrupt_handler, engine);

    engine->ctx = JS_NewContext(engine->rt);
    if (!engine->ctx) {
        JS_FreeRuntime(engine->rt);
        free(engine);
        return NULL;
    }

    // --- Inject EdgeOS API Object ---
    JSValue global_obj = JS_GetGlobalObject(engine->ctx);    
    // Log API
    JSValue log_obj = JS_NewObject(engine->ctx);
    JS_SetPropertyStr(engine->ctx, log_obj, "info", JS_NewCFunction(engine->ctx, js_log_info, "info", 1));
    JS_SetPropertyStr(engine->ctx, global_obj, "Log", log_obj);

    // DataHub API
    JSValue dh_obj = JS_NewObject(engine->ctx);
    JS_SetPropertyStr(engine->ctx, dh_obj, "read", JS_NewCFunction(engine->ctx, js_datahub_read, "read", 1));
    JS_SetPropertyStr(engine->ctx, global_obj, "DataHub", dh_obj);

    // Arbiter API
    JSValue arb_obj = JS_NewObject(engine->ctx);
    JS_SetPropertyStr(engine->ctx, arb_obj, "control", JS_NewCFunction(engine->ctx, js_arbiter_control, "control", 2));
    JS_SetPropertyStr(engine->ctx, global_obj, "Arbiter", arb_obj);

    JS_FreeValue(engine->ctx, global_obj);

    return engine;
}

int js_engine_eval(js_engine_t *engine, const char *code, const char *filename) {
    JSValue val = JS_Eval(engine->ctx, code, strlen(code), filename, JS_EVAL_TYPE_GLOBAL);
    int ret = 0;
    if (JS_IsException(val)) {
        js_std_dump_error(engine->ctx);
        ret = -1;
    }
    JS_FreeValue(engine->ctx, val);
    return ret;
}

int js_engine_run_file(js_engine_t *engine, const char *file_path) {
    size_t len;
    uint8_t *buf = js_load_file(engine->ctx, &len, file_path);
    if (!buf) return -1;

    JSValue val = JS_Eval(engine->ctx, (const char *)buf, len, file_path, JS_EVAL_TYPE_GLOBAL);
    int ret = 0;
    if (JS_IsException(val)) {
        js_std_dump_error(engine->ctx);
        ret = -1;
    }
    JS_FreeValue(engine->ctx, val);
    js_free(engine->ctx, buf);
    return ret;
}

uv_loop_t* js_engine_get_loop(js_engine_t *engine) {
    return engine->svc->loop;
}

void js_engine_destroy(js_engine_t *engine) {
    if (!engine) return;
    JS_FreeContext(engine->ctx);
    JS_FreeRuntime(engine->rt);
    free(engine);
}
