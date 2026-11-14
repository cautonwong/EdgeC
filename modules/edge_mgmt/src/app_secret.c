/**
 * @file app_secret.c
 * @brief Host layer for the frozen per-app secret contract.
 */

#include "edgeos/app_secret.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

const char *edge_secret_result_code(edge_secret_result_t result) {
    switch (result) {
        case EDGE_SECRET_OK:
            return "ok";
        case EDGE_SECRET_INVALID:
            return "invalid";
        case EDGE_SECRET_TOO_LARGE:
            return "too_large";
        case EDGE_SECRET_LIMIT:
            return "limit";
        case EDGE_SECRET_STORE:
            return "store_error";
        default:
            return "store_error";
    }
}

static edge_secret_result_t _fail(edge_secret_result_t result, char *reason, size_t reason_size) {
    if (reason && reason_size) snprintf(reason, reason_size, "%s", edge_secret_result_code(result));
    return result;
}

static bool _valid_key(const char *key, size_t size) {
    if (!key) return false;
    size_t len = strlen(key);
    return len > 0 && len < size;
}

edge_secret_result_t edge_secret_set(const edge_secret_store_t *store, const char *app_id,
                                     const char *name, const void *value, size_t len, char *reason,
                                     size_t reason_size) {
    if (!store || !store->get || !store->put || !store->count) {
        return _fail(EDGE_SECRET_INVALID, reason, reason_size);
    }
    if (!_valid_key(app_id, EDGE_SECRET_APP_ID_SIZE) || !_valid_key(name, EDGE_SECRET_NAME_SIZE) ||
        !value || len == 0) {
        return _fail(EDGE_SECRET_INVALID, reason, reason_size);
    }
    if (len > EDGE_SECRET_VALUE_MAX) return _fail(EDGE_SECRET_TOO_LARGE, reason, reason_size);

    size_t stored_len = 0;
    int found = store->get(store->ctx, app_id, name, NULL, 0, &stored_len);
    if (found < 0) return _fail(EDGE_SECRET_STORE, reason, reason_size);

    if (found == 0) {
        size_t count = 0;
        if (store->count(store->ctx, app_id, &count) != 0) {
            return _fail(EDGE_SECRET_STORE, reason, reason_size);
        }
        if (count >= EDGE_SECRET_PER_APP_MAX) return _fail(EDGE_SECRET_LIMIT, reason, reason_size);
    }

    if (store->put(store->ctx, app_id, name, value, len) != 0) {
        return _fail(EDGE_SECRET_STORE, reason, reason_size);
    }
    if (reason && reason_size) snprintf(reason, reason_size, "ok");
    return EDGE_SECRET_OK;
}

edge_secret_result_t edge_secret_delete(const edge_secret_store_t *store, const char *app_id,
                                        const char *name) {
    if (!store || !store->remove) return EDGE_SECRET_INVALID;
    if (!_valid_key(app_id, EDGE_SECRET_APP_ID_SIZE) || !_valid_key(name, EDGE_SECRET_NAME_SIZE)) {
        return EDGE_SECRET_INVALID;
    }
    return store->remove(store->ctx, app_id, name) == 0 ? EDGE_SECRET_OK : EDGE_SECRET_STORE;
}

edge_secret_result_t edge_secret_list(const edge_secret_store_t *store, const char *app_id,
                                      char names[][EDGE_SECRET_NAME_SIZE], size_t max,
                                      size_t *count) {
    if (!store || !store->list_names || !names || !count) return EDGE_SECRET_INVALID;
    if (!_valid_key(app_id, EDGE_SECRET_APP_ID_SIZE)) return EDGE_SECRET_INVALID;
    *count = 0;
    return store->list_names(store->ctx, app_id, names, max, count) == 0 ? EDGE_SECRET_OK
                                                                         : EDGE_SECRET_STORE;
}

edge_secret_result_t edge_secret_get_for_app(const edge_secret_store_t *store, const char *app_id,
                                             const char *name, void *out, size_t cap, size_t *len) {
    if (!store || !store->get || !out || cap == 0 || !len) return EDGE_SECRET_INVALID;
    if (!_valid_key(app_id, EDGE_SECRET_APP_ID_SIZE) || !_valid_key(name, EDGE_SECRET_NAME_SIZE)) {
        return EDGE_SECRET_INVALID;
    }

    *len = 0;
    int found = store->get(store->ctx, app_id, name, out, cap, len);
    if (found < 0) return EDGE_SECRET_STORE;
    if (found == 0) return EDGE_SECRET_INVALID;
    if (*len > cap) return EDGE_SECRET_TOO_LARGE;
    return EDGE_SECRET_OK;
}
