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

    /** Malloc'd JSON with the clock trust state; NULL when unavailable. */
    char *(*time_status_json)(void *ctx);

    /** Mark the wall clock trusted; epoch_ms 0 means the current clock. */
    int (*time_mark)(void *ctx, uint64_t epoch_ms, char *reason, size_t reason_size);

    int (*start_app)(void *ctx, const char *app_id);
    int (*stop_app)(void *ctx, const char *app_id);

    /**
     * Uninstall an app. disposition is "keep" (default), "purge" or "export".
     * export_path is required for "export" and ignored otherwise.
     */
    int (*uninstall_app)(void *ctx, const char *app_id, const char *disposition,
                         const char *export_path);

    /**
     * Install and start an app, then attach its wasm stage to the pipeline.
     * manifest_json may be NULL; name/trigger_tag/entry_func build the default
     * manifest in that case. wasm may be NULL for a metadata-only install.
     * idempotency_key may be NULL; a replay of a terminal task is answered from
     * the ledger without running the operation again.
     */
    int (*install_app)(void *ctx, const char *manifest_json, const char *name,
                       uint32_t trigger_tag, const char *entry_func,
                       const void *wasm, size_t wasm_len, const char *idempotency_key);

    /**
     * Install from a signed `.eapp` package. The port verifies the container
     * signature, manifest schema and wasm imports before installing.
     * Returns 0 when the install is queued, negative when the package or the
     * install is rejected.
     */
    int (*install_package)(void *ctx, const uint8_t *data, size_t len,
                           const char *idempotency_key,
                           const uint8_t expected_pubkey[32]);

    /**
     * Upgrade from a signed `.eapp` package. The port verifies the container
     * and swaps the package. Returns 0 when queued.
     */
    int (*upgrade_package)(void *ctx, const char *app_id, const uint8_t *data, size_t len,
                           const char *idempotency_key);

    /** Malloc'd stream topology JSON; caller frees. NULL when unavailable. */
    char *(*stream_topology_json)(void *ctx);

    /**
     * Approve a capability grant. scope_json is a JSON array of exact scopes.
     * Returns 0 on success, negative on rejection or store failure.
     */
    int (*approve_grant)(void *ctx, const char *app_id, const char *capability,
                         const char *scope_json, const char *purpose, uint64_t expires_at,
                         uint8_t sensitivity_max, const char *actor);

    /** Revoke one capability grant. Returns 0 on success. */
    int (*revoke_grant)(void *ctx, const char *app_id, const char *capability);

    /**
     * Upgrade an installed app from a manifest JSON string and, when wasm is
     * present, swap the package. Grant inheritance runs inside. idempotency_key
     * may be NULL. Returns 0 on success or when a replayed task says the same
     * request already succeeded / is still running, negative otherwise.
     */
    int (*upgrade_app)(void *ctx, const char *manifest_json, const void *wasm, size_t wasm_len,
                       const char *idempotency_key);

    /**
     * Roll back one upgrade to the previous version. Returns 0 when the job is
     * queued, negative when it cannot be submitted.
     */
    int (*rollback_app)(void *ctx, const char *app_id);

    /** Malloc'd JSON array of grants; app_id empty lists all. Caller frees. */
    char *(*grants_json)(void *ctx, const char *app_id);

    /**
     * Secret admin path. Values are caller-encrypted ciphertext; there is no
     * read-back. secret_set returns 0 on success, otherwise -1 and writes a
     * short code into reason. secret_list returns a malloc'd JSON array of
     * names only; caller frees.
     */
    int (*secret_set)(void *ctx, const char *app_id, const char *name, const void *value,
                      size_t len, char *reason, size_t reason_size);
    int (*secret_delete)(void *ctx, const char *app_id, const char *name);
    char *(*secret_list)(void *ctx, const char *app_id);

    /**
     * Task ledger admin path. Submit returns malloc'd JSON with the task and a
     * "replayed" flag; status returns malloc'd task JSON or NULL; finish returns
     * 0 on success; purge returns malloc'd JSON with the removed count.
     */
    char *(*task_submit)(void *ctx, const char *kind, const char *idempotency_key);
    char *(*task_status)(void *ctx, const char *id);
    int (*task_finish)(void *ctx, const char *id, const char *state, const char *result_json);
    char *(*task_purge)(void *ctx);

    /**
     * Registry status JSON (generation, apps, consumers, converged) and a
     * publish action that writes gen-<N>.json atomically and returns its own
     * JSON result. Both are malloc'd; caller frees. NULL when unavailable.
     */
    char *(*registry_status_json)(void *ctx, const char *app_id);
    char *(*registry_snapshot)(void *ctx);

    /**
     * Consumer heartbeat: remember the generation a reader has applied.
     * Returns 0 when accepted, negative when the consumer table is full.
     */
    int (*consumer_heartbeat)(void *ctx, const char *name,
                              unsigned long long applied_generation);
} rest_control_port_t;

#endif /* REST_CONTROL_PORT_H */
