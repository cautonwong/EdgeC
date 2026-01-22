#include "js_engine.h"

#include <quickjs/quickjs-libc.h>
#include <quickjs/quickjs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "edge_log.h"
#include "edge_service.h"

#define JS_EXEC_TIMEOUT_MS 10

struct js_engine_s {
    service_t *svc;
    JSRuntime *rt;
    JSContext *ctx;
    size_t mem_limit;
};

// --- [SE-02] Interrupt Handler for Fuel Safety ---
static int js_interrupt_handler(JSRuntime *rt, void *opaque) {
    uint64_t *start_time = (uint64_t *)opaque;
    uint64_t now = uv_hrtime() / 1000000; // milliseconds
    if (now - *start_time > JS_EXEC_TIMEOUT_MS) {
        LOG_WARN(LOG_CAT_SCHED, "JS script execution timed out (> %dms), terminating.", JS_EXEC_TIMEOUT_MS);
        return 1; // Terminate
    }
    return 0;
}


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

// --- JS Bindings: Arbiter.getStatuses() ---
static JSValue js_arbiter_get_statuses(JSContext *ctx, JSValueConst this_val, int argc,
                                       JSValueConst *argv) {
    (void)this_val;
    (void)argc;
    (void)argv;
    js_engine_t *engine = JS_GetContextOpaque(ctx);
    arbiter_t *arb = service_get_arbiter(engine->svc);
    if (!arb) return JS_NULL;

    arbiter_bus_status_t stats[BUS_MAX];
    int count = arbiter_get_statuses(arb, stats, BUS_MAX);

    JSValue arr = JS_NewArray(ctx);
    for (int i = 0; i < count; i++) {
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "bus_id", JS_NewInt32(ctx, stats[i].bus_id));
        JS_SetPropertyStr(ctx, obj, "is_busy", JS_NewBool(ctx, stats[i].is_busy));
        JS_SetPropertyStr(ctx, obj, "queue_len", JS_NewInt32(ctx, stats[i].queue_len));
        JS_SetPropertyStr(ctx, obj, "current_job", JS_NewString(ctx, stats[i].current_job_id));
        JS_SetPropertyUint32(ctx, arr, (uint32_t)i, obj);
    }
    return arr;
}

// --- JS Bindings: System.getPoolStats() ---
static JSValue js_system_get_pool_stats(JSContext *ctx, JSValueConst this_val, int argc,
                                        JSValueConst *argv) {
    (void)this_val;
    (void)argc;
    (void)argv;
    js_engine_t *engine = JS_GetContextOpaque(ctx);
    datahub_t *hub = service_get_datahub(engine->svc);
    if (!hub) return JS_NULL;

    net_buf_pool_stats_t stats;

    if (hub->io_pool) {
        net_buf_pool_audit(hub->io_pool, 0, &stats);  // 0 表示只审计不回收

    } else {
        memset(&stats, 0, sizeof(stats));
    }

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "total", JS_NewUint32(ctx, stats.total_count));
    JS_SetPropertyStr(ctx, obj, "free", JS_NewUint32(ctx, stats.free_count));
    JS_SetPropertyStr(ctx, obj, "peak", JS_NewUint32(ctx, stats.peak_usage));
    return obj;
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
        cmd_msg_init_write(&cmd, tag_id, value, ARBITER_PRIO_NORMAL);
        // 实际上此处应由 Orchestrator 包装后再提交，此处为演示直接注入总线 0
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

    // [SE-03] Enforce memory limit
    if (mem_limit > 0) {
        JS_SetMemoryLimit(engine->rt, mem_limit);
    } else {
        JS_SetMemoryLimit(engine->rt, 1024 * 1024); // Default 1MB
    }

    JSValue global_obj = JS_GetGlobalObject(engine->ctx);

    // 1. console
    JSValue console = JS_NewObject(engine->ctx);
    JS_SetPropertyStr(engine->ctx, console, "log",
                      JS_NewCFunction(engine->ctx, js_console_log, "log", 1));
    JS_SetPropertyStr(engine->ctx, global_obj, "console", console);

    // 2. Arbiter
    JSValue arb = JS_NewObject(engine->ctx);
    JS_SetPropertyStr(engine->ctx, arb, "submit",
                      JS_NewCFunction(engine->ctx, js_arbiter_submit, "submit", 2));
    JS_SetPropertyStr(engine->ctx, arb, "getStatuses",
                      JS_NewCFunction(engine->ctx, js_arbiter_get_statuses, "getStatuses", 0));
    JS_SetPropertyStr(engine->ctx, global_obj, "Arbiter", arb);

    // 3. System
    JSValue sys = JS_NewObject(engine->ctx);
    JS_SetPropertyStr(engine->ctx, sys, "getPoolStats",
                      JS_NewCFunction(engine->ctx, js_system_get_pool_stats, "getPoolStats", 0));
    JS_SetPropertyStr(engine->ctx, global_obj, "System", sys);

    JS_FreeValue(engine->ctx, global_obj);
    return engine;
}

JSValue js_engine_eval(js_engine_t *engine, const char *code, const char *filename) {
    if (!engine || !code) return JS_EXCEPTION;

    uint64_t start_time = uv_hrtime() / 1000000;
    JS_SetInterruptHandler(engine->rt, js_interrupt_handler, &start_time);

    JSValue val = JS_Eval(engine->ctx, code, strlen(code), filename, JS_EVAL_TYPE_GLOBAL);

    JS_SetInterruptHandler(engine->rt, NULL, NULL); // Disable handler

    if (JS_IsException(val)) {
        js_std_dump_error(engine->ctx);
    }
    return val;
}

int js_engine_run_file(js_engine_t *engine, const char *file_path) {
    if (!engine || !file_path) return -1;
    size_t len;
    uint8_t *buf = js_load_file(engine->ctx, &len, file_path);
    if (!buf) {
        fprintf(stderr, "[JS] Failed to load file: %s\n", file_path);
        return -1;
    }

    uint64_t start_time = uv_hrtime() / 1000000;
    JS_SetInterruptHandler(engine->rt, js_interrupt_handler, &start_time);

    JSValue val = JS_Eval(engine->ctx, (const char *)buf, len, file_path, JS_EVAL_TYPE_GLOBAL);
    
    JS_SetInterruptHandler(engine->rt, NULL, NULL); // Disable handler

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

struct JSContext *js_engine_get_context(js_engine_t *engine) {
    return engine ? engine->ctx : NULL;
}

void js_engine_destroy(js_engine_t *engine) {
    if (!engine) return;
    JS_RunGC(engine->rt);
    JS_FreeContext(engine->ctx);
    JS_FreeRuntime(engine->rt);
    free(engine);
}