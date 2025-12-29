#include "js_engine.h"

#include <quickjs/quickjs-libc.h>
#include <quickjs/quickjs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct js_engine_s {
    service_t *svc;
    JSRuntime *rt;
    JSContext *ctx;
    size_t mem_limit;
};

// --- JS Bindings: console.log ---
static JSValue js_console_log(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    (void)this_val;
    for (int i = 0; i < argc; i++) {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (str) {
            printf("[JS] %s%s", str, (i == argc - 1) ? "" : " ");
            JS_FreeCString(ctx, str);
        }
    }
    printf("\n");
    return JS_UNDEFINED;
}

// --- JS Bindings: DataHub.read(tag_id) ---
static JSValue js_datahub_read(JSContext *ctx, JSValueConst this_val, int argc,
                               JSValueConst *argv) {
    (void)this_val;
    js_engine_t *engine = JS_GetContextOpaque(ctx);
    if (!engine || argc < 1) return JS_EXCEPTION;

    uint32_t tag_id;
    JS_ToUint32(ctx, &tag_id, argv[0]);

    datahub_t *hub = service_get_datahub(engine->svc);
    if (!hub) return JS_NULL;

    // 从 SHM 中读取最新数据
    PipeMsg msg;
    uint64_t seq = 0;  // 演示：总是尝试读取最新
    if (shm_ring_read(&hub->ring, &seq, &msg) == 0) {
        // 在全场景示例中，DataHub 应提供按 ID 查找的索引
        // 这里仅作为演示逻辑
        if (msg.tag_id == tag_id) {
            return JS_NewFloat64(ctx, msg.payload.val.f);
        }
    }
    return JS_NULL;
}

// --- JS Bindings: Arbiter.submit(tag_id, value) ---
static JSValue js_arbiter_submit(JSContext *ctx, JSValueConst this_val, int argc,
                                 JSValueConst *argv) {
    (void)this_val;
    js_engine_t *engine = JS_GetContextOpaque(ctx);
    if (!engine || argc < 2) return JS_EXCEPTION;

    uint32_t tag_id;
    double value;
    JS_ToUint32(ctx, &tag_id, argv[0]);
    JS_ToFloat64(ctx, &value, argv[1]);

    arbiter_t *arb = service_get_arbiter(engine->svc);
    if (arb) {
        cmd_msg_t cmd;
        cmd_msg_init_write(&cmd, tag_id, value, CMD_PRIO_NORMAL);
        arbiter_submit_cmd(arb, 0, &cmd);
        return JS_TRUE;
    }
    return JS_FALSE;
}

// --- Lifecycle ---

js_engine_t *js_engine_create(service_t *svc, size_t mem_limit) {
    js_engine_t *engine = calloc(1, sizeof(js_engine_t));
    if (!engine) return NULL;

    engine->svc = svc;
    engine->rt = JS_NewRuntime();
    engine->ctx = JS_NewContext(engine->rt);
    JS_SetContextOpaque(engine->ctx, engine);

    if (mem_limit > 0) {
        JS_SetMemoryLimit(engine->rt, mem_limit);
    }

    JSValue global_obj = JS_GetGlobalObject(engine->ctx);

    // 1. 注入 console.log
    JSValue console = JS_NewObject(engine->ctx);
    JS_SetPropertyStr(engine->ctx, console, "log",
                      JS_NewCFunction(engine->ctx, js_console_log, "log", 1));
    JS_SetPropertyStr(engine->ctx, global_obj, "console", console);

    // 2. 注入 DataHub 对象
    JSValue j_hub = JS_NewObject(engine->ctx);
    JS_SetPropertyStr(engine->ctx, j_hub, "read",
                      JS_NewCFunction(engine->ctx, js_datahub_read, "read", 1));
    JS_SetPropertyStr(engine->ctx, global_obj, "DataHub", j_hub);

    // 3. 注入 Arbiter 对象
    JSValue j_arb = JS_NewObject(engine->ctx);
    JS_SetPropertyStr(engine->ctx, j_arb, "submit",
                      JS_NewCFunction(engine->ctx, js_arbiter_submit, "submit", 2));
    JS_SetPropertyStr(engine->ctx, global_obj, "Arbiter", j_arb);

    JS_FreeValue(engine->ctx, global_obj);
    return engine;
}

int js_engine_eval(js_engine_t *engine, const char *code, const char *filename) {
    if (!engine || !code) return -1;
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
    if (!engine || !file_path) return -1;
    size_t len;
    uint8_t *buf = js_load_file(engine->ctx, &len, file_path);
    if (!buf) {
        fprintf(stderr, "[JS] Failed to load file: %s\n", file_path);
        return -1;
    }

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

uv_loop_t *js_engine_get_loop(js_engine_t *engine) {
    return engine ? service_get_loop(engine->svc) : NULL;
}

void js_engine_destroy(js_engine_t *engine) {
    if (!engine) return;

    // 强制执行一次 GC 以清理残留对象
    JS_RunGC(engine->rt);

    JS_FreeContext(engine->ctx);
    JS_FreeRuntime(engine->rt);
    free(engine);
}
