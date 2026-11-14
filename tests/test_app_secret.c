/**
 * @file test_app_secret.c
 * @brief Limits and storage round-trip for the frozen secret contract.
 */

#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "edge_secret_sqlite.h"
#include "edgeos/app_secret.h"
#include "sqlite_service.h"

typedef struct {
    char app_id[EDGE_SECRET_APP_ID_SIZE];
    char name[EDGE_SECRET_NAME_SIZE];
    unsigned char value[EDGE_SECRET_VALUE_MAX];
    size_t len;
} mem_entry_t;

typedef struct {
    mem_entry_t entries[EDGE_SECRET_PER_APP_MAX * 2];
    size_t count;
    int fail;
} mem_store_t;

static mem_entry_t *_find(mem_store_t *store, const char *app_id, const char *name) {
    for (size_t i = 0; i < store->count; i++) {
        if (strcmp(store->entries[i].app_id, app_id) == 0 &&
            strcmp(store->entries[i].name, name) == 0)
            return &store->entries[i];
    }
    return NULL;
}

static int _mem_get(void *ctx, const char *app_id, const char *name, void *out, size_t cap,
                    size_t *len) {
    mem_store_t *store = (mem_store_t *)ctx;
    if (store->fail) return -1;
    mem_entry_t *entry = _find(store, app_id, name);
    if (!entry) return 0;
    if (len) *len = entry->len;
    if (out && cap > 0) memcpy(out, entry->value, entry->len < cap ? entry->len : cap);
    return 1;
}

static int _mem_put(void *ctx, const char *app_id, const char *name, const void *value,
                    size_t len) {
    mem_store_t *store = (mem_store_t *)ctx;
    if (store->fail) return -1;
    mem_entry_t *entry = _find(store, app_id, name);
    if (!entry) {
        if (store->count >= sizeof(store->entries) / sizeof(store->entries[0])) return -1;
        entry = &store->entries[store->count++];
        snprintf(entry->app_id, sizeof(entry->app_id), "%s", app_id);
        snprintf(entry->name, sizeof(entry->name), "%s", name);
    }
    memcpy(entry->value, value, len);
    entry->len = len;
    return 0;
}

static int _mem_remove(void *ctx, const char *app_id, const char *name) {
    mem_store_t *store = (mem_store_t *)ctx;
    if (store->fail) return -1;
    mem_entry_t *entry = _find(store, app_id, name);
    if (!entry) return 0;
    size_t index = (size_t)(entry - store->entries);
    store->entries[index] = store->entries[store->count - 1];
    store->count--;
    return 0;
}

static int _mem_list_names(void *ctx, const char *app_id, char names[][EDGE_SECRET_NAME_SIZE],
                           size_t max, size_t *count) {
    mem_store_t *store = (mem_store_t *)ctx;
    if (store->fail) return -1;
    size_t copied = 0;
    for (size_t i = 0; i < store->count && copied < max; i++) {
        if (strcmp(store->entries[i].app_id, app_id) == 0) {
            snprintf(names[copied++], EDGE_SECRET_NAME_SIZE, "%s", store->entries[i].name);
        }
    }
    *count = copied;
    return 0;
}

static int _mem_count(void *ctx, const char *app_id, size_t *count) {
    mem_store_t *store = (mem_store_t *)ctx;
    if (store->fail) return -1;
    size_t total = 0;
    for (size_t i = 0; i < store->count; i++) {
        if (strcmp(store->entries[i].app_id, app_id) == 0) total++;
    }
    *count = total;
    return 0;
}

static edge_secret_store_t _mem_port(mem_store_t *store) {
    edge_secret_store_t port = {
        .ctx = store,
        .get = _mem_get,
        .put = _mem_put,
        .remove = _mem_remove,
        .list_names = _mem_list_names,
        .count = _mem_count,
    };
    return port;
}

