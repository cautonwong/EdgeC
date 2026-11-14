/**
 * @file test_app_task.c
 * @brief Kind limits, 24h idempotency, final states, retention, SQLite.
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "edge_app_task_sqlite.h"
#include "edgeos/app_task.h"
#include "sqlite_service.h"

#define MEM_TASKS 16

typedef struct {
    edge_app_task_t tasks[MEM_TASKS];
    size_t count;
    int fail;
} mem_store_t;

static edge_app_task_t *_find_id(mem_store_t *mem, const char *id) {
    for (size_t i = 0; i < mem->count; i++) {
        if (strcmp(mem->tasks[i].id, id) == 0) return &mem->tasks[i];
    }
    return NULL;
}

static edge_app_task_t *_find_key(mem_store_t *mem, const char *key) {
    for (size_t i = 0; i < mem->count; i++) {
        if (strcmp(mem->tasks[i].idempotency_key, key) == 0) return &mem->tasks[i];
    }
    return NULL;
}

static int _mem_insert(void *ctx, const edge_app_task_t *task) {
    mem_store_t *mem = (mem_store_t *)ctx;
    if (mem->fail) return -1;
    edge_app_task_t *slot = _find_id(mem, task->id);
    if (!slot) slot = _find_key(mem, task->idempotency_key);
    if (!slot) {
        if (mem->count >= MEM_TASKS) return -1;
        slot = &mem->tasks[mem->count++];
    }
    *slot = *task;
    return 0;
}

static int _mem_get(void *ctx, const char *id, edge_app_task_t *out) {
    mem_store_t *mem = (mem_store_t *)ctx;
    if (mem->fail) return -1;
    edge_app_task_t *task = _find_id(mem, id);
    if (!task) return 0;
    *out = *task;
    return 1;
}

static int _mem_find_by_key(void *ctx, const char *key, edge_app_task_t *out) {
    mem_store_t *mem = (mem_store_t *)ctx;
    if (mem->fail) return -1;
    edge_app_task_t *task = _find_key(mem, key);
    if (!task) return 0;
    *out = *task;
    return 1;
}

static int _mem_update(void *ctx, const edge_app_task_t *task) {
    mem_store_t *mem = (mem_store_t *)ctx;
    if (mem->fail) return -1;
    edge_app_task_t *slot = _find_id(mem, task->id);
    if (!slot) return 1;
    *slot = *task;
    return 0;
}

static int _mem_purge(void *ctx, int64_t cutoff_ms, size_t *removed) {
    mem_store_t *mem = (mem_store_t *)ctx;
    if (mem->fail) return -1;
    size_t kept = 0;
    *removed = 0;
    for (size_t i = 0; i < mem->count; i++) {
        edge_app_task_t *task = &mem->tasks[i];
        int terminal =
            task->state == EDGE_APP_TASK_SUCCEEDED || task->state == EDGE_APP_TASK_FAILED;
        if (terminal && task->updated_at < cutoff_ms) {
            (*removed)++;
            continue;
        }
        mem->tasks[kept++] = *task;
    }
    mem->count = kept;
    return 0;
}

static edge_app_task_store_t _mem_port(mem_store_t *mem) {
    edge_app_task_store_t store = {
        .ctx = mem,
        .insert = _mem_insert,
        .get = _mem_get,
        .find_by_key = _mem_find_by_key,
        .update = _mem_update,
        .purge_expired = _mem_purge,
    };
    return store;
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

static void test_kind_and_key_validation(void **state) {
    (void)state;
    mem_store_t mem = {0};
    edge_app_task_store_t store = _mem_port(&mem);
    edge_app_task_t task;
    int replayed = -1;

    assert_int_equal(
        edge_app_task_submit(&store, (edge_app_task_kind_t)0, "k", 1000, &task, &replayed),
        EDGE_APP_TASK_INVALID);
    assert_int_equal(
        edge_app_task_submit(&store, (edge_app_task_kind_t)4, "k", 1000, &task, &replayed),
        EDGE_APP_TASK_INVALID);
    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_INSTALL, "", 1000, &task, &replayed),
        EDGE_APP_TASK_INVALID);

    char long_key[EDGE_APP_TASK_KEY_SIZE + 1];
    memset(long_key, 'k', EDGE_APP_TASK_KEY_SIZE);
    long_key[EDGE_APP_TASK_KEY_SIZE] = '\0';
    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_INSTALL, long_key, 1000, &task, &replayed),
        EDGE_APP_TASK_INVALID);
    assert_int_equal(mem.count, 0);

    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_INSTALL, "install-1", 1000, &task, &replayed),
        EDGE_APP_TASK_OK);
    assert_int_equal(replayed, 0);
    assert_int_equal(task.kind, EDGE_APP_TASK_INSTALL);
    assert_int_equal(task.state, EDGE_APP_TASK_PENDING);
    assert_int_equal(task.created_at, 1000);
    assert_int_equal(task.updated_at, 1000);
    assert_true(strlen(task.id) > 0);

    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_UPGRADE, "upgrade-1", 1000, &task, &replayed),
        EDGE_APP_TASK_OK);
    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_BACKUP, "backup-1", 1000, &task, &replayed),
        EDGE_APP_TASK_OK);
    assert_int_equal(mem.count, 3);
}

static void test_idempotent_replay_window(void **state) {
    (void)state;
    mem_store_t mem = {0};
    edge_app_task_store_t store = _mem_port(&mem);
    edge_app_task_t task;
    int replayed = 0;

    const int64_t created = 1000000;
    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_INSTALL, "idem", created, &task, &replayed),
        EDGE_APP_TASK_OK);
    char id[EDGE_APP_TASK_ID_SIZE];
    snprintf(id, sizeof(id), "%s", task.id);
    assert_int_equal(
        edge_app_task_finish(&store, id, EDGE_APP_TASK_SUCCEEDED, "{}", created + 1000),
        EDGE_APP_TASK_OK);

    // A replay returns the first outcome, not a new PENDING row.
    assert_int_equal(edge_app_task_submit(&store, EDGE_APP_TASK_INSTALL, "idem", created + 3600000,
                                          &task, &replayed),
                     EDGE_APP_TASK_OK);
    assert_int_equal(replayed, 1);
    assert_string_equal(task.id, id);
    assert_int_equal(task.state, EDGE_APP_TASK_SUCCEEDED);
    assert_string_equal(task.result_json, "{}");
    assert_int_equal(task.created_at, created);
    assert_int_equal(mem.count, 1);

    assert_int_equal(edge_app_task_submit(&store, EDGE_APP_TASK_INSTALL, "idem",
                                          created + EDGE_APP_TASK_IDEMPOTENCY_MS, &task, &replayed),
                     EDGE_APP_TASK_OK);
    assert_int_equal(replayed, 1);

    // One millisecond past the window the key may start a fresh task.
    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_INSTALL, "idem",
                             created + EDGE_APP_TASK_IDEMPOTENCY_MS + 1, &task, &replayed),
        EDGE_APP_TASK_OK);
    assert_int_equal(replayed, 0);
    assert_int_equal(task.state, EDGE_APP_TASK_PENDING);
    assert_int_equal(task.created_at, created + EDGE_APP_TASK_IDEMPOTENCY_MS + 1);
    assert_int_equal(mem.count, 1);
}

static void test_finish_rejects_terminal_transition(void **state) {
    (void)state;
    mem_store_t mem = {0};
    edge_app_task_store_t store = _mem_port(&mem);
    edge_app_task_t task;
    int replayed = 0;

    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_UPGRADE, "final", 1000, &task, &replayed),
        EDGE_APP_TASK_OK);
    assert_int_equal(edge_app_task_finish(&store, task.id, EDGE_APP_TASK_RUNNING, NULL, 1100),
                     EDGE_APP_TASK_OK);
    assert_int_equal(
        edge_app_task_finish(&store, task.id, EDGE_APP_TASK_SUCCEEDED, "{\"version\":2}", 1200),
        EDGE_APP_TASK_OK);

    assert_int_equal(edge_app_task_finish(&store, task.id, EDGE_APP_TASK_FAILED, NULL, 1300),
                     EDGE_APP_TASK_INVALID);
    assert_int_equal(edge_app_task_finish(&store, task.id, EDGE_APP_TASK_SUCCEEDED, NULL, 1300),
                     EDGE_APP_TASK_INVALID);

    edge_app_task_t stored;
    assert_int_equal(_mem_get(&mem, task.id, &stored), 1);
    assert_int_equal(stored.state, EDGE_APP_TASK_SUCCEEDED);
    assert_int_equal(stored.updated_at, 1200);
    assert_string_equal(stored.result_json, "{\"version\":2}");

    // Finish cannot move a task back to PENDING, and unknown ids are reported.
    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_BACKUP, "pending", 1000, &task, &replayed),
        EDGE_APP_TASK_OK);
    assert_int_equal(edge_app_task_finish(&store, task.id, EDGE_APP_TASK_PENDING, NULL, 1100),
                     EDGE_APP_TASK_INVALID);
    assert_int_equal(
        edge_app_task_finish(&store, "task-missing", EDGE_APP_TASK_RUNNING, NULL, 1100),
        EDGE_APP_TASK_NOT_FOUND);

    char oversized[EDGE_APP_TASK_RESULT_SIZE + 1];
    memset(oversized, 'x', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = '\0';
    assert_int_equal(edge_app_task_finish(&store, task.id, EDGE_APP_TASK_RUNNING, oversized, 1100),
                     EDGE_APP_TASK_INVALID);
    assert_int_equal(_mem_get(&mem, task.id, &stored), 1);
    assert_int_equal(stored.state, EDGE_APP_TASK_PENDING);
}

static void test_purge_removes_only_old_terminal_tasks(void **state) {
    (void)state;
    mem_store_t mem = {0};
    edge_app_task_store_t store = _mem_port(&mem);
    edge_app_task_t task;
    int replayed = 0;
    const int64_t t0 = 10000000;

    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_INSTALL, "done", t0, &task, &replayed),
        EDGE_APP_TASK_OK);
    char done_id[EDGE_APP_TASK_ID_SIZE];
    snprintf(done_id, sizeof(done_id), "%s", task.id);
    assert_int_equal(
        edge_app_task_finish(&store, done_id, EDGE_APP_TASK_SUCCEEDED, NULL, t0 + 1000),
        EDGE_APP_TASK_OK);

    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_UPGRADE, "still-running", t0, &task, &replayed),
        EDGE_APP_TASK_OK);

    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_BACKUP, "failed", t0 + 5000, &task, &replayed),
        EDGE_APP_TASK_OK);
    char failed_id[EDGE_APP_TASK_ID_SIZE];
    snprintf(failed_id, sizeof(failed_id), "%s", task.id);
    assert_int_equal(edge_app_task_finish(&store, failed_id, EDGE_APP_TASK_FAILED, NULL, t0 + 6000),
                     EDGE_APP_TASK_OK);

    assert_int_equal(edge_app_task_purge(&store, t0 + EDGE_APP_TASK_RETENTION_MS),
                     EDGE_APP_TASK_OK);
    assert_int_equal(mem.count, 3);

    assert_int_equal(edge_app_task_purge(&store, t0 + EDGE_APP_TASK_RETENTION_MS + 1001),
                     EDGE_APP_TASK_OK);
    assert_int_equal(mem.count, 2);
    edge_app_task_t stored;
    assert_int_equal(_mem_get(&mem, done_id, &stored), 0);
    assert_int_equal(_mem_find_by_key(&mem, "still-running", &stored), 1);
    assert_int_equal(_mem_find_by_key(&mem, "failed", &stored), 1);

    assert_int_equal(edge_app_task_purge(&store, t0 + EDGE_APP_TASK_RETENTION_MS + 6001),
                     EDGE_APP_TASK_OK);
    assert_int_equal(mem.count, 1);
    assert_int_equal(_mem_find_by_key(&mem, "failed", &stored), 0);
    assert_int_equal(_mem_find_by_key(&mem, "still-running", &stored), 1);
}

static void test_sqlite_roundtrip(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_app_task.db";
    unlink(path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    edge_app_task_store_t store = edge_app_task_sqlite(svc);

    edge_app_task_t task;
    int replayed = 0;
    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_INSTALL, "sqlite-key", 5000, &task, &replayed),
        EDGE_APP_TASK_OK);
    char id[EDGE_APP_TASK_ID_SIZE];
    snprintf(id, sizeof(id), "%s", task.id);
    assert_int_equal(edge_app_task_finish(&store, id, EDGE_APP_TASK_RUNNING, NULL, 6000),
                     EDGE_APP_TASK_OK);
    assert_int_equal(
        edge_app_task_finish(&store, id, EDGE_APP_TASK_SUCCEEDED, "{\"installed\":true}", 7000),
        EDGE_APP_TASK_OK);

    sqlite_svc_destroy(svc);
    svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    store = edge_app_task_sqlite(svc);

    edge_app_task_t loaded;
    assert_int_equal(store.get(store.ctx, id, &loaded), 1);
    assert_int_equal(loaded.kind, EDGE_APP_TASK_INSTALL);
    assert_int_equal(loaded.state, EDGE_APP_TASK_SUCCEEDED);
    assert_int_equal(loaded.created_at, 5000);
    assert_int_equal(loaded.updated_at, 7000);
    assert_string_equal(loaded.idempotency_key, "sqlite-key");
    assert_string_equal(loaded.result_json, "{\"installed\":true}");

    assert_int_equal(store.find_by_key(store.ctx, "sqlite-key", &loaded), 1);
    assert_string_equal(loaded.id, id);

    assert_int_equal(
        edge_app_task_submit(&store, EDGE_APP_TASK_INSTALL, "sqlite-key", 7100, &task, &replayed),
        EDGE_APP_TASK_OK);
    assert_int_equal(replayed, 1);
    assert_string_equal(task.id, id);

    assert_int_equal(edge_app_task_purge(&store, 7000 + EDGE_APP_TASK_RETENTION_MS + 1),
                     EDGE_APP_TASK_OK);
    assert_int_equal(store.get(store.ctx, id, &loaded), 0);

    sqlite_svc_destroy(svc);
    unlink(path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_kind_and_key_validation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_idempotent_replay_window, setup, teardown),
        cmocka_unit_test_setup_teardown(test_finish_rejects_terminal_transition, setup, teardown),
        cmocka_unit_test_setup_teardown(test_purge_removes_only_old_terminal_tasks, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_sqlite_roundtrip, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
