/**
 * @file edge_app_wasm_abi.h
 * @brief Host ABI v1 imports for wasm apps.
 *
 * app-host registers these under the "env" module. Handles come from
 * point_resolve; access is 0 for read and 1 for publish. point_read fills the
 * value, quality (0 good, 1 bad, 2 uncertain) and source timestamp in ms.
 * Errors are the EDGE_APP_ERR_* indices of the WIT error-code enum, offset by
 * one so that 0 means success.
 */
#ifndef EDGE_APP_WASM_ABI_H
#define EDGE_APP_WASM_ABI_H

#include <stdint.h>
#include <string.h>

#define EDGE_APP_IMPORT(name) __attribute__((import_module("env"), import_name(name)))

enum {
    EDGE_APP_OK = 0,
    EDGE_APP_ERR_DENIED_NOT_GRANTED = 1,
    EDGE_APP_ERR_DENIED_SCOPE = 2,
    EDGE_APP_ERR_DENIED_EXPIRED = 3,
    EDGE_APP_ERR_DENIED_REVOKED = 4,
    EDGE_APP_ERR_DENIED_POLICY = 5,
    EDGE_APP_ERR_DENIED_QUOTA = 6,
    EDGE_APP_ERR_DENIED_PACKAGE_MISMATCH = 7,
    EDGE_APP_ERR_DENIED_ABI = 8,
    EDGE_APP_ERR_INVALID_MANIFEST = 9,
    EDGE_APP_ERR_NOT_FOUND = 10,
    EDGE_APP_ERR_STALE = 11,
    EDGE_APP_ERR_BUSY = 12,
    EDGE_APP_ERR_INVALID = 13,
    EDGE_APP_ERR_INTERNAL = 14,
    /* Ext-only: no value and no default. Not in the frozen WIT error enum. */
    EDGE_APP_ERR_NOT_SET = 15,
    /* Ext-only: the host product has no settings store. */
    EDGE_APP_ERR_NOT_SUPPORTED = 16,
};

EDGE_APP_IMPORT("point_resolve")
int edge_abi_point_resolve(const char *name, uint32_t *handle, uint32_t *access);

EDGE_APP_IMPORT("point_read")
int edge_abi_point_read(uint32_t handle, double *value, uint32_t *quality, uint64_t *source_ts);

EDGE_APP_IMPORT("point_subscribe")
int edge_abi_point_subscribe(uint32_t handle, double deadband, uint32_t min_interval_ms,
                             uint32_t max_interval_ms, uint32_t *subscription);

EDGE_APP_IMPORT("point_publish")
int edge_abi_point_publish(uint32_t handle, double value, uint32_t quality, uint64_t source_ts);

EDGE_APP_IMPORT("app_log")
void edge_abi_log(uint32_t level, const char *message);

EDGE_APP_IMPORT("app_now")
uint64_t edge_abi_now(void);

/* Ext import, not part of the frozen WIT host interface. Writes the setting as
 * NUL-terminated JSON text. Returns 0 or an EDGE_APP_ERR_* code. */
EDGE_APP_IMPORT("setting_get")
int edge_abi_setting_get_raw(const char *key, uint32_t key_len, char *out, uint32_t cap);

static inline int edge_abi_setting_get(const char *key, char *out, uint32_t cap) {
    return edge_abi_setting_get_raw(key, (uint32_t)strlen(key), out, cap);
}

/* Ext import, not part of the frozen WIT host interface. Asks the host to
 * deliver settings_changed events for later operator writes on this connection. */
EDGE_APP_IMPORT("setting_watch")
int edge_abi_setting_watch(void);

#endif /* EDGE_APP_WASM_ABI_H */
