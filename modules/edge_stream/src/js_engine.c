#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <quickjs/quickjs.h>

#include "arbiter.h"
#include "datahub.h"
#include "edge_log.h"
#include "edge_service.h"

typedef struct {
    JSRuntime *rt;
    JSContext *ctx;
    service_t *svc;
} js_engine_t;

#define JS_EXEC_TIMEOUT_MS 100

static int js_interrupt_handler(JSRuntime *rt, void *opaque) {
    (void)rt;
    if (!opaque) return 0;
    uint64_t *start_time = (uint64_t *)opaque;
    uint64_t now = uv_hrtime() / 1000000;  // milliseconds
    if (now - *start_time > JS_EXEC_TIMEOUT_MS) {
        LOG_WARN(LOG_CAT_SCHED, "JS script execution timed out (> %dms), terminating.",
                 JS_EXEC_TIMEOUT_MS);
        return 1;  // Terminate
    }
    return 0;
}

// --- JS Bindings: DataHub.read(tag_id) ---
static JSValue js_datahub_read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_EXCEPTION;
    uint32_t tag_id;
    if (JS_ToUint32(ctx, &tag_id, argv[0])) return JS_EXCEPTION;

    js_engine_t *engine = JS_GetContextOpaque(ctx);
    datahub_t *hub = service_get_datahub(engine->svc);
    
    pipe_msg_t msg;
    if (datahub_read_shadow(hub, tag_id, &msg) == 0) {
        return JS_NewFloat64(ctx, msg.payload.val.f);
    }
    return JS_NULL;
}

// --- JS Bindings: Arbiter.getStatuses() ---
static JSValue js_arbiter_get_statuses(JSContext *ctx, JSValueConst this_val, int argc,
                                       JSValueConst *argv) {
    (void)this_val; (void)argc; (void)argv;
    js_engine_t *engine = JS_GetContextOpaque(ctx);
    arbiter_t *arb = service_get_arbiter(engine->svc);
    if (!arb) return JS_NULL;

    arbiter_bus_status_t stats[ARBITER_MAX_BUSES];
    int count = arbiter_get_statuses(arb, stats, ARBITER_MAX_BUSES);

    JSValue arr = JS_NewArray(ctx);
    for (int i = 0; i < count; i++) {
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "bus_id", JS_NewInt32(ctx, stats[i].bus_id));
        JS_SetPropertyStr(ctx, obj, "is_busy", JS_NewBool(ctx, stats[i].is_busy));
        JS_SetPropertyStr(ctx, obj, "queue_len", JS_NewInt32(ctx, stats[i].queue_len));
        JS_SetPropertyUint32(ctx, arr, (uint32_t)i, obj);
    }
    return arr;
}

// --- JS Bindings: Arbiter.submit(bus_id, tag_id, value) ---
static JSValue js_arbiter_submit(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 3) return JS_EXCEPTION;
    int bus_id; uint32_t tag_id; double value;
    JS_ToInt32(ctx, &bus_id, argv[0]);
    JS_ToUint32(ctx, &tag_id, argv[1]);
    JS_ToFloat64(ctx, &value, argv[2]);

    js_engine_t *engine = JS_GetContextOpaque(ctx);
    arbiter_t *arb = service_get_arbiter(engine->svc);
    
    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(cmd, tag_id, value, ARBITER_PRIO_NORMAL);
    
    arbiter_submit_cmd(arb, bus_id, cmd);
    return JS_UNDEFINED;
}

void js_engine_register_globals(JSContext *ctx, service_t *svc) {
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue dcu_obj = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, dcu_obj, "read", JS_NewCFunction(ctx, js_datahub_read, "read", 1));
    JS_SetPropertyStr(ctx, dcu_obj, "getStatuses", JS_NewCFunction(ctx, js_arbiter_get_statuses, "getStatuses", 0));
    JS_SetPropertyStr(ctx, dcu_obj, "submit", JS_NewCFunction(ctx, js_arbiter_submit, "submit", 3));
    
    JS_SetPropertyStr(ctx, global_obj, "DCU", dcu_obj);
    JS_FreeValue(ctx, global_obj);
}

static uint8_t *js_load_file(JSContext *ctx, size_t *pout_len, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = js_malloc(ctx, (size_t)size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t read_bytes = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[read_bytes] = '\0';
    *pout_len = read_bytes;
    return buf;
}

js_engine_t *js_engine_create(service_t *svc, size_t mem_limit) {
    js_engine_t *engine = calloc(1, sizeof(js_engine_t));
    if (!engine) return NULL;

    engine->svc = svc;
    engine->rt = JS_NewRuntime();
    engine->ctx = JS_NewContext(engine->rt);
    JS_SetContextOpaque(engine->ctx, engine);

    if (mem_limit > 0) {
        JS_SetMemoryLimit(engine->rt, mem_limit);
    } else {
        JS_SetMemoryLimit(engine->rt, 1024 * 1024);  // Default 1MB
    }

    js_engine_register_globals(engine->ctx, svc);
    return engine;
}

JSValue js_engine_eval(js_engine_t *engine, const char *code, const char *filename) {
    if (!engine || !code) return JS_EXCEPTION;

    uint64_t start_time = uv_hrtime() / 1000000;
    JS_SetInterruptHandler(engine->rt, js_interrupt_handler, &start_time);

    JSValue val = JS_Eval(engine->ctx, code, strlen(code), filename, JS_EVAL_TYPE_GLOBAL);

    JS_SetInterruptHandler(engine->rt, NULL, NULL);

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

    JS_SetInterruptHandler(engine->rt, NULL, NULL);

    int ret = 0;
    if (JS_IsException(val)) {
        ret = -1;
    }
    JS_FreeValue(engine->ctx, val);
    js_free(engine->ctx, buf);
    return ret;
}

uv_loop_t *js_engine_get_loop(js_engine_t *engine) {
    return engine ? service_get_loop(engine->svc) : NULL;
}

JSContext *js_engine_get_context(js_engine_t *engine) {
    return engine ? engine->ctx : NULL;
}

void js_engine_destroy(js_engine_t *engine) {
    if (!engine) return;
    JS_RunGC(engine->rt);
    JS_FreeContext(engine->ctx);
    JS_FreeRuntime(engine->rt);
    free(engine);
}
