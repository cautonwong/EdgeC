/**
 * @file app_secret.h
 * @brief Frozen host/admin contract for per-app secret storage.
 *
 * Values are caller-encrypted ciphertext. This layer never sees, derives, or
 * returns plaintext; device-key encryption is outside the frozen interface.
 * The admin path can set, delete, and list names. There is no read-back, and
 * nothing here logs a value.
 */
#ifndef EDGEOS_APP_SECRET_H
#define EDGEOS_APP_SECRET_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Largest stored ciphertext, in bytes. */
#define EDGE_SECRET_VALUE_MAX 4096u
/** Largest number of secret names per app. */
#define EDGE_SECRET_PER_APP_MAX 64u
#define EDGE_SECRET_APP_ID_SIZE 64
#define EDGE_SECRET_NAME_SIZE 64

typedef enum {
    EDGE_SECRET_OK = 0,
    /** NULL args, or empty / oversized app_id or name, or empty value. */
    EDGE_SECRET_INVALID,
    /** Value over 4 KB on set, or output buffer too small to hold a stored value. */
    EDGE_SECRET_TOO_LARGE,
    /** 64 names already stored for this app; only new names are refused. */
    EDGE_SECRET_LIMIT,
    /** The store read or write failed. */
    EDGE_SECRET_STORE,
} edge_secret_result_t;

/** Wire code for a result, e.g. "too_large". Never includes a value. */
const char *edge_secret_result_code(edge_secret_result_t result);

/**
 * Store port. The host layer owns every limit; stores only persist bytes.
 * get returns 1 when found (copies min(len, cap) bytes and reports the full
 * length through *len; out may be NULL to probe existence), 0 when absent,
 * and -1 on error.
 */
typedef struct {
    void *ctx;
    int (*get)(void *ctx, const char *app_id, const char *name, void *out, size_t cap, size_t *len);
    int (*put)(void *ctx, const char *app_id, const char *name, const void *value, size_t len);
    /** 0 on success (an absent name is success), -1 on error. */
    int (*remove)(void *ctx, const char *app_id, const char *name);
    /** Names only. Copies at most max names and sets *count. 0 or -1. */
    int (*list_names)(void *ctx, const char *app_id, char names[][EDGE_SECRET_NAME_SIZE],
                      size_t max, size_t *count);
    int (*count)(void *ctx, const char *app_id, size_t *count);
} edge_secret_store_t;

/**
 * Validate and upsert by (app_id, name). The 4 KB limit applies to the value;
 * the 64-entry limit applies only when name is not already stored. On failure
 * reason receives a short code ("invalid", "too_large", "limit", "store_error").
 */
edge_secret_result_t edge_secret_set(const edge_secret_store_t *store, const char *app_id,
                                     const char *name, const void *value, size_t len, char *reason,
                                     size_t reason_size);

/** Delete one name. Absent names succeed so retries converge. */
edge_secret_result_t edge_secret_delete(const edge_secret_store_t *store, const char *app_id,
                                        const char *name);

/** List names only; values never leave the store through this path. */
edge_secret_result_t edge_secret_list(const edge_secret_store_t *store, const char *app_id,
                                      char names[][EDGE_SECRET_NAME_SIZE], size_t max,
                                      size_t *count);

/**
 * App-side read path. Copies the stored ciphertext and its length. Returns
 * INVALID when the name is absent, TOO_LARGE when cap cannot hold it.
 */
edge_secret_result_t edge_secret_get_for_app(const edge_secret_store_t *store, const char *app_id,
                                             const char *name, void *out, size_t cap, size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_APP_SECRET_H */
