#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <quickjs/quickjs.h>

#include "arbiter.h"
#include "datahub.h"
#include "edge_log.h"
#include "edge_service.h"

typedef struct {
    JSContext *ctx;
    service_t *svc;
} js_engine_t;

static int js_interrupt_handler(JSRuntime *rt, void *opaque) {
    (void)rt; (void)opaque;
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
