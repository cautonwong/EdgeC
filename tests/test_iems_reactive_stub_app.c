/**
 * @file test_iems_reactive_stub_app.c
 * @brief P2-03b: PF strategy app publish wiring over the real product Host ABI.
 *
 * The app under test is tests/fixtures/ems_reactive_stub_app.c, a native
 * stand-in for the wasm package because this environment has no wasm toolchain.
 * It is spawned in place of app-host by the product runtime, so the lifecycle,
 * tick delivery, settings, authz, PCC reads and DataHub writes all go through
 * the real product path. The pure PF state machine is exercised at its exact
 * boundaries in test_iems_pf_decision; this file checks the app wiring: the
 * four PCC inputs, the eight PF settings, the three own points, the per-tick
 * host log, the enable gate and the denied-publish read-only degrade.
 */

#include <cmocka.h>
#include <math.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "app_points_sqlite.h"
#include "app_repo_sqlite.h"
#include "app_settings_sqlite.h"
#include "datahub_priv.h"
#include "edge_app_host.h"
#include "edge_app_mgr.h"
#include "edge_app_sdk.h"
#include "edge_authz_sqlite.h"
#include "edge_time_trust.h"
#include "edgeos/edge_authz.h"
#include "iems_app_host_ports.h"
#include "iems_app_manager_adapter.h"
#include "iems_app_runtime.h"
#include "pipe_msg.h"
#include "rest_control_port.h"
#include "sqlite_service.h"

#define APP_ID "ems"
#define OTHER_APP_ID "other"
#define APP_BASE 131072u
#define PCC_P_TAG 1001u
#define PCC_Q_TAG 1002u
#define PCC_PF_TAG 1003u
#define PCC_ONLINE_TAG 1004u
#define MANIFEST_PATH EDGEC_REACTIVE_STUB_MANIFEST

static const edge_tag_meta_t kProductTags[] = {
    {.id = PCC_P_TAG, .name = "pcc_p_kw", .type = "number", .unit = "kW"},
    {.id = PCC_Q_TAG, .name = "pcc_q_kvar", .type = "number", .unit = "kvar"},
    {.id = PCC_PF_TAG, .name = "pcc_pf", .type = "number"},
    {.id = PCC_ONLINE_TAG, .name = "pcc_online", .type = "number"},
};

static const uint8_t kStubWasm[8] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};

typedef struct {
    uv_loop_t loop;
    sqlite_svc_t *db;
    datahub_t *hub;
    edge_app_mgr_t *mgr;
    void *mgr_mem;
    rest_control_port_t control;
    iems_control_ctx_t control_ctx;
    iems_app_runtime_t *runtime;
    char db_path[128];
    char apps_dir[128];
    char shm_name[64];
    char shm_path[96];
} scenario_t;

static void close_handle(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static void trust_clock(sqlite_svc_t *db) {
    char reason[EDGE_TIME_TRUST_REASON_SIZE];
    /* WSL wall time can step back slightly; floor 5 s in the past. */
    uint64_t wall = edge_time_now_wall_ms();
    assert_true(wall > 5000);
    assert_int_equal(edge_time_trust_mark(db, wall - 5000, edge_time_now_monotonic_ms(), reason,
                                          sizeof(reason)),
                     0);
}

static void grant_app(sqlite_svc_t *db, const char *app_id, const char *capability,
                      const char *scope_json) {
    edge_authz_store_t store = edge_authz_sqlite_store(db);
    edge_app_grant_t grant = {0};
    snprintf(grant.app_id, sizeof(grant.app_id), "%s", app_id);
    snprintf(grant.capability, sizeof(grant.capability), "%s", capability);
    snprintf(grant.scope_json, sizeof(grant.scope_json), "%s", scope_json);
    snprintf(grant.purpose, sizeof(grant.purpose), "test");
    assert_int_equal(edge_authz_approve_grant(&store, &grant, 1000), EDGE_AUTHZ_OK);
}

static char *read_text_file(const char *path) {
    FILE *file = fopen(path, "rb");
    assert_non_null(file);
    assert_int_equal(fseek(file, 0, SEEK_END), 0);
    long size = ftell(file);
    assert_true(size > 0);
    rewind(file);
    char *text = malloc((size_t)size + 1);
    assert_non_null(text);
    assert_int_equal(fread(text, 1, (size_t)size, file), (size_t)size);
    fclose(file);
    text[size] = '\0';
    return text;
}

/* Resolve app.<owner>.<name> through the product point table, then read the
 * app slot. A zeroed shadow slot is "never published": return 0 only when the
 * stored message belongs to this handle and carries a publish timestamp. */
static int read_app_point(datahub_t *hub, sqlite_svc_t *db, const char *app_id, const char *name,
                          pipe_msg_t *out) {
    app_point_row_t rows[8];
    size_t count = 0;
    char reason[160];
    if (app_points_list(db, app_id, rows, 8, &count, reason, sizeof(reason)) != 0) return -1;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(rows[i].name, name) != 0) continue;
        uint32_t handle = APP_BASE + (uint32_t)rows[i].slot;
        memset(out, 0, sizeof(*out));
        if (datahub_read_shadow(hub, handle, out) != 0) return -1;
        if (out->ts == 0 || out->tag_id != handle) return -1;
        return 0;
    }
    return -1;
}

static void seed_pcc(datahub_t *hub, double p_kw, double q_kvar, double pf, double online) {
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, PCC_P_TAG, p_kw, 1720000000000ULL);
    assert_int_equal(datahub_write_shadow(hub, &msg), 0);
    pipe_msg_init_scalar(&msg, PCC_Q_TAG, q_kvar, 1720000000000ULL);
    assert_int_equal(datahub_write_shadow(hub, &msg), 0);
    pipe_msg_init_scalar(&msg, PCC_PF_TAG, pf, 1720000000000ULL);
    assert_int_equal(datahub_write_shadow(hub, &msg), 0);
    pipe_msg_init_scalar(&msg, PCC_ONLINE_TAG, online, 1720000000000ULL);
    assert_int_equal(datahub_write_shadow(hub, &msg), 0);
}

/* The app logs through the host to stdout; capture it to assert on the
 * observable per-tick lines. */
typedef struct {
    int saved_fd;
    FILE *file;
} stdout_capture_t;

