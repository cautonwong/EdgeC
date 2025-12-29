#ifndef EDGE_JS_ENGINE_H
#define EDGE_JS_ENGINE_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "edge_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct js_engine_s js_engine_t;
struct JSContext;

/**
 * @brief Initialize the QuickJS Scripting Service.
 *
 * @param svc The main service container (for context binding).
 * @param mem_limit Memory limit in bytes (e.g., 2*1024*1024 for 2MB).
 * @return js_engine_t* Pointer to the engine instance, or NULL on failure.
 */
js_engine_t *js_engine_create(service_t *svc, size_t mem_limit);

/**
 * @brief Run a JavaScript script from a file.
 *
 * @param engine The engine instance.
 * @param file_path Path to the .js file.
 * @return 0 on success, negative on error.
 */
int js_engine_run_file(js_engine_t *engine, const char *file_path);

/**
 * @brief Evaluate a string of JavaScript code.
 *
 * @param engine The engine instance.
 * @param code The JS code string.
 * @param filename A virtual filename for error reporting.
 * @return 0 on success.
 */
int js_engine_eval(js_engine_t *engine, const char *code, const char *filename);

/**
 * @brief Get the libuv loop associated with the engine.
 */
uv_loop_t *js_engine_get_loop(js_engine_t *engine);

/**
 * @brief Get the underlying JS context.
 */
struct JSContext *js_engine_get_context(js_engine_t *engine);

/**
 * @brief Shutdown the JS engine and release resources.
 */
void js_engine_destroy(js_engine_t *engine);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_JS_ENGINE_H
