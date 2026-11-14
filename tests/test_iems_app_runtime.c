/**
 * @file test_iems_app_runtime.c
 * @brief Product path: control.start_app spawns app-host, probe publishes, stop reaps.
 */

#include <cmocka.h>
#include <math.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "app_points_sqlite.h"
#include "app_repo_sqlite.h"
#include "arbiter.h"
#include "datahub_priv.h"
#include "edge_service.h"
#include "edge_app_mgr.h"
#include "edge_authz_sqlite.h"
#include "edgeos/edge_authz.h"
#include "iems_app_manager_adapter.h"
#include "iems_app_runtime.h"
#include "pipe_msg.h"
#include "rest_control_port.h"
#include "sqlite_service.h"

#define APP_BASE 131072u

static const edge_tag_meta_t kTags[] = {
    {.id = 1001, .name = "U_A", .type = "number", .unit = "V"},
};

static int copy_file(const char *from, const char *to) {
    FILE *src = fopen(from, "rb");
    if (!src) return -1;
    FILE *dst = fopen(to, "wb");
    if (!dst) {
        fclose(src);
        return -1;
    }
    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, n, dst) != n) {
            fclose(src);
            fclose(dst);
            return -1;
        }
    }
    fclose(src);
    fclose(dst);
    return 0;
}

static void grant(sqlite_svc_t *db, const char *capability, const char *scope_json) {
    edge_authz_store_t store = edge_authz_sqlite_store(db);
    edge_app_grant_t grant = {0};
    snprintf(grant.app_id, sizeof(grant.app_id), "demo");
    snprintf(grant.capability, sizeof(grant.capability), "%s", capability);
    snprintf(grant.scope_json, sizeof(grant.scope_json), "%s", scope_json);
    snprintf(grant.purpose, sizeof(grant.purpose), "test");
    assert_int_equal(edge_authz_approve_grant(&store, &grant, 1000), EDGE_AUTHZ_OK);
}

static int wait_for_publish(uv_loop_t *loop, datahub_t *hub, int timeout_ms, double expected) {
    for (int waited = 0; waited < timeout_ms; waited += 10) {
        uv_run(loop, UV_RUN_NOWAIT);
        pipe_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        if (datahub_read_shadow(hub, APP_BASE, &msg) == 0 &&
            fabs(msg.payload.val.f - expected) < 1e-6) {
            return 0;
        }
        usleep(10000);
    }
    return -1;
}

static void on_app_fault(void *ctx, const char *app_id, int64_t status, int term_signal) {
    iems_control_app_faulted((iems_control_ctx_t *)ctx, app_id, status, term_signal);
}

static void on_app_restarted(void *ctx, const char *app_id) {
    iems_control_app_restarted((iems_control_ctx_t *)ctx, app_id);
}