static void capture_stdout(stdout_capture_t *cap) {
    fflush(stdout);
    cap->saved_fd = dup(STDOUT_FILENO);
    assert_true(cap->saved_fd >= 0);
    cap->file = tmpfile();
    assert_non_null(cap->file);
    assert_int_equal(dup2(fileno(cap->file), STDOUT_FILENO), STDOUT_FILENO);
}

static char *restore_stdout(stdout_capture_t *cap) {
    fflush(stdout);
    assert_int_equal(dup2(cap->saved_fd, STDOUT_FILENO), STDOUT_FILENO);
    close(cap->saved_fd);
    rewind(cap->file);
    char *buf = calloc(1, 32768);
    assert_non_null(buf);
    size_t n = fread(buf, 1, 32767, cap->file);
    buf[n] = '\0';
    fclose(cap->file);
    return buf;
}

static int count_substr(const char *haystack, const char *needle) {
    int count = 0;
    size_t len = strlen(needle);
    for (const char *p = strstr(haystack, needle); p; p = strstr(p + len, needle)) count++;
    return count;
}

static void assert_setting_number(sqlite_svc_t *db, const char *key, double want) {
    char value[64] = "";
    char reason[64] = "";
    assert_int_equal(
        app_settings_value_get(db, APP_ID, key, value, sizeof(value), reason, sizeof(reason)), 1);
    char *end = NULL;
    double got = strtod(value, &end);
    assert_true(end != value);
    assert_true(fabs(got - want) < 1e-9);
}

static void assert_setting_text(sqlite_svc_t *db, const char *key, const char *want) {
    char value[64] = "";
    char reason[64] = "";
    assert_int_equal(
        app_settings_value_get(db, APP_ID, key, value, sizeof(value), reason, sizeof(reason)), 1);
    assert_string_equal(value, want);
}

static void scenario_start(scenario_t *s, int grant_publish) {
    memset(s, 0, sizeof(*s));
    assert_int_equal(uv_loop_init(&s->loop), 0);

    snprintf(s->db_path, sizeof(s->db_path), "/tmp/iems_reactive_stub_%d.db", (int)getpid());
    unlink(s->db_path);
    sqlite_config_t sqlite_cfg = {.strategy = STRATEGY_LITE, .db_path = s->db_path};
    s->db = sqlite_svc_create(&s->loop, &sqlite_cfg);
    assert_non_null(s->db);
    trust_clock(s->db);

    size_t mgr_size = edge_app_mgr_storage_size();
    s->mgr_mem = malloc(mgr_size);
    assert_non_null(s->mgr_mem);
    edge_app_repo_t repo = app_repo_sqlite(s->db);
    s->mgr = edge_app_mgr_create_with_repo_in(s->mgr_mem, mgr_size, &s->loop, &repo, NULL);
    assert_non_null(s->mgr);
    snprintf(s->apps_dir, sizeof(s->apps_dir), "/tmp/iems_reactive_stub_apps_%d", (int)getpid());
    edge_app_mgr_set_apps_dir(s->mgr, s->apps_dir);

    s->hub = calloc(1, sizeof(datahub_t));
    assert_non_null(s->hub);
    snprintf(s->shm_name, sizeof(s->shm_name), "iems_reactive_stub_%d", (int)getpid());
    snprintf(s->shm_path, sizeof(s->shm_path), "/edgeos_shadow_%s", s->shm_name);
    shm_unlink(s->shm_path);
    assert_int_equal(datahub_init(s->hub, &s->loop, s->shm_name, 64, APP_BASE + 4096, NULL, 0,
                                  NULL, NULL, NULL),
                     0);
    seed_pcc(s->hub, 220.5, 0.0, 1.0, 1.0);

    iems_control_port_init(&s->control, &s->control_ctx, s->mgr, NULL, s->db);

    /* Direct spawn: this container has no launcher isolation. */
    setenv("EDGEC_LAUNCHER", "0", 1);
    const char *stub_app = getenv("EDGEC_REACTIVE_STUB_APP");
    assert_non_null(stub_app);
    iems_app_runtime_config_t runtime_cfg = {
        .hub = s->hub,
        .db = s->db,
        .tags = kProductTags,
        .tag_count = 4,
        .app_base = APP_BASE,
        .app_host_path = stub_app,
    };
    s->runtime = iems_app_runtime_create(&s->loop, &runtime_cfg);
    assert_non_null(s->runtime);
    iems_control_set_app_runtime(&s->control_ctx, s->runtime);

    grant_app(s->db, APP_ID, "tags.read",
              "[\"pcc_p_kw\",\"pcc_q_kvar\",\"pcc_pf\",\"pcc_online\"]");
    if (grant_publish) {
        grant_app(s->db, APP_ID, "points.publish",
                  "[\"app.ems.q_target_kvar\",\"app.ems.strategy_state\","
                  "\"app.ems.strategy_ts_ms\"]");
    }
}

static void scenario_stop(scenario_t *s) {
    if (s->runtime) iems_app_runtime_destroy(s->runtime);
    if (s->mgr) edge_app_mgr_destroy(s->mgr);
    free(s->mgr_mem);
    if (s->db) sqlite_svc_destroy(s->db);
    if (s->hub) {
        datahub_destroy(s->hub);
        free(s->hub);
    }
    unlink(s->db_path);
    char wasm_path[256];
    snprintf(wasm_path, sizeof(wasm_path), "%s/%s/app.wasm", s->apps_dir, APP_ID);
    unlink(wasm_path);
    char app_dir[256];
    snprintf(app_dir, sizeof(app_dir), "%s/%s", s->apps_dir, APP_ID);
    rmdir(app_dir);
    rmdir(s->apps_dir);
    uv_walk(&s->loop, close_handle, NULL);
    for (int i = 0; i < 100; i++) uv_run(&s->loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&s->loop), 0);
}

/* cmocka teardown runs even after a failed assertion, so a red test still
 * stops its app child and closes the loop instead of leaking SIGCHLD state. */
static scenario_t g_scenario;

static int setup_with_publish(void **state) {
    scenario_start(&g_scenario, 1);
    *state = &g_scenario;
    return 0;
}

static int setup_read_only(void **state) {
    scenario_start(&g_scenario, 0);
    *state = &g_scenario;
    return 0;
}

static int teardown_scenario(void **state) {
    (void)state;
    scenario_stop(&g_scenario);
    return 0;
}

static void drain_until(uv_loop_t *loop, int (*ready)(void *), void *ctx, int timeout_ms) {
    for (int waited = 0; waited < timeout_ms; waited += 10) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (ready(ctx)) return;
        usleep(10000);
    }
}