static void _on_close_cb(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int setup(void **state) {
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    return 0;
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    uv_walk(loop, _on_close_cb, NULL);
    while (uv_loop_close(loop) == UV_EBUSY) uv_run(loop, UV_RUN_ONCE);
    free(loop);
    return 0;
}

static void test_limits_upsert_and_4kb_boundary(void **state) {
    (void)state;
    mem_store_t mem = {0};
    edge_secret_store_t store = _mem_port(&mem);
    char reason[32];

    const unsigned char first[] = {0xde, 0xad, 0xbe, 0xef};
    assert_int_equal(
        edge_secret_set(&store, "app.demo", "token", first, sizeof(first), reason, sizeof(reason)),
        EDGE_SECRET_OK);
    assert_string_equal(reason, "ok");

    unsigned char out[EDGE_SECRET_VALUE_MAX];
    size_t len = 0;
    assert_int_equal(edge_secret_get_for_app(&store, "app.demo", "token", out, sizeof(out), &len),
                     EDGE_SECRET_OK);
    assert_int_equal(len, sizeof(first));
    assert_memory_equal(out, first, sizeof(first));

    const unsigned char second[] = {0x01};
    assert_int_equal(edge_secret_set(&store, "app.demo", "token", second, sizeof(second), reason,
                                     sizeof(reason)),
                     EDGE_SECRET_OK);
    assert_int_equal(edge_secret_get_for_app(&store, "app.demo", "token", out, sizeof(out), &len),
                     EDGE_SECRET_OK);
    assert_int_equal(len, sizeof(second));
    assert_memory_equal(out, second, sizeof(second));

    char name[EDGE_SECRET_NAME_SIZE];
    for (int i = 1; i < (int)EDGE_SECRET_PER_APP_MAX; i++) {
        snprintf(name, sizeof(name), "key%d", i);
        assert_int_equal(edge_secret_set(&store, "app.demo", name, second, sizeof(second), reason,
                                         sizeof(reason)),
                         EDGE_SECRET_OK);
    }
    assert_int_equal(edge_secret_set(&store, "app.demo", "one-too-many", second, sizeof(second),
                                     reason, sizeof(reason)),
                     EDGE_SECRET_LIMIT);
    assert_string_equal(reason, "limit");

    // Upserting an existing name at the limit still succeeds.
    assert_int_equal(edge_secret_set(&store, "app.demo", "token", second, sizeof(second), reason,
                                     sizeof(reason)),
                     EDGE_SECRET_OK);

    unsigned char max_value[EDGE_SECRET_VALUE_MAX];
    memset(max_value, 0xa5, sizeof(max_value));
    assert_int_equal(edge_secret_set(&store, "app.big", "blob", max_value, sizeof(max_value),
                                     reason, sizeof(reason)),
                     EDGE_SECRET_OK);
    assert_int_equal(edge_secret_set(&store, "app.big", "blob", max_value, sizeof(max_value) + 1,
                                     reason, sizeof(reason)),
                     EDGE_SECRET_TOO_LARGE);
    assert_string_equal(reason, "too_large");

    // A buffer smaller than the stored value is reported, with the full length.
    assert_int_equal(edge_secret_get_for_app(&store, "app.big", "blob", out, 16, &len),
                     EDGE_SECRET_TOO_LARGE);
    assert_int_equal(len, sizeof(max_value));
}

static void test_invalid_delete_and_list(void **state) {
    (void)state;
    mem_store_t mem = {0};
    edge_secret_store_t store = _mem_port(&mem);
    char reason[32];
    const unsigned char value[] = {0x11, 0x22};

    assert_int_equal(
        edge_secret_set(&store, "", "token", value, sizeof(value), reason, sizeof(reason)),
        EDGE_SECRET_INVALID);
    assert_string_equal(reason, "invalid");
    assert_int_equal(
        edge_secret_set(&store, "app.demo", "", value, sizeof(value), reason, sizeof(reason)),
        EDGE_SECRET_INVALID);
    assert_int_equal(
        edge_secret_set(&store, "app.demo", "token", NULL, sizeof(value), reason, sizeof(reason)),
        EDGE_SECRET_INVALID);
    assert_int_equal(edge_secret_set(&store, "app.demo", "token", value, 0, reason, sizeof(reason)),
                     EDGE_SECRET_INVALID);

    assert_int_equal(
        edge_secret_set(&store, "app.demo", "alpha", value, sizeof(value), reason, sizeof(reason)),
        EDGE_SECRET_OK);
    assert_int_equal(
        edge_secret_set(&store, "app.demo", "beta", value, sizeof(value), reason, sizeof(reason)),
        EDGE_SECRET_OK);

    char names[EDGE_SECRET_PER_APP_MAX][EDGE_SECRET_NAME_SIZE];
    size_t count = 0;
    assert_int_equal(edge_secret_list(&store, "app.demo", names, EDGE_SECRET_PER_APP_MAX, &count),
                     EDGE_SECRET_OK);
    assert_int_equal(count, 2);
    assert_string_equal(names[0], "alpha");
    assert_string_equal(names[1], "beta");

    assert_int_equal(edge_secret_delete(&store, "app.demo", "alpha"), EDGE_SECRET_OK);
    assert_int_equal(edge_secret_list(&store, "app.demo", names, EDGE_SECRET_PER_APP_MAX, &count),
                     EDGE_SECRET_OK);
    assert_int_equal(count, 1);
    assert_string_equal(names[0], "beta");

    // Deleting an absent name converges instead of failing.
    assert_int_equal(edge_secret_delete(&store, "app.demo", "alpha"), EDGE_SECRET_OK);

    unsigned char out[16];
    size_t len = 0;
    assert_int_equal(edge_secret_get_for_app(&store, "app.demo", "alpha", out, sizeof(out), &len),
                     EDGE_SECRET_INVALID);
}

static void test_store_failure_maps_to_store(void **state) {
    (void)state;
    mem_store_t mem = {0};
    edge_secret_store_t store = _mem_port(&mem);
    char reason[32];
    const unsigned char value[] = {0x42};

    mem.fail = 1;
    assert_int_equal(
        edge_secret_set(&store, "app.demo", "token", value, sizeof(value), reason, sizeof(reason)),
        EDGE_SECRET_STORE);
    assert_string_equal(reason, "store_error");
    assert_int_equal(edge_secret_delete(&store, "app.demo", "token"), EDGE_SECRET_STORE);

    char names[EDGE_SECRET_PER_APP_MAX][EDGE_SECRET_NAME_SIZE];
    size_t count = 0;
    assert_int_equal(edge_secret_list(&store, "app.demo", names, EDGE_SECRET_PER_APP_MAX, &count),
                     EDGE_SECRET_STORE);

    unsigned char out[16];
    size_t len = 0;
    assert_int_equal(edge_secret_get_for_app(&store, "app.demo", "token", out, sizeof(out), &len),
                     EDGE_SECRET_STORE);
}

static void test_sqlite_roundtrip(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_app_secret.db";
    unlink(path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    edge_secret_store_t store = edge_secret_sqlite(svc);
    char reason[32];

    const unsigned char value[] = {0x00, 0xff, 0x10, 0x80, 0x7f};
    assert_int_equal(
        edge_secret_set(&store, "app.demo", "cloud", value, sizeof(value), reason, sizeof(reason)),
        EDGE_SECRET_OK);

    unsigned char out[32];
    size_t len = 0;
    assert_int_equal(edge_secret_get_for_app(&store, "app.demo", "cloud", out, sizeof(out), &len),
                     EDGE_SECRET_OK);
    assert_int_equal(len, sizeof(value));
    assert_memory_equal(out, value, sizeof(value));

    char names[EDGE_SECRET_PER_APP_MAX][EDGE_SECRET_NAME_SIZE];
    size_t count = 0;
    assert_int_equal(edge_secret_list(&store, "app.demo", names, EDGE_SECRET_PER_APP_MAX, &count),
                     EDGE_SECRET_OK);
    assert_int_equal(count, 1);
    assert_string_equal(names[0], "cloud");

    // The table keeps the ciphertext byte for byte and has no read-back path.
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    sqlite3_stmt *stmt;
    assert_int_equal(sqlite3_prepare_v2(db,
                                        "SELECT hex(value) FROM app_secret WHERE app_id='app.demo' "
                                        "AND name='cloud';",
                                        -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_string_equal((const char *)sqlite3_column_text(stmt, 0), "00FF10807F");
    sqlite3_finalize(stmt);

    assert_int_equal(edge_secret_delete(&store, "app.demo", "cloud"), EDGE_SECRET_OK);
    assert_int_equal(edge_secret_list(&store, "app.demo", names, EDGE_SECRET_PER_APP_MAX, &count),
                     EDGE_SECRET_OK);
    assert_int_equal(count, 0);

    sqlite_svc_destroy(svc);
    unlink(path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_limits_upsert_and_4kb_boundary, setup, teardown),
        cmocka_unit_test_setup_teardown(test_invalid_delete_and_list, setup, teardown),
        cmocka_unit_test_setup_teardown(test_store_failure_maps_to_store, setup, teardown),
        cmocka_unit_test_setup_teardown(test_sqlite_roundtrip, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
