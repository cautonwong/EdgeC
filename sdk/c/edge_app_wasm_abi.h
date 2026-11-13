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
};

EDGE_APP_IMPORT("point_resolve")
int edge_abi_point_resolve(const char *name, uint32_t *handle, uint32_t *access);

EDGE_APP_IMPORT("point_read")
int edge_abi_point_read(uint32_t handle, double *value, uint32_t *quality, uint64_t *source_ts);

EDGE_APP_IMPORT("point_publish")
int edge_abi_point_publish(uint32_t handle, double value, uint32_t quality, uint64_t source_ts);

EDGE_APP_IMPORT("app_log")
void edge_abi_log(uint32_t level, const char *message);

EDGE_APP_IMPORT("app_now")
uint64_t edge_abi_now(void);

#endif /* EDGE_APP_WASM_ABI_H */