typedef struct {
    iems_app_runtime_t *runtime;
} running_probe_t;

static int stub_is_running(void *ctx) {
    running_probe_t *probe = (running_probe_t *)ctx;
    return iems_app_runtime_is_running(probe->runtime, APP_ID) == 1;
}

/* tick_ms is 1000, so three acked ticks prove the disabled path read and
 * logged the PCC sample at least three times. */
static int stub_ticked(void *ctx) {
    running_probe_t *probe = (running_probe_t *)ctx;
    uint64_t enqueued = 0;
    uint64_t skipped = 0;
    if (iems_app_runtime_tick_counters(probe->runtime, APP_ID, &enqueued, &skipped) != 0) return 0;
    return enqueued >= 3;
}

typedef struct {
    iems_app_runtime_t *runtime;
    uint64_t base;
} tick_probe_t;

static int ticks_grew(void *ctx) {
    tick_probe_t *probe = (tick_probe_t *)ctx;
    uint64_t enqueued = 0;
    uint64_t skipped = 0;
    if (iems_app_runtime_tick_counters(probe->runtime, APP_ID, &enqueued, &skipped) != 0) return 0;
    return enqueued >= probe->base + 2;
}

typedef struct {
    datahub_t *hub;
    sqlite_svc_t *db;
    const char *name;
} point_probe_t;

static int point_published(void *ctx) {
    point_probe_t *probe = (point_probe_t *)ctx;
    pipe_msg_t msg;
    return read_app_point(probe->hub, probe->db, APP_ID, probe->name, &msg) == 0;
}

typedef struct {
    datahub_t *hub;
    sqlite_svc_t *db;
    const char *name;
    uint64_t base_ts;
} point_ts_probe_t;

static int point_ts_advanced(void *ctx) {
    point_ts_probe_t *probe = (point_ts_probe_t *)ctx;
    pipe_msg_t msg;
    if (read_app_point(probe->hub, probe->db, APP_ID, probe->name, &msg) != 0) return 0;
    return msg.ts > probe->base_ts;
}

typedef struct {
    datahub_t *hub;
    sqlite_svc_t *db;
    const char *name;
    double want;
    double tolerance;
} point_value_probe_t;

static int point_value_ready(void *ctx) {
    point_value_probe_t *probe = (point_value_probe_t *)ctx;
    pipe_msg_t msg;
    if (read_app_point(probe->hub, probe->db, APP_ID, probe->name, &msg) != 0) return 0;
    return fabs(msg.payload.val.f - probe->want) <= probe->tolerance;
}

/* Shadow write time and published value of one app point, for cadence checks. */
static int read_point_ts(datahub_t *hub, sqlite_svc_t *db, const char *name, uint64_t *ts,
                         double *value) {
    pipe_msg_t msg;
    if (read_app_point(hub, db, APP_ID, name, &msg) != 0) return -1;
    if (ts) *ts = msg.ts;
    if (value) *value = msg.payload.val.f;
    return 0;
}

/* Count a needle in the output captured so far. edge_log_out flushes each log
 * line, and pread leaves the stream's write position alone. */
static int captured_count(stdout_capture_t *cap, const char *needle) {
    fflush(stdout);
    char buf[32768];
    ssize_t n = pread(fileno(cap->file), buf, sizeof(buf) - 1, 0);
    if (n < 0) return -1;
    buf[n] = '\0';
    return count_substr(buf, needle);
}

typedef struct {
    iems_app_runtime_t *runtime;
    uint64_t want;
} tick_watermark_t;

static int ticks_reached(void *ctx) {
    tick_watermark_t *watermark = (tick_watermark_t *)ctx;
    uint64_t enqueued = 0;
    uint64_t skipped = 0;
    if (iems_app_runtime_tick_counters(watermark->runtime, APP_ID, &enqueued, &skipped) != 0)
        return 0;
    return enqueued >= watermark->want;
}

