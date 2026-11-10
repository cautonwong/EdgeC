/**
 * @file rest_control_port.h
 * @brief Consumer-defined control port for the REST management plane.
 *
 * The REST server owns this contract. The composition root supplies an adapter
 * (for example the IEMS control adapter), so edge_io never includes app-manager
 * or stream-engine component headers.
 */

#ifndef REST_CONTROL_PORT_H
#define REST_CONTROL_PORT_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *ctx;

    /** Malloc'd JSON array of installed apps; caller frees. */
    char *(*apps_json)(void *ctx);

    int (*start_app)(void *ctx, const char *app_id);
    int (*stop_app)(void *ctx, const char *app_id);

    /**
     * Install and start an app, then attach its wasm stage to the pipeline.
     * manifest_json may be NULL; name/trigger_tag/entry_func build the default
     * manifest in that case. wasm may be NULL for a metadata-only install.
     */
    int (*install_app)(void *ctx, const char *manifest_json, const char *name,
                       uint32_t trigger_tag, const char *entry_func,
                       const void *wasm, size_t wasm_len);

    /** Malloc'd stream topology JSON; caller frees. NULL when unavailable. */
    char *(*stream_topology_json)(void *ctx);
} rest_control_port_t;

#endif /* REST_CONTROL_PORT_H */
