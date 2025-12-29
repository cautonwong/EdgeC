#include <quickjs/quickjs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "js_engine.h"
#include "stream_engine.h"

/**
 * @brief 有状态的 JS 规则算子
 * 对齐规范 9.2：C 侧持有状态对象，JS 负责计算逻辑
 */
stage_ret_t stage_js_rule(pipeline_stage_t *stage, pipe_event_t *ev) {
    js_engine_t *engine = (js_engine_t *)stage->priv_cfg;
    if (!engine || ev->type != EVT_DATA_POINT) return STAGE_OK;

    JSContext *ctx = js_engine_get_context(engine);

    // 1. 准备事件对象
    JSValue j_ev = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, j_ev, "tag_id", JS_NewUint32(ctx, ev->tag_id));
    JS_SetPropertyStr(ctx, j_ev, "val", JS_NewFloat64(ctx, ev->payload.val));
    JS_SetPropertyStr(ctx, j_ev, "ts", JS_NewInt64(ctx, (int64_t)ev->calendar_ms));

    // 2. 获取持久化状态对象 (由 C 侧维护)
    JSValue *p_state = (JSValue *)stage->priv_state;

    // 3. 执行脚本中的 on_event(event, state) 函数
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue func = JS_GetPropertyStr(ctx, global_obj, "on_event");

    if (JS_IsFunction(ctx, func)) {
        JSValue args[2];
        args[0] = j_ev;
        args[1] = *p_state;  // 直接传递引用，JS 内部修改会保留

        JSValue ret_val = JS_Call(ctx, func, global_obj, 2, args);
        if (JS_IsException(ret_val)) {
            // Error handling could be added here
        }
        JS_FreeValue(ctx, ret_val);
    }

    JS_FreeValue(ctx, func);
    JS_FreeValue(ctx, global_obj);
    JS_FreeValue(ctx, j_ev);  // args[0] was just a copy of this handle

    return STAGE_OK;
}

static void js_stage_cleanup(void *state) {
    // Ideally we need the context here to free the JSValue
    // For now, just free the container.
    // In a full implementation, engine_destroy would handle JSValue cleanup.
    free(state);
}

pipeline_stage_t *stage_js_rule_create(js_engine_t *engine, const char *script) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "js_rule", 31);
    s->process = stage_js_rule;
    s->priv_cfg = engine;

    JSContext *ctx = js_engine_get_context(engine);

    // Create the persistent state object for this stage
    JSValue *p_state = malloc(sizeof(JSValue));
    *p_state = JS_NewObject(ctx);
    s->priv_state = p_state;
    s->cleanup = js_stage_cleanup;

    // Pre-evaluate the script to define the on_event function
    if (script) {
        js_engine_eval(engine, script, "rule_init.js");
    }

    return s;
}