static void test_install_start_tick_stop(void **state) {
    scenario_t *s = (scenario_t *)*state;
    running_probe_t probe = {.runtime = s->runtime};

    char *manifest = read_text_file(MANIFEST_PATH);
    stdout_capture_t cap;
    capture_stdout(&cap);
    int install_rc = s->control.install_app(s->control.ctx, manifest, NULL, 0, NULL, kStubWasm,
                                           sizeof(kStubWasm), "p2-03b-install");
    free(manifest);
    drain_until(&s->loop, stub_is_running, &probe, 5000);
    drain_until(&s->loop, stub_ticked, &probe, 15000);
    uint64_t enqueued = 0;
    uint64_t skipped = 0;
    int counts_rc = iems_app_runtime_tick_counters(s->runtime, APP_ID, &enqueued, &skipped);
    char *startup_logs = restore_stdout(&cap);

    assert_int_equal(install_rc, 0);
    assert_int_equal(iems_app_runtime_is_running(s->runtime, APP_ID), 1);
    assert_non_null(strstr(startup_logs, "ems.pf: control ready"));
    /* A3: disabled ticks still read the PCC sample once each; the enable
     * setting gates publishing only. The third tick may still be in flight
     * when its period is counted, so two completed reads are required. */
    assert_true(count_substr(startup_logs, "ems.pf: p=220.500 q=0.000 pf=1.000 online=1") >= 2);
    free(startup_logs);

    app_manifest_t installed;
    assert_int_equal(edge_app_mgr_get_app(s->mgr, APP_ID, &installed), 0);
    assert_int_equal(installed.state, APP_STATE_RUNNING);
    /* A5: the installed manifest keeps the declared 1 s tick period. */
    assert_int_equal(installed.tick_ms, 1000);

    /* A5: exactly the three own points, with numeric state. */
    app_point_row_t rows[8];
    size_t point_count = 0;
    char reason[160];
    assert_int_equal(app_points_list(s->db, APP_ID, rows, 8, &point_count, reason, sizeof(reason)),
                     0);
    assert_int_equal(point_count, 3);
    assert_string_equal(rows[0].name, "q_target_kvar");
    assert_int_equal(rows[0].type, APP_POINT_NUMBER);
    assert_string_equal(rows[1].name, "strategy_state");
    assert_int_equal(rows[1].type, APP_POINT_INTEGER);
    assert_string_equal(rows[2].name, "strategy_ts_ms");
    assert_int_equal(rows[2].type, APP_POINT_INTEGER);

    /* A5: the manifest's settings defaults, read back from the product store
     * rather than from the app. */
    assert_setting_text(s->db, "enable", "false");
    assert_setting_number(s->db, "en_pf_ctrl", 0.0);
    assert_setting_number(s->db, "reactive_mode", 0.0);
    assert_setting_number(s->db, "pf_target", 0.97);
    assert_setting_number(s->db, "pf_deadband", 0.02);
    assert_setting_number(s->db, "p_min_kw", 2.5);
    assert_setting_number(s->db, "start_delay_s", 2.0);
    assert_setting_number(s->db, "hold_s", 3.0);
    assert_setting_number(s->db, "control_cycle_s", 2.0);

    /* A3: enable defaults false; the app acked three ticks and published nothing. */
    assert_int_equal(counts_rc, 0);
    assert_true(enqueued >= 3);

    pipe_msg_t before_enable;
    assert_int_not_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &before_enable), 0);
    assert_int_not_equal(read_app_point(s->hub, s->db, APP_ID, "strategy_state", &before_enable),
                         0);

    capture_stdout(&cap);
    int enable_rc = s->control.settings_set(s->control.ctx, APP_ID, "enable", "true", reason,
                                            sizeof(reason));
    /* The tick publishes q_target/state before strategy_ts_ms; waiting for the
     * last point proves all three publishes of a tick completed. */
    point_probe_t ts_probe = {.hub = s->hub, .db = s->db, .name = "strategy_ts_ms"};
    drain_until(&s->loop, point_published, &ts_probe, 5000);
    pipe_msg_t first_tick;
    int first_rc = read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &first_tick);
    pipe_msg_t first_state;
    int first_state_rc = read_app_point(s->hub, s->db, APP_ID, "strategy_state", &first_state);
    /* The default en_pf_ctrl=0 keeps the decision DISABLED, so the app still
     * publishes every enabled tick: q=0 and state=0. */
    point_ts_probe_t ts_advanced = {
        .hub = s->hub, .db = s->db, .name = "q_target_kvar", .base_ts = first_tick.ts};
    drain_until(&s->loop, point_ts_advanced, &ts_advanced, 15000);
    pipe_msg_t later_tick;
    int later_rc = read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &later_tick);
    char *tick_logs = restore_stdout(&cap);

    assert_int_equal(enable_rc, 0);
    assert_int_equal(first_rc, 0);
    assert_int_equal(first_state_rc, 0);
    assert_int_equal(later_rc, 0);
    assert_int_equal((int)llround(first_state.payload.val.f), 0);
    assert_true(later_tick.ts > first_tick.ts);
    assert_non_null(strstr(tick_logs, "ems.pf: p=220.500"));
    assert_non_null(strstr(tick_logs, "state=0 q_target=0.000 reason=DISABLED"));
    free(tick_logs);

    pipe_msg_t q_target = later_tick;
    assert_true(fabs(q_target.payload.val.f - 0.0) < 1e-9);

    pipe_msg_t ts_ms;
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "strategy_ts_ms", &ts_ms), 0);
    assert_true(ts_ms.payload.val.f > 0.0);

    /* The zero target lives on the app point; the PCC product input is untouched. */
    pipe_msg_t pcc_p;
    assert_int_equal(datahub_read_shadow(s->hub, PCC_P_TAG, &pcc_p), 0);
    assert_true(fabs(pcc_p.payload.val.f - 220.5) < 1e-6);

    /* A4: stop freezes publishing and leaves no live tick source. */
    assert_int_equal(s->control.stop_app(s->control.ctx, APP_ID), 0);
    for (int waited = 0; waited < 5000 && iems_app_runtime_is_running(s->runtime, APP_ID);
         waited += 10) {
        uv_run(&s->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    assert_int_equal(iems_app_runtime_is_running(s->runtime, APP_ID), 0);
    uint64_t stop_sent = 0;
    uint64_t stop_acked = 0;
    uint64_t stop_expired = 0;
    iems_app_runtime_stop_counters(s->runtime, &stop_sent, &stop_acked, &stop_expired);
    assert_int_equal(stop_sent, 1);
    assert_int_equal(stop_acked, 1);
    assert_int_equal(stop_expired, 0);

    uint64_t stopped_enqueued = 0;
    uint64_t stopped_skipped = 0;
    assert_int_not_equal(
        iems_app_runtime_tick_counters(s->runtime, APP_ID, &stopped_enqueued, &stopped_skipped), 0);

    pipe_msg_t after_stop;
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &after_stop), 0);
    for (int i = 0; i < 30; i++) {
        uv_run(&s->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    pipe_msg_t idle;
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &idle), 0);
    assert_int_equal(idle.ts, after_stop.ts);
    assert_true(fabs(idle.payload.val.f - after_stop.payload.val.f) < 1e-9);

    app_manifest_t stopped;
    assert_int_equal(edge_app_mgr_get_app(s->mgr, APP_ID, &stopped), 0);
    assert_int_equal(stopped.state, APP_STATE_STOPPED);
}

/* A1/A2/A4 at the app layer: settings injection, the four PCC reads, the
 * golden Q target, the delayed STANDBY -> RUNNING transition, meter fault and
 * recovery, and the per-tick host log. Expected values come from the ticket
 * formula and v3 strategy_pf_ctrl.c:163-193, not from a run of the app. */
