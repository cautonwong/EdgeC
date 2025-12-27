#ifndef EDGE_SCRIPT_LOADER_H
#define EDGE_SCRIPT_LOADER_H

#include <uv.h>

#include "services/js_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct script_loader_s script_loader_t;

/**
 * @brief Create a dynamic script loader.
 *
 * @param engine The JS engine instance to use for execution.
 * @param dir_path Directory to watch for .js files.
 * @param interval_ms Execution interval for the loaded scripts.
 * @return script_loader_t* Pointer to the loader instance.
 */
script_loader_t *script_loader_create(js_engine_t *engine, const char *dir_path,
                                      uint64_t interval_ms);

/**
 * @brief Start watching and executing scripts.
 */
int script_loader_start(script_loader_t *loader);

/**
 * @brief Shutdown the loader.
 */
void script_loader_destroy(script_loader_t *loader);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_SCRIPT_LOADER_H
