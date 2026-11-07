#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <quickjs/quickjs.h>

#include "arbiter.h"
#include "edge_log.h"
#include "js_arbiter_port.h"
#include "stream_bus_port.h"

typedef struct {
    JSRuntime *rt;
    JSContext *ctx;
    uv_loop_t *loop;
    js_arbiter_port_t arb;
    stream_bus_port_t bus;
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
static JSValue js_bus_read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_EXCEPTION;
    uint32_t tag_id;
    if (JS_ToUint32(ctx, &tag_id, argv[0])) return JS_EXCEPTION;

    js_engine_t *engine = JS_GetContextOpaque(ctx);

    pipe_msg_t msg;
    if (stream_bus_read_shadow(&engine->bus, tag_id, &msg) == 0) {
        return JS_NewFloat64(ctx, msg.payload.val.f);
    }
    return JS_NULL;
}

// --- JS Bindings: Arbiter.getStatuses() ---
static JSValue js_arbiter_get_statuses(JSContext *ctx, JSValueConst this_val, int argc,
                                       JSValueConst *argv) {
    (void)this_val; (void)argc; (void)argv;
    js_engine_t *engine = JS_GetContextOpaque(ctx);
    if (!engine->arb.get_statuses) return JS_NULL;

    arbiter_bus_status_t stats[ARBITER_MAX_BUSES];
    int count = js_arbiter_port_get_statuses(&engine->arb, stats, ARBITER_MAX_BUSES);

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
    if (!engine->arb.submit) return JS_UNDEFINED;

    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(cmd, tag_id, value, ARBITER_PRIO_NORMAL);

    js_arbiter_port_submit(&engine->arb, bus_id, cmd);
    return JS_UNDEFINED;
}

void js_engine_register_globals(JSContext *ctx) {
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue dcu_obj = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, dcu_obj, "read", JS_NewCFunction(ctx, js_bus_read, "read", 1));
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

size_t js_engine_storage_size(void) {
    return sizeof(js_engine_t);
}

js_engine_t *js_engine_create_in(void *storage, size_t storage_size, uv_loop_t *loop,
                                 const js_arbiter_port_t *arb,
                                 const stream_bus_port_t *bus, size_t mem_limit) {
    if (!storage || storage_size < sizeof(js_engine_t)) return NULL;
    js_engine_t *engine = storage;
    memset(engine, 0, sizeof(*engine));
    engine->loop = loop;
    engine->arb = arb ? *arb : (js_arbiter_port_t){0};
    engine->bus = bus ? *bus : (stream_bus_port_t){0};
    engine->rt = JS_NewRuntime();
    engine->ctx = JS_NewContext(engine->rt);
    JS_SetContextOpaque(engine->ctx, engine);

    if (mem_limit > 0) {
        JS_SetMemoryLimit(engine->rt, mem_limit);
    } else {
        JS_SetMemoryLimit(engine->rt, 1024 * 1024);  // Default 1MB
    }

    js_engine_register_globals(engine->ctx);
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
    return engine ? engine->loop : NULL;
}

JSContext *js_engine_get_context(js_engine_t *engine) {
    return engine ? engine->ctx : NULL;
}

void js_engine_destroy(js_engine_t *engine) {
    if (!engine) return;
    JS_RunGC(engine->rt);
    JS_FreeContext(engine->ctx);
    JS_FreeRuntime(engine->rt);
}