static void test_pf_golden_and_state_sequence(void **state) {
    scenario_t *s = (scenario_t *)*state;
    running_probe_t probe = {.runtime = s->runtime};

    char *manifest = read_text_file(MANIFEST_PATH);
    assert_int_equal(s->control.install_app(s->control.ctx, manifest, NULL, 0, NULL, kStubWasm,
                                           sizeof(kStubWasm), "p2-03b-pf"),
                     0);
    free(manifest);
    drain_until(&s->loop, stub_is_running, &probe, 5000);
    drain_until(&s->loop, stub_ticked, &probe, 15000);

    /* A3: nothing is published while the app-level enable is false. */
    pipe_msg_t before;
    assert_int_not_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &before), 0);
    assert_int_not_equal(read_app_point(s->hub, s->db, APP_ID, "strategy_state", &before), 0);

    /* Configure the v3 PF settings while still disabled; pf_target 0.95 and
     * start_delay 2 s are the ticket's A1/A2 values. */
    char reason[160];
    static const char *const keys[] = {"en_pf_ctrl",   "reactive_mode", "pf_target", "pf_deadband",
                                       "p_min_kw",     "start_delay_s", "hold_s",    "control_cycle_s"};
    static const char *const values[] = {"1", "1", "0.95", "0.02", "2.5", "2", "3", "1"};
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        assert_int_equal(s->control.settings_set(s->control.ctx, APP_ID, keys[i], values[i], reason,
                                                 sizeof(reason)),
                         0);
    }
    /* Out-of-band PCC sample: P=100 kW, Q=40 kVAr, PF=0.9, meter online. */
    seed_pcc(s->hub, 100.0, 40.0, 0.9, 1.0);

    stdout_capture_t cap;
    capture_stdout(&cap);
    assert_int_equal(s->control.settings_set(s->control.ctx, APP_ID, "enable", "true", reason,
                                             sizeof(reason)),
                     0);

    /* The first enabled tick crosses DISABLED -> STANDBY: state 1, q 0. */
    point_probe_t state_probe = {.hub = s->hub, .db = s->db, .name = "strategy_state"};
    drain_until(&s->loop, point_published, &state_probe, 5000);
    pipe_msg_t first_state;
    pipe_msg_t first_q;
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "strategy_state", &first_state), 0);
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &first_q), 0);
    assert_int_equal((int)llround(first_state.payload.val.f), 1);
    assert_true(fabs(first_q.payload.val.f) < 1e-9);

    /* A1: qt = 100*tan(acos(0.95)) = 32.8684105; q_tol = min(0.5*qt, 0.30)
     * = 0.30; q_edge = 32.5684105; |Q|=40 > qt and elapsed >= 2 s ->
     * RUNNING with q_target = clip(40, +/-32.5684105) = 32.5684105. */
    point_value_probe_t running_probe = {
        .hub = s->hub, .db = s->db, .name = "strategy_state", .want = 2.0, .tolerance = 0.5};
    drain_until(&s->loop, point_value_ready, &running_probe, 15000);
    pipe_msg_t running_state;
    pipe_msg_t running_q;
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "strategy_state", &running_state), 0);
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &running_q), 0);
    assert_int_equal((int)llround(running_state.payload.val.f), 2);
    assert_true(fabs(running_q.payload.val.f - 32.56841051788632) < 1e-3);

    /* A2 hold: back inside the band, v3 keeps the PCS on its last command
     * (strategy_pf_ctrl.c:669-673). The app must publish that held value,
     * not 0. */
    seed_pcc(s->hub, 100.0, 10.0, 0.99, 1.0);
    point_value_probe_t in_band_probe = {
        .hub = s->hub, .db = s->db, .name = "strategy_state", .want = 1.0, .tolerance = 0.5};
    drain_until(&s->loop, point_value_ready, &in_band_probe, 15000);
    pipe_msg_t in_band_q;
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &in_band_q), 0);
    assert_true(fabs(in_band_q.payload.val.f - 32.56841051788632) < 1e-3);

    /* A2 waiting hold: out of band again starts the start delay, and the app
     * keeps publishing the held Q until it elapses. */
    seed_pcc(s->hub, 100.0, 60.0, 0.9, 1.0);
    point_value_probe_t waiting_probe = {
        .hub = s->hub, .db = s->db, .name = "strategy_state", .want = 2.0, .tolerance = 0.5};
    drain_until(&s->loop, point_value_ready, &waiting_probe, 15000);

    /* A4: meter offline -> FAULT + q 0; back online -> STANDBY. */
    seed_pcc(s->hub, 100.0, 40.0, 0.9, 0.0);
    point_value_probe_t fault_probe = {
        .hub = s->hub, .db = s->db, .name = "strategy_state", .want = 3.0, .tolerance = 0.5};
    drain_until(&s->loop, point_value_ready, &fault_probe, 15000);
    pipe_msg_t fault_q;
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &fault_q), 0);
    assert_true(fabs(fault_q.payload.val.f) < 1e-9);
    seed_pcc(s->hub, 100.0, 40.0, 0.9, 1.0);
    point_value_probe_t standby_probe = {
        .hub = s->hub, .db = s->db, .name = "strategy_state", .want = 1.0, .tolerance = 0.5};
    drain_until(&s->loop, point_value_ready, &standby_probe, 15000);
    /* Recovery emits its zero target on the app point, not only in the
     * module's decision. */
    pipe_msg_t recovered_q;
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &recovered_q), 0);
    assert_true(fabs(recovered_q.payload.val.f) < 1e-9);
    char *logs = restore_stdout(&cap);

    /* Every evaluated tick logs P/Q/PF, online, state, q_target and reason. */
    assert_non_null(strstr(logs, "p=100.000 q=40.000 pf=0.900 online=1 state=1 q_target=0.000 "
                                 "reason=ENABLED"));
    assert_non_null(strstr(logs, "reason=WAITING_START"));
    assert_non_null(strstr(logs, "state=2 q_target=32.568 reason=RUNNING"));
    assert_non_null(strstr(logs, "state=1 q_target=32.568 reason=IN_BAND"));
    assert_non_null(strstr(logs, "state=1 q_target=32.568 reason=WAITING_START"));
    assert_non_null(strstr(logs, "online=0 state=3 q_target=0.000 reason=METER_OFFLINE"));
    assert_non_null(strstr(logs, "state=1 q_target=0.000 reason=FAULT_RECOVERED"));
    free(logs);

    assert_int_equal(s->control.stop_app(s->control.ctx, APP_ID), 0);
    for (int waited = 0; waited < 5000 && iems_app_runtime_is_running(s->runtime, APP_ID);
         waited += 10) {
        uv_run(&s->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    assert_int_equal(iems_app_runtime_is_running(s->runtime, APP_ID), 0);
}

/* C1: with enable=true, en_pf_ctrl=0 and the manifest's default 2 s cycle, a
 * decision tick publishes and the tick between decisions only logs the PCC
 * sample. The expected cadence comes from the ticket/manifest default (2), not
 * from the app. */
static void test_control_cycle_cadence(void **state) {
    scenario_t *s = (scenario_t *)*state;
    running_probe_t probe = {.runtime = s->runtime};

    char *manifest = read_text_file(MANIFEST_PATH);
    assert_int_equal(s->control.install_app(s->control.ctx, manifest, NULL, 0, NULL, kStubWasm,
                                           sizeof(kStubWasm), "p2-03b-cadence"),
                     0);
    free(manifest);
    drain_until(&s->loop, stub_is_running, &probe, 5000);
    drain_until(&s->loop, stub_ticked, &probe, 15000);

    assert_setting_number(s->db, "control_cycle_s", 2.0);
    assert_setting_number(s->db, "en_pf_ctrl", 0.0);
    assert_setting_text(s->db, "enable", "false");

    /* Let two ticks consume this sample before the measured window so every
     * captured log line carries it. */
    seed_pcc(s->hub, 100.0, 40.0, 0.9, 1.0);
    uint64_t base = 0;
    uint64_t base_skipped = 0;
    assert_int_equal(iems_app_runtime_tick_counters(s->runtime, APP_ID, &base, &base_skipped), 0);
    tick_probe_t settle_probe = {.runtime = s->runtime, .base = base};
    drain_until(&s->loop, ticks_grew, &settle_probe, 15000);

    stdout_capture_t cap;
    capture_stdout(&cap);
    char reason[160];
    assert_int_equal(
        s->control.settings_set(s->control.ctx, APP_ID, "enable", "true", reason, sizeof(reason)),
        0);

    /* Decision tick: the enabled app publishes its three own points. */
    point_probe_t ts_publish = {.hub = s->hub, .db = s->db, .name = "strategy_ts_ms"};
    drain_until(&s->loop, point_published, &ts_publish, 5000);
    uint64_t ts1 = 0;
    double ts1_value = 0.0;
    assert_int_equal(read_point_ts(s->hub, s->db, "strategy_ts_ms", &ts1, &ts1_value), 0);
    uint64_t e1 = 0;
    uint64_t e1_skipped = 0;
    assert_int_equal(iems_app_runtime_tick_counters(s->runtime, APP_ID, &e1, &e1_skipped), 0);
    int log1 = captured_count(&cap, "ems.pf: p=");
    assert_true(log1 >= 1);
    pipe_msg_t first_q;
    assert_int_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &first_q), 0);
    assert_true(fabs(first_q.payload.val.f) < 1e-9);

    /* Skipped tick: its log line arrives, then the shadow must not move. */
    tick_watermark_t next_tick = {.runtime = s->runtime, .want = e1 + 1};
    drain_until(&s->loop, ticks_reached, &next_tick, 5000);
    for (int waited = 0; waited < 5000 && captured_count(&cap, "ems.pf: p=") < log1 + 1;
         waited += 10) {
        uv_run(&s->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    assert_true(captured_count(&cap, "ems.pf: p=") >= log1 + 1);
    /* Give any publish carried by that same tick time to reach the host. */
    for (int i = 0; i < 10; i++) {
        uv_run(&s->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    uint64_t ts_skipped = 0;
    double ts_skipped_value = 0.0;
    assert_int_equal(read_point_ts(s->hub, s->db, "strategy_ts_ms", &ts_skipped, &ts_skipped_value),
                     0);
    assert_int_equal(ts_skipped, ts1);
    assert_true(fabs(ts_skipped_value - ts1_value) < 0.5);

    /* Second decision tick: exactly two ticks after the first publish. */
    tick_watermark_t decision_tick = {.runtime = s->runtime, .want = e1 + 2};
    drain_until(&s->loop, ticks_reached, &decision_tick, 10000);
    point_ts_probe_t advanced = {
        .hub = s->hub, .db = s->db, .name = "strategy_ts_ms", .base_ts = ts1};
    drain_until(&s->loop, point_ts_advanced, &advanced, 5000);
    uint64_t ts2 = 0;
    double ts2_value = 0.0;
    assert_int_equal(read_point_ts(s->hub, s->db, "strategy_ts_ms", &ts2, &ts2_value), 0);
    uint64_t e2 = 0;
    uint64_t e2_skipped = 0;
    assert_int_equal(iems_app_runtime_tick_counters(s->runtime, APP_ID, &e2, &e2_skipped), 0);
    char *logs = restore_stdout(&cap);

    assert_true(ts2 > ts1);
    assert_true(ts2_value > ts1_value);
    assert_int_equal(e2 - e1, 2);

    /* Every logged tick read all four PCC inputs, the skipped tick included;
     * fewer full lines means a tick ran without its P/Q/PF/online reads. */
    int total = count_substr(logs, "ems.pf: p=");
    int sampled = count_substr(logs, "ems.pf: p=100.000 q=40.000 pf=0.900 online=1");
    assert_true(total >= 3);
    assert_int_equal(sampled, total);
    free(logs);

    assert_int_equal(s->control.stop_app(s->control.ctx, APP_ID), 0);
    for (int waited = 0; waited < 5000 && iems_app_runtime_is_running(s->runtime, APP_ID);
         waited += 10) {
        uv_run(&s->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    assert_int_equal(iems_app_runtime_is_running(s->runtime, APP_ID), 0);
}

/* C2: disabled ticks still read and evaluate; enable=false alone holds the
 * decision at DISABLED/0 even when the settings ask for PF control. */
static void test_disabled_ticks_evaluate_and_gate(void **state) {
    scenario_t *s = (scenario_t *)*state;
    running_probe_t probe = {.runtime = s->runtime};

    char *manifest = read_text_file(MANIFEST_PATH);
    assert_int_equal(s->control.install_app(s->control.ctx, manifest, NULL, 0, NULL, kStubWasm,
                                           sizeof(kStubWasm), "p2-03b-disabled"),
                     0);
    free(manifest);
    drain_until(&s->loop, stub_is_running, &probe, 5000);
    drain_until(&s->loop, stub_ticked, &probe, 15000);

    char reason[160];
    assert_int_equal(
        s->control.settings_set(s->control.ctx, APP_ID, "en_pf_ctrl", "1", reason, sizeof(reason)),
        0);
    assert_int_equal(s->control.settings_set(s->control.ctx, APP_ID, "reactive_mode", "1", reason,
                                             sizeof(reason)),
                     0);
    assert_setting_text(s->db, "enable", "false");

    stdout_capture_t cap;
    capture_stdout(&cap);
    uint64_t e0 = 0;
    uint64_t e0_skipped = 0;
    assert_int_equal(iems_app_runtime_tick_counters(s->runtime, APP_ID, &e0, &e0_skipped), 0);
    tick_watermark_t three_ticks = {.runtime = s->runtime, .want = e0 + 3};
    drain_until(&s->loop, ticks_reached, &three_ticks, 15000);
    uint64_t e1 = 0;
    uint64_t e1_skipped = 0;
    assert_int_equal(iems_app_runtime_tick_counters(s->runtime, APP_ID, &e1, &e1_skipped), 0);
    char *logs = restore_stdout(&cap);

    int total = count_substr(logs, "ems.pf: p=");
    int disabled = count_substr(logs, "state=0 q_target=0.000 reason=DISABLED");
    /* One log line per delivered tick; the last may still be in flight. */
    assert_true(e1 >= e0 + 3);
    assert_true(total >= (int)(e1 - e0) - 1);
    assert_true(disabled >= (int)(e1 - e0) - 1);
    free(logs);

    /* The gate, not the settings, holds the outputs: nothing is published. */
    pipe_msg_t unpublished;
    assert_int_not_equal(read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &unpublished), 0);
    assert_int_not_equal(read_app_point(s->hub, s->db, APP_ID, "strategy_state", &unpublished), 0);
    assert_int_not_equal(read_app_point(s->hub, s->db, APP_ID, "strategy_ts_ms", &unpublished), 0);

    /* A tick disabled after RUNNING must re-evaluate to DISABLED, not replay
     * the stale RUNNING decision. */
    seed_pcc(s->hub, 100.0, 40.0, 0.9, 1.0);
    capture_stdout(&cap);
    assert_int_equal(s->control.settings_set(s->control.ctx, APP_ID, "enable", "true", reason,
                                             sizeof(reason)),
                     0);
    point_value_probe_t running = {
        .hub = s->hub, .db = s->db, .name = "strategy_state", .want = 2.0, .tolerance = 0.5};
    drain_until(&s->loop, point_value_ready, &running, 20000);
    char *enabled_logs = restore_stdout(&cap);
    assert_non_null(strstr(enabled_logs, "reason=RUNNING"));
    free(enabled_logs);

    capture_stdout(&cap);
    uint64_t d0 = 0;
    uint64_t d0_skipped = 0;
    assert_int_equal(iems_app_runtime_tick_counters(s->runtime, APP_ID, &d0, &d0_skipped), 0);
    assert_int_equal(s->control.settings_set(s->control.ctx, APP_ID, "enable", "false", reason,
                                             sizeof(reason)),
                     0);
    tick_watermark_t next_ticks = {.runtime = s->runtime, .want = d0 + 3};
    drain_until(&s->loop, ticks_reached, &next_ticks, 15000);
    char *after_disable = restore_stdout(&cap);
    assert_non_null(strstr(after_disable, "state=0 q_target=0.000 reason=DISABLED"));
    free(after_disable);

    assert_int_equal(s->control.stop_app(s->control.ctx, APP_ID), 0);
    for (int waited = 0; waited < 5000 && iems_app_runtime_is_running(s->runtime, APP_ID);
         waited += 10) {
        uv_run(&s->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    assert_int_equal(iems_app_runtime_is_running(s->runtime, APP_ID), 0);
}

/* Publishing without the points.publish grant is denied; the app must keep
 * running (read-only degradation) and keep reading the PCC inputs. */
static void test_denied_publish_degrades_read_only(void **state) {
    scenario_t *s = (scenario_t *)*state;
    running_probe_t probe = {.runtime = s->runtime};

    char *manifest = read_text_file(MANIFEST_PATH);
    stdout_capture_t cap;
    capture_stdout(&cap);
    int install_rc = s->control.install_app(s->control.ctx, manifest, NULL, 0, NULL, kStubWasm,
                                           sizeof(kStubWasm), "p2-03b-denied");
    free(manifest);
    drain_until(&s->loop, stub_is_running, &probe, 5000);
    drain_until(&s->loop, stub_ticked, &probe, 15000);

    uint64_t enqueued = 0;
    uint64_t skipped = 0;
    int counts_rc = iems_app_runtime_tick_counters(s->runtime, APP_ID, &enqueued, &skipped);
    pipe_msg_t before_enable;
    int before_rc = read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &before_enable);

    char reason[160];
    assert_int_equal(
        s->control.settings_set(s->control.ctx, APP_ID, "en_pf_ctrl", "1", reason, sizeof(reason)),
        0);
    assert_int_equal(s->control.settings_set(s->control.ctx, APP_ID, "reactive_mode", "1", reason,
                                             sizeof(reason)),
                     0);
    /* One-tick cadence keeps the denial observable in this test; the default
     * 2 s cadence is pinned by the manifest assertion above. */
    assert_int_equal(s->control.settings_set(s->control.ctx, APP_ID, "control_cycle_s", "1", reason,
                                             sizeof(reason)),
                     0);
    /* Enable: the next ticks read the PCC sample and attempt the publish the
     * manifest declares but no points.publish grant covers. */
    int enable_rc = s->control.settings_set(s->control.ctx, APP_ID, "enable", "true", reason,
                                            sizeof(reason));
    tick_probe_t tick_probe = {.runtime = s->runtime, .base = enqueued};
    drain_until(&s->loop, ticks_grew, &tick_probe, 15000);
    /* Let the last queued ticks and their log frames reach the host. */
    for (int i = 0; i < 10; i++) {
        uv_run(&s->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }

    uint64_t after_enqueued = 0;
    uint64_t after_skipped = 0;
    int after_counts_rc = iems_app_runtime_tick_counters(s->runtime, APP_ID, &after_enqueued,
                                                         &after_skipped);
    int running_after = iems_app_runtime_is_running(s->runtime, APP_ID);
    pipe_msg_t q_target;
    int publish_rc = read_app_point(s->hub, s->db, APP_ID, "q_target_kvar", &q_target);
    char *logs = restore_stdout(&cap);

    assert_int_equal(install_rc, 0);
    assert_int_equal(counts_rc, 0);
    assert_true(enqueued >= 3);
    /* A3 during the disabled phase: nothing published yet. */
    assert_int_not_equal(before_rc, 0);
    assert_int_equal(enable_rc, 0);
    /* The denied publish did not stop the app: it kept acking ticks. */
    assert_int_equal(after_counts_rc, 0);
    assert_true(after_enqueued >= enqueued + 2);
    assert_int_equal(running_after, 1);

    app_manifest_t manifest_state;
    assert_int_equal(edge_app_mgr_get_app(s->mgr, APP_ID, &manifest_state), 0);
    assert_int_equal(manifest_state.state, APP_STATE_RUNNING);

    assert_int_not_equal(publish_rc, 0);
    /* The host rejected the publish with the missing-grant code; the app
     * degraded to read-only but kept reading the PCC sample. */
    const char *degrade = strstr(logs, "ems.pf: publish denied_not_granted, read-only");
    assert_non_null(degrade);
    assert_non_null(strstr(degrade, "ems.pf: p=220.500"));
    free(logs);

    assert_int_equal(s->control.stop_app(s->control.ctx, APP_ID), 0);
    for (int waited = 0; waited < 5000 && iems_app_runtime_is_running(s->runtime, APP_ID);
         waited += 10) {
        uv_run(&s->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    assert_int_equal(iems_app_runtime_is_running(s->runtime, APP_ID), 0);
}

static void child_write_isolation(int fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);

    uint32_t handle = 0;
    const char *access = NULL;
    const char *err = edge_app_point_resolve(sdk, "pcc_p_kw", &handle, &access);
    if (err || !access || handle != PCC_P_TAG || strcmp(access, "read") != 0) _exit(11);

    edge_app_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    sample.value.kind = EDGE_APP_VALUE_FLOAT;
    sample.value.number = 1.0;
    sample.quality = "good";

    /* Writing a product point is a scope violation, not a missing grant. */
    err = edge_app_point_publish(sdk, PCC_P_TAG, &sample);
    if (!err || strcmp(err, "denied_scope") != 0) _exit(12);

    /* Another app's point resolves read-only and cannot be written. */
    handle = 0;
    access = NULL;
    err = edge_app_point_resolve(sdk, "app.other.value", &handle, &access);
    if (err || !access || strcmp(access, "read") != 0) _exit(13);
    err = edge_app_point_publish(sdk, handle, &sample);
    if (!err || strcmp(err, "denied_scope") != 0) _exit(14);

    /* Positive control: the manifest's own point is writable. */
    handle = 0;
    access = NULL;
    err = edge_app_point_resolve(sdk, "app.ems.q_target_kvar", &handle, &access);
    if (err || !access || strcmp(access, "publish") != 0) _exit(15);
    err = edge_app_point_publish(sdk, handle, &sample);
    if (err) _exit(16);

    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void test_write_isolation(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    char db_path[128];
    snprintf(db_path, sizeof(db_path), "/tmp/iems_reactive_iso_%d.db", (int)getpid());
    unlink(db_path);
    sqlite_config_t sqlite_cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &sqlite_cfg);
    assert_non_null(db);

    sqlite3 *raw = (sqlite3 *)sqlite_svc_get_db_handle(db);
    sqlite3_stmt *stmt;
    assert_int_equal(sqlite3_prepare_v2(
                         raw,
                         "INSERT INTO app_registry(id, version, state) VALUES('ems','0.2.0',1);",
                         -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);
    assert_int_equal(sqlite3_prepare_v2(
                         raw,
                         "INSERT INTO app_registry(id, version, state) VALUES('other','1.0.0',1);",
                         -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);

    app_point_spec_t own[3];
    memset(own, 0, sizeof(own));
    snprintf(own[0].name, sizeof(own[0].name), "q_target_kvar");
    own[0].type = APP_POINT_NUMBER;
    own[0].contract_version = 1;
    snprintf(own[1].name, sizeof(own[1].name), "strategy_state");
    own[1].type = APP_POINT_INTEGER;
    own[1].contract_version = 1;
    snprintf(own[2].name, sizeof(own[2].name), "strategy_ts_ms");
    own[2].type = APP_POINT_INTEGER;
    own[2].contract_version = 1;
    char reason[160];
    assert_int_equal(app_points_sync(db, APP_ID, own, 3, reason, sizeof(reason)), 0);

    app_point_spec_t other;
    memset(&other, 0, sizeof(other));
    snprintf(other.name, sizeof(other.name), "value");
    other.type = APP_POINT_NUMBER;
    other.contract_version = 1;
    assert_int_equal(app_points_sync(db, OTHER_APP_ID, &other, 1, reason, sizeof(reason)), 0);

    /* Deliberately over-broad admin grant: the ownership check, not authz,
     * must still reject a write to another app's point. */
    grant_app(db, APP_ID, "points.publish",
              "[\"app.ems.q_target_kvar\",\"app.other.value\"]");

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    assert_non_null(hub);
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "iems_reactive_iso_%d", (int)getpid());
    char shm_path[96];
    snprintf(shm_path, sizeof(shm_path), "/edgeos_shadow_%s", shm_name);
    shm_unlink(shm_path);
    assert_int_equal(datahub_init(hub, &loop, shm_name, 64, APP_BASE + 4096, NULL, 0, NULL, NULL,
                                  NULL),
                     0);

    iems_app_host_ctx_t ctx = {
        .app_id = APP_ID,
        .hub = hub,
        .db = db,
        .product_tags = kProductTags,
        .product_tag_count = 4,
        .app_base = APP_BASE,
    };
    app_host_ports_t ports;
    iems_app_host_ports_init(&ports, &ctx);

    int fds[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    app_host_server_t *server = app_host_server_create(&loop, fds[0], &ports);
    assert_non_null(server);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        child_write_isolation(fds[1]);
    }
    close(fds[1]);

    int status = -1;
    for (int waited = 0; waited < 5000; waited += 10) {
        uv_run(&loop, UV_RUN_NOWAIT);
        if (waitpid(pid, &status, WNOHANG) == pid) break;
        usleep(10000);
    }
    if (status == -1) {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
    }
    assert_true(status != -1);
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), 0);

    app_host_server_destroy(server);
    for (int i = 0; i < 20; i++) uv_run(&loop, UV_RUN_NOWAIT);

    datahub_destroy(hub);
    free(hub);
    sqlite_svc_destroy(db);
    unlink(db_path);
    uv_walk(&loop, close_handle, NULL);
    for (int i = 0; i < 100; i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_install_start_tick_stop, setup_with_publish,
                                        teardown_scenario),
        cmocka_unit_test_setup_teardown(test_pf_golden_and_state_sequence, setup_with_publish,
                                        teardown_scenario),
        cmocka_unit_test_setup_teardown(test_control_cycle_cadence, setup_with_publish,
                                        teardown_scenario),
        cmocka_unit_test_setup_teardown(test_disabled_ticks_evaluate_and_gate, setup_with_publish,
                                        teardown_scenario),
        cmocka_unit_test_setup_teardown(test_denied_publish_degrades_read_only, setup_read_only,
                                        teardown_scenario),
        cmocka_unit_test(test_write_isolation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
