#ifndef EDGE_JS_ENGINE_H
#define EDGE_JS_ENGINE_H

#include <quickjs/quickjs.h>  // 必须包含以使用 JSValue
#include <stdbool.h>
#include <stdint.h>
typedef struct uv_loop_s uv_loop_t;

#include "js_arbiter_port.h"
#include "stream_bus_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct js_engine_s js_engine_t;

/**
 * @brief Initialize the QuickJS Scripting Service.
 */
size_t js_engine_storage_size(void);

js_engine_t *js_engine_create_in(void *storage, size_t storage_size, uv_loop_t *loop,
                                 const js_arbiter_port_t *arb,
                                 const stream_bus_port_t *bus, size_t mem_limit);

/**
 * @brief Run a JavaScript script from a file.
 */
int js_engine_run_file(js_engine_t *engine, const char *file_path);

/**
 * @brief Evaluate a string of JavaScript code.
 * @return The result JSValue. Caller must free it.
 */
JSValue js_engine_eval(js_engine_t *engine, const char *code, const char *filename);

/**
 * @brief Get the libuv loop associated with the engine.
 */
uv_loop_t *js_engine_get_loop(js_engine_t *engine);

/**
 * @brief Get the underlying JS context.
 */
JSContext *js_engine_get_context(js_engine_t *engine);

/**
 * @brief Shutdown the JS engine and release resources.
 */
void js_engine_destroy(js_engine_t *engine);

#ifdef __cplusplus
}
#endif

#endif