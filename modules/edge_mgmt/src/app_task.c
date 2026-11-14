/**
 * @file app_task.c
 * @brief Host layer for the lightweight task contract.
 */

#include "edgeos/app_task.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

static bool _valid_text(const char *text, size_t size) {
    if (!text) return false;
    size_t len = strlen(text);
    return len > 0 && len < size;
}

static bool _valid_kind(edge_app_task_kind_t kind) {
    return kind == EDGE_APP_TASK_INSTALL || kind == EDGE_APP_TASK_UPGRADE ||
           kind == EDGE_APP_TASK_BACKUP;
}

static bool _terminal(edge_app_task_state_t state) {
    return state == EDGE_APP_TASK_SUCCEEDED || state == EDGE_APP_TASK_FAILED;
}

// Id is stable for a key, so an expired key's replacement keeps the same handle.
static void _make_id(const char *key, char out[EDGE_APP_TASK_ID_SIZE]) {
    uint64_t hash = FNV_OFFSET;
    for (const char *p = key; *p; p++) {
        hash ^= (unsigned char)*p;
        hash *= FNV_PRIME;
    }
    snprintf(out, EDGE_APP_TASK_ID_SIZE, "task-%016llx", (unsigned long long)hash);
}

edge_app_task_result_t edge_app_task_submit(const edge_app_task_store_t *store,
                                            edge_app_task_kind_t kind, const char *idempotency_key,
                                            int64_t now, edge_app_task_t *out, int *replayed) {
    if (!store || !store->find_by_key || !store->insert || !out || !replayed) {
        return EDGE_APP_TASK_INVALID;
    }
    if (!_valid_kind(kind) || !_valid_text(idempotency_key, EDGE_APP_TASK_KEY_SIZE) || now < 0) {
        return EDGE_APP_TASK_INVALID;
    }

    edge_app_task_t existing;
    int found = store->find_by_key(store->ctx, idempotency_key, &existing);
    if (found < 0) return EDGE_APP_TASK_STORE;
    if (found == 1 && now - existing.created_at <= EDGE_APP_TASK_IDEMPOTENCY_MS) {
        *out = existing;
        *replayed = 1;
        return EDGE_APP_TASK_OK;
    }

    edge_app_task_t task;
    memset(&task, 0, sizeof(task));
    _make_id(idempotency_key, task.id);
    task.kind = kind;
    snprintf(task.idempotency_key, sizeof(task.idempotency_key), "%s", idempotency_key);
    task.state = EDGE_APP_TASK_PENDING;
    task.created_at = now;
    task.updated_at = now;

    if (store->insert(store->ctx, &task) != 0) return EDGE_APP_TASK_STORE;
    *out = task;
    *replayed = 0;
    return EDGE_APP_TASK_OK;
}

edge_app_task_result_t edge_app_task_finish(const edge_app_task_store_t *store, const char *id,
                                            edge_app_task_state_t state, const char *result_json,
                                            int64_t now) {
    if (!store || !store->get || !store->update) return EDGE_APP_TASK_INVALID;
    if (!_valid_text(id, EDGE_APP_TASK_ID_SIZE) || now < 0) return EDGE_APP_TASK_INVALID;
    if (state != EDGE_APP_TASK_RUNNING && state != EDGE_APP_TASK_SUCCEEDED &&
        state != EDGE_APP_TASK_FAILED) {
        return EDGE_APP_TASK_INVALID;
    }
    if (result_json && strlen(result_json) >= EDGE_APP_TASK_RESULT_SIZE) {
        return EDGE_APP_TASK_INVALID;
    }

    edge_app_task_t task;
    int found = store->get(store->ctx, id, &task);
    if (found < 0) return EDGE_APP_TASK_STORE;
    if (found == 0) return EDGE_APP_TASK_NOT_FOUND;
    if (_terminal(task.state)) return EDGE_APP_TASK_INVALID;

    task.state = state;
    task.updated_at = now;
    if (result_json) snprintf(task.result_json, sizeof(task.result_json), "%s", result_json);

    int updated = store->update(store->ctx, &task);
    if (updated < 0) return EDGE_APP_TASK_STORE;
    return updated == 0 ? EDGE_APP_TASK_OK : EDGE_APP_TASK_NOT_FOUND;
}

edge_app_task_result_t edge_app_task_purge(const edge_app_task_store_t *store, int64_t now) {
    if (!store || !store->purge_expired || now < 0) return EDGE_APP_TASK_INVALID;
    size_t removed = 0;
    return store->purge_expired(store->ctx, now - EDGE_APP_TASK_RETENTION_MS, &removed) == 0
               ? EDGE_APP_TASK_OK
               : EDGE_APP_TASK_STORE;
}