static void close_handle(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static void test_control_start_spawns_app_host(void **state) {
    (void)state;
    const char *app_host = getenv("EDGEC_APP_HOST_PATH");
    const char *probe = getenv("EDGEC_ABI_PROBE_WASM");
    assert_non_null(app_host);
    assert_non_null(probe);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    char app_dir[] = "/tmp/edgeos_runtime_XXXXXX";
    assert_non_null(mkdtemp(app_dir));
    char wasm_path[256];
    snprintf(wasm_path, sizeof(wasm_path), "%s/app.wasm", app_dir);
    assert_int_equal(copy_file(probe, wasm_path), 0);

    const char *db_path = "test_iems_app_runtime.db";
    unlink(db_path);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &cfg);
    assert_non_null(db);

    sqlite3 *raw = (sqlite3 *)sqlite_svc_get_db_handle(db);
    sqlite3_stmt *stmt;
    assert_int_equal(sqlite3_prepare_v2(raw,
                                        "INSERT INTO app_registry(id, version, state, wasm_path, "
                                        "heap_limit_kb) VALUES('demo','1.0.0',1,?1,64);",
                                        -1, &stmt, NULL),
                     SQLITE_OK);
    sqlite3_bind_text(stmt, 1, wasm_path, -1, SQLITE_STATIC);
    assert_int_equal(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);

    app_point_spec_t point;
    memset(&point, 0, sizeof(point));
    snprintf(point.name, sizeof(point.name), "value");
    point.type = APP_POINT_NUMBER;
    point.contract_version = 1;
    char reason[160];
    assert_int_equal(app_points_sync(db, "demo", &point, 1, reason, sizeof(reason)), 0);
    grant(db, "tags.read", "[\"U_A\"]");
    grant(db, "points.publish", "[\"app.demo.value\"]");

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    assert_non_null(hub);
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "test_app_runtime_%d", (int)getpid());
    char shm_path[96];
    snprintf(shm_path, sizeof(shm_path), "/edgeos_shadow_%s", shm_name);
    shm_unlink(shm_path);
    assert_int_equal(datahub_init(hub, &loop, shm_name, 64, APP_BASE + 4096, NULL, 0, NULL, NULL,
                                  NULL),
                     0);
    pipe_msg_t seed;
    pipe_msg_init_scalar(&seed, 1001, 220.5, 1720000000000ULL);
    assert_int_equal(datahub_write_shadow(hub, &seed), 0);

    size_t mgr_size = edge_app_mgr_storage_size();
    void *mgr_mem = malloc(mgr_size);
    edge_app_repo_t repo = app_repo_sqlite(db);
    edge_app_mgr_t *mgr = edge_app_mgr_create_with_repo_in(mgr_mem, mgr_size, &loop, &repo, NULL);
    assert_non_null(mgr);

    service_t *svc = service_new(&loop, hub, (arbiter_t *)0x1234, NULL, NULL);
    assert_non_null(svc);
    iems_control_ctx_t control_ctx;
    rest_control_port_t control;
    iems_control_port_init(&control, &control_ctx, mgr, NULL, db);
    service_set_control_port(svc, &control);

    setenv("EDGEC_LAUNCHER", "0", 1);
    iems_app_runtime_config_t runtime_cfg = {
        .hub = hub,
        .db = db,
        .tags = kTags,
        .tag_count = 1,
        .app_base = APP_BASE,
        .app_host_path = app_host,
        .on_fault = on_app_fault,
        .on_fault_ctx = &control_ctx,
        .on_restarted = on_app_restarted,
        .on_restarted_ctx = &control_ctx,
    };
    iems_app_runtime_t *runtime = iems_app_runtime_create(&loop, &runtime_cfg);
    assert_non_null(runtime);
    iems_control_set_app_runtime(&control_ctx, runtime);

    assert_int_equal(control.start_app(control.ctx, "demo"), 0);
    for (int i = 0; i < 3000 && edge_app_mgr_is_busy(mgr); i++) { uv_run(&loop, UV_RUN_NOWAIT); usleep(1000); }
    assert_int_equal(iems_app_runtime_is_running(runtime, "demo"), 1);
    assert_int_equal(wait_for_publish(&loop, hub, 3000, 221.5), 0);

    assert_int_equal(control.stop_app(control.ctx, "demo"), 0);
    for (int i = 0; i < 3000 && edge_app_mgr_is_busy(mgr); i++) { uv_run(&loop, UV_RUN_NOWAIT); usleep(1000); }
    for (int i = 0; i < 1000 && iems_app_runtime_is_running(runtime, "demo"); i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    assert_int_equal(iems_app_runtime_is_running(runtime, "demo"), 0);

    app_manifest_t manifest;
    assert_int_equal(edge_app_mgr_get_app(mgr, "demo", &manifest), 0);
    assert_int_equal(manifest.state, APP_STATE_STOPPED);

    /* A crashing app-host is isolated, then restarted up to the cap. */
    setenv("EDGEC_APP_RESTART_DELAY_MS", "20", 1);
    assert_int_equal(control.start_app(control.ctx, "demo"), 0);
    for (int i = 0; i < 3000 && edge_app_mgr_is_busy(mgr); i++) { uv_run(&loop, UV_RUN_NOWAIT); usleep(1000); }
    assert_int_equal(wait_for_publish(&loop, hub, 3000, 221.5), 0);

    for (int attempt = 1; attempt <= IEMS_APP_RESTART_CAP + 1; attempt++) {
        int pid = iems_app_runtime_pid(runtime, "demo");
        assert_true(pid > 0);
        assert_int_equal(kill(pid, SIGKILL), 0);

        if (attempt <= IEMS_APP_RESTART_CAP) {
            int restarted = 0;
            for (int i = 0; i < 300; i++) {
                uv_run(&loop, UV_RUN_NOWAIT);
                int next_pid = iems_app_runtime_pid(runtime, "demo");
                if (next_pid > 0 && next_pid != pid &&
                    edge_app_mgr_get_app(mgr, "demo", &manifest) == 0 &&
                    manifest.state == APP_STATE_RUNNING &&
                    manifest.restart_count == attempt) {
                    restarted = 1;
                    break;
                }
                usleep(10000);
            }
            assert_int_equal(restarted, 1);
        } else {
            int isolated = 0;
            for (int i = 0; i < 300; i++) {
                uv_run(&loop, UV_RUN_NOWAIT);
                if (!iems_app_runtime_is_running(runtime, "demo") &&
                    edge_app_mgr_get_app(mgr, "demo", &manifest) == 0 &&
                    manifest.state == APP_STATE_FAULTED &&
                    manifest.restart_count == attempt) {
                    isolated = 1;
                    break;
                }
                usleep(10000);
            }
            assert_int_equal(isolated, 1);
        }
    }

    /* A fault after the window starts a new streak, not a continuation. */
    edge_app_mgr_set_fault_window_ms(mgr, 1);
    assert_int_equal(control.start_app(control.ctx, "demo"), 0);
    int running = 0;
    for (int i = 0; i < 300; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        if (iems_app_runtime_is_running(runtime, "demo")) {
            running = 1;
            break;
        }
        usleep(10000);
    }
    assert_int_equal(running, 1);

    usleep(5000);
    int pid = iems_app_runtime_pid(runtime, "demo");
    assert_true(pid > 0);
    assert_int_equal(kill(pid, SIGKILL), 0);
    int restarted = 0;
    for (int i = 0; i < 300; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        int next_pid = iems_app_runtime_pid(runtime, "demo");
        if (next_pid > 0 && next_pid != pid &&
            edge_app_mgr_get_app(mgr, "demo", &manifest) == 0 &&
            manifest.state == APP_STATE_RUNNING && manifest.restart_count == 1) {
            restarted = 1;
            break;
        }
        usleep(10000);
    }
    assert_int_equal(restarted, 1);

    iems_app_runtime_destroy(runtime);
    edge_app_mgr_destroy(mgr);
    free(mgr_mem);
    sqlite_svc_destroy(db);
    datahub_destroy(hub);
    free(hub);
    unlink(db_path);
    unlink(wasm_path);
    rmdir(app_dir);
    uv_walk(&loop, close_handle, NULL);
    for (int i = 0; i < 100; i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_control_start_spawns_app_host),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
