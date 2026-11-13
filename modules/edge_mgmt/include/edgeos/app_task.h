/**
 * @file app_task.h
 * @brief Lightweight task records for install, upgrade, and backup.
 *
 * The model covers exactly three kinds. Idempotency keys are honored for 24
 * hours, terminal states are final, and there is no cancel API. Terminal rows
 * are retained for 7 days after the last update.
 */
#ifndef EDGEOS_APP_TASK_H
#define EDGEOS_APP_TASK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_APP_TASK_ID_SIZE 64
#define EDGE_APP_TASK_KEY_SIZE 128
#define EDGE_APP_TASK_RESULT_SIZE 4096
#define EDGE_APP_TASK_IDEMPOTENCY_MS (24LL * 60 * 60 * 1000)
#define EDGE_APP_TASK_RETENTION_MS (7LL * 24 * 60 * 60 * 1000)

typedef enum {
    EDGE_APP_TASK_INSTALL = 1,
    EDGE_APP_TASK_UPGRADE = 2,
    EDGE_APP_TASK_BACKUP = 3,
} edge_app_task_kind_t;

typedef enum {
    EDGE_APP_TASK_PENDING = 0,
    EDGE_APP_TASK_RUNNING = 1,
    EDGE_APP_TASK_SUCCEEDED = 2,
    EDGE_APP_TASK_FAILED = 3,
} edge_app_task_state_t;

typedef enum {
    EDGE_APP_TASK_OK = 0,
    /** NULL args, unknown kind, empty/oversized key, negative time. */
    EDGE_APP_TASK_INVALID,
    /** No task with the requested id. */
    EDGE_APP_TASK_NOT_FOUND,
    /** The store read or write failed. */
    EDGE_APP_TASK_STORE,
} edge_app_task_result_t;

typedef struct {
    char id[EDGE_APP_TASK_ID_SIZE];
    edge_app_task_kind_t kind;
    char idempotency_key[EDGE_APP_TASK_KEY_SIZE];
    edge_app_task_state_t state;
    int64_t created_at;
    int64_t updated_at;
    /** Empty string when the task has no result yet. */
    char result_json[EDGE_APP_TASK_RESULT_SIZE];
} edge_app_task_t;

/**
 * Store port. The host layer owns kind, state, and idempotency rules; the
 * store persists rows. Times are epoch milliseconds.
 */
typedef struct {
    void *ctx;
    /** Upsert by idempotency_key and by id. 0 on success, -1 on error. */
    int (*insert)(void *ctx, const edge_app_task_t *task);
    /** 1 when found and copied to out, 0 when absent, -1 on error. */
    int (*get)(void *ctx, const char *id, edge_app_task_t *out);
    /** 1 when found and copied to out, 0 when absent, -1 on error. */
    int (*find_by_key)(void *ctx, const char *idempotency_key, edge_app_task_t *out);
    /** Updates state, updated_at, and result_json by id. 0 ok, 1 no row, -1 error. */
    int (*update)(void *ctx, const edge_app_task_t *task);
    /** Deletes terminal tasks with updated_at < cutoff_ms. 0 ok, -1 error. */
    int (*purge_expired)(void *ctx, int64_t cutoff_ms, size_t *removed);
} edge_app_task_store_t;

/**
 * Submit a task. A matching key whose task was created within the last 24
 * hours is returned unchanged with *replayed = 1. Otherwise a new PENDING task
 * replaces any expired row for that key and *replayed = 0.
 */
edge_app_task_result_t edge_app_task_submit(const edge_app_task_store_t *store,
                                            edge_app_task_kind_t kind, const char *idempotency_key,
                                            int64_t now, edge_app_task_t *out, int *replayed);

/**
 * Move a task to RUNNING, SUCCEEDED, or FAILED. A terminal task rejects every
 * later transition. result_json may be NULL and must fit in the result field.
 */
edge_app_task_result_t edge_app_task_finish(const edge_app_task_store_t *store, const char *id,
                                            edge_app_task_state_t state, const char *result_json,
                                            int64_t now);

/** Delete terminal tasks whose updated_at is older than 7 days. */
edge_app_task_result_t edge_app_task_purge(const edge_app_task_store_t *store, int64_t now);

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_APP_TASK_H */
