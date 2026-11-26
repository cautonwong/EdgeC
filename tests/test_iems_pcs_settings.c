/**
 * @file test_iems_pcs_settings.c
 * @brief P2-02a ems/lc 设置加载：golden 正例、三值 source、fail closed、无半加载、app getter。
 */

#include <cmocka.h>
#include <dirent.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <uv.h>

#include "iems_app.h"
#include "iems_config_loader.h"
#include "iems_pcs_settings.h"

#ifndef IEMS_CONFIG_GOLDEN_DIR
#error "IEMS_CONFIG_GOLDEN_DIR must point at tools/migration/golden"
#endif

_Static_assert(IEMS_EMS_SETTINGS_SCALAR_KEYS == 18, "ems schema has 18 retained scalar keys");
_Static_assert(IEMS_LC_SETTINGS_KEYS == 24, "lc schema object branch has 24 keys");

static char g_root[256];

static void remove_tree(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        unlink(path);
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char child[512];
        snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        struct stat st;
        if (lstat(child, &st) == 0 && S_ISDIR(st.st_mode)) {
            remove_tree(child);
        } else {
            unlink(child);
        }
    }
    closedir(dir);
    rmdir(path);
}

static int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }
    char buf[4096];
    size_t n;
    int rc = 0;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            rc = -1;
            break;
        }
    }
    fclose(in);
    fclose(out);
    return rc;
}

static int write_text(const char *path, const char *text) {
    FILE *out = fopen(path, "wb");
    if (!out) return -1;
    size_t len = strlen(text);
    int rc = fwrite(text, 1, len, out) == len ? 0 : -1;
    fclose(out);
    return rc;
}

static void golden_ems(char *path, size_t cap) {
    snprintf(path, cap, "%s/ems/ems_cfg.json", IEMS_CONFIG_GOLDEN_DIR);
}

static void golden_lc(char *path, size_t cap) {
    snprintf(path, cap, "%s/lc/lc/lc_cfg.json", IEMS_CONFIG_GOLDEN_DIR);
}

static int copy_golden_pcs(void) {
    char src[512];
    char dst[512];
    golden_ems(src, sizeof(src));
    snprintf(dst, sizeof(dst), "%s/ems_cfg.json", g_root);
    if (copy_file(src, dst) != 0) return -1;
    golden_lc(src, sizeof(src));
    snprintf(dst, sizeof(dst), "%s/lc/lc_cfg.json", g_root);
    return copy_file(src, dst);
}

static int copy_golden_device_port(void) {
    char dst[512];
    snprintf(dst, sizeof(dst), "%s/device_config.json", g_root);
    if (copy_file(IEMS_CONFIG_GOLDEN_DIR "/device/device_config.json", dst) != 0) return -1;
    snprintf(dst, sizeof(dst), "%s/port_config.json", g_root);
    return copy_file(IEMS_CONFIG_GOLDEN_DIR "/port/port_config.json", dst);
}

static int load_root(const char *root, iems_pcs_settings_t *out, char *err, size_t err_size) {
    iems_config_file_source_t fs;
    iems_config_file_source_init(&fs, root);
    iems_config_source_t source = iems_config_file_source_port(&fs);
    return iems_pcs_settings_load(&source, out, err, err_size);
}

static int setup(void **state) {
    (void)state;
    snprintf(g_root, sizeof(g_root), "/tmp/iems_pcs_settings_%d", (int)getpid());
    remove_tree(g_root);
    char lc[512];
    snprintf(lc, sizeof(lc), "%s/lc", g_root);
    if (mkdir(g_root, 0755) != 0 || mkdir(lc, 0755) != 0) return -1;
    return 0;
}

static int teardown(void **state) {
    (void)state;
    remove_tree(g_root);
    return 0;
}

/* A1: golden ems/lc（转换后形态）逐字段加载。 */
static void test_golden_files_load_every_retained_field(void **state) {
    (void)state;
    assert_int_equal(copy_golden_pcs(), 0);
    iems_pcs_settings_t s;
    memset(&s, 0, sizeof(s));
    char err[256];
    assert_int_equal(load_root(g_root, &s, err, sizeof(err)), 0);
    assert_string_equal(err, "");
    assert_true(s.loaded);

    /* ems: 13 个保留旧键（golden 值） */
    assert_int_equal(s.ems.Pmlmax, 800);
    assert_int_equal(s.ems.K1, 90);
    assert_int_equal(s.ems.K2, 90);
    assert_int_equal(s.ems.EnProtectDOD, 1);
    assert_int_equal(s.ems.EnBmsAlarm, 1);
    assert_int_equal(s.ems.SOCmax, 90);
    assert_int_equal(s.ems.SOCmin, 10);
    assert_int_equal(s.ems.Pset, 5);
    assert_int_equal(s.ems.Adjpreriod, 5);
    assert_int_equal(s.ems.SleepMode, 1);
    assert_float_equal(s.ems.SocKeepActiveSOC, 40.0, 1e-9);
    assert_float_equal(s.ems.SocKeepMargin, 5.0, 1e-9);
    assert_float_equal(s.ems.SocKeepChargePower, 0.0, 1e-9);
    /* ems: schema 当前读取键不在 golden 里 → 默认 0 */
    assert_int_equal(s.ems.PvMode, 0);
    assert_int_equal(s.ems.CabPower, 0);
    assert_int_equal(s.ems.ENRPComp, 0);
    assert_float_equal(s.ems.SetPowerFactor, 0.0, 1e-9);
    assert_int_equal(s.ems.SocKeepModeEnable, 0);
    assert_int_equal(s.ems.tou_count, 0);

    /* lc: 24 个键（golden 值） */
    assert_int_equal(s.lc.EnBmsAlarm, 1);
    assert_int_equal(s.lc.EnProtectDOD, 1);
    assert_int_equal(s.lc.BmsPowerLimit, 0);
    assert_int_equal(s.lc.PcsAutoTurn, 0);
    assert_int_equal(s.lc.PcsAutoTurnOffTime, 0);
    assert_int_equal(s.lc.EnIoProtect, 0);
    assert_int_equal(s.lc.SOCmax, 90);
    assert_int_equal(s.lc.SOCmin, 10);
    assert_int_equal(s.lc.PowerLimit, 80);
    assert_int_equal(s.lc.TMSCtrlTempCold, 0);
    assert_int_equal(s.lc.TMSCtrlTempHot, 0);
    assert_int_equal(s.lc.TMSCtrlState, 0);
    assert_int_equal(s.lc.AirCondStartStopCtrl, 0);
    assert_float_equal(s.lc.singleMaxVoltage, 4.1, 1e-9);
    assert_float_equal(s.lc.singleMinVoltage, 2.6, 1e-9);
    assert_int_equal(s.lc.enSingleVoltage, 0);
    assert_float_equal(s.lc.singleMaxVoltageErr, 0.3, 1e-9);
    assert_float_equal(s.lc.singleMinVoltageErr, 0.3, 1e-9);
    assert_int_equal(s.lc.SOCminErr, 4);
    assert_int_equal(s.lc.SOCmaxErr, 4);
    assert_int_equal(s.lc.EnDraughtFanPower, 0);
    assert_int_equal(s.lc.DFanPowerThreShold, 0);
    assert_int_equal(s.lc.EnDraughtFanTem, 0);
    assert_int_equal(s.lc.DFanTemThreShold, 0);
}

/* A2/A4: 缺文件 → ABSENT，默认保留，加载不失败；组合各自独立。 */
static void test_absent_files_keep_defaults(void **state) {
    (void)state;
    iems_pcs_settings_t s;
    memset(&s, 0, sizeof(s));
    char err[256];
    assert_int_equal(load_root(g_root, &s, err, sizeof(err)), 0);
    assert_false(s.loaded);
    assert_int_equal(s.ems.Pmlmax, 0);
    assert_int_equal(s.lc.PowerLimit, 0);
}

static void test_partial_files_load_independently(void **state) {
    (void)state;
    char src[512];
    char dst[512];

    /* ems 有、lc 缺 */
    golden_ems(src, sizeof(src));
    snprintf(dst, sizeof(dst), "%s/ems_cfg.json", g_root);
    assert_int_equal(copy_file(src, dst), 0);
    iems_pcs_settings_t s;
    memset(&s, 0, sizeof(s));
    char err[256];
    assert_int_equal(load_root(g_root, &s, err, sizeof(err)), 0);
    assert_false(s.loaded);
    assert_int_equal(s.ems.Pmlmax, 800);
    assert_int_equal(s.lc.PowerLimit, 0);
    unlink(dst);

    /* lc 有、ems 缺 */
    golden_lc(src, sizeof(src));
    snprintf(dst, sizeof(dst), "%s/lc/lc_cfg.json", g_root);
    assert_int_equal(copy_file(src, dst), 0);
    memset(&s, 0, sizeof(s));
    assert_int_equal(load_root(g_root, &s, err, sizeof(err)), 0);
    assert_false(s.loaded);
    assert_int_equal(s.ems.Pmlmax, 0);
    assert_int_equal(s.lc.PowerLimit, 80);
}

/* A3/A4: 坏文件 → -1，错误含路径与字段，模型不变（无半加载）。 */
static void expect_ems_failure(const char *ems_text, const char *want_path,
                               const char *want_field) {
    char path[512];
    snprintf(path, sizeof(path), "%s/ems_cfg.json", g_root);
    assert_int_equal(write_text(path, ems_text), 0);
    iems_pcs_settings_t model;
    memset(&model, 0xA5, sizeof(model));
    iems_pcs_settings_t before;
    memcpy(&before, &model, sizeof(model));
    char err[256];
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), -1);
    assert_non_null(strstr(err, want_path));
    if (want_field) assert_non_null(strstr(err, want_field));
    assert_memory_equal(&model, &before, sizeof(model));
}

static void test_bad_ems_file_fails_closed(void **state) {
    (void)state;
    expect_ems_failure("{", "ems_cfg.json", NULL);
    expect_ems_failure("{}", "ems_cfg.json", "schema_version");
    expect_ems_failure("{\"schema_version\":\"2.0.0\"}", "ems_cfg.json", "schema_version");
    expect_ems_failure("{\"schema_version\":\"1.0.0\",\"Bogus\":1}", "ems_cfg.json", "Bogus");
    /* deprecated 旧名已被转换器丢弃：加载器按未知键拒绝 */
    expect_ems_failure("{\"schema_version\":\"1.0.0\",\"pv_mode\":1}", "ems_cfg.json", "pv_mode");
    expect_ems_failure("{\"schema_version\":\"1.0.0\",\"Pmlmax\":\"x\"}", "ems_cfg.json", "Pmlmax");
    expect_ems_failure("{\"schema_version\":\"1.0.0\",\"Pmlmax\":5000000000}", "ems_cfg.json",
                       "Pmlmax");
    expect_ems_failure("{\"schema_version\":\"1.0.0\",\"SocKeepMargin\":1e400}",
                       "ems_cfg.json", "SocKeepMargin");
    expect_ems_failure(
        "{\"schema_version\":\"1.0.0\",\"tou\":[{},{},{},{},{},{},{}]}", "ems_cfg.json", "tou");
    char cleanup[512];
    snprintf(cleanup, sizeof(cleanup), "%s/ems_cfg.json", g_root);
    unlink(cleanup);
}

static void test_bad_lc_file_fails_closed_without_half_load(void **state) {
    (void)state;
    char src[512];
    char dst[512];
    golden_ems(src, sizeof(src));
    snprintf(dst, sizeof(dst), "%s/ems_cfg.json", g_root);
    assert_int_equal(copy_file(src, dst), 0); /* ems 先成功 */

    char path[512];
    snprintf(path, sizeof(path), "%s/lc/lc_cfg.json", g_root);
    assert_int_equal(write_text(path, "{\"schema_version\":\"1.0.0\",\"PowerLimit\":\"x\"}"), 0);

    iems_pcs_settings_t model;
    memset(&model, 0xA5, sizeof(model));
    iems_pcs_settings_t before;
    memcpy(&before, &model, sizeof(model));
    char err[256];
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), -1);
    assert_non_null(strstr(err, "lc/lc_cfg.json"));
    assert_non_null(strstr(err, "PowerLimit"));
    /* ems 已解析但整体失败：模型必须原样（无半加载） */
    assert_memory_equal(&model, &before, sizeof(model));
}

static void test_lc_unknown_key_and_bad_version_fail(void **state) {
    (void)state;
    char path[512];
    snprintf(path, sizeof(path), "%s/lc/lc_cfg.json", g_root);
    assert_int_equal(write_text(path, "{\"schema_version\":\"1.0.0\",\"Nope\":1}"), 0);
    iems_pcs_settings_t model;
    memset(&model, 0, sizeof(model));
    char err[256];
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), -1);
    assert_non_null(strstr(err, "Nope"));

    assert_int_equal(write_text(path, "{\"schema_version\":\"9.9.9\"}"), 0);
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), -1);
    assert_non_null(strstr(err, "schema_version"));
}

static void close_live_handle(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

/* A5: iems_app 装配后只读 getter 返回同一份加载结果；device/port 加载不受影响。 */
static void test_app_getter_returns_loaded_settings(void **state) {
    (void)state;
    assert_int_equal(copy_golden_device_port(), 0);
    assert_int_equal(copy_golden_pcs(), 0);

    char appctl[300];
    snprintf(appctl, sizeof(appctl), "%s/appctl.sock", g_root);
    assert_int_equal(setenv("EDGEC_APPCTL_SOCK", appctl, 1), 0);
    assert_int_equal(setenv("IEMS_IEC104_HOST", "127.0.0.1", 1), 0);
    assert_int_equal(setenv("IEMS_IEC104_PORT", "0", 1), 0);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    char db[300], wal[300], snap[300], ui[300], north[300], shm[64];
    snprintf(db, sizeof(db), "%s/app.db", g_root);
    snprintf(wal, sizeof(wal), "%s/wal.log", g_root);
    snprintf(snap, sizeof(snap), "%s/snap.bin", g_root);
    snprintf(ui, sizeof(ui), "%s/ui.sock", g_root);
    snprintf(north, sizeof(north), "%s/north.sock", g_root);
    snprintf(shm, sizeof(shm), "IEMS_PCS_TEST_%d", (int)getpid());
    iems_app_config_t cfg = {
        .db_path = db,
        .shm_name = shm,
        .wal_path = wal,
        .snap_path = snap,
        .rest_port = 19000 + (int)(getpid() % 1000),
        .ui_sock = ui,
        .north_sock = north,
        .config_dir = g_root,
    };
    iems_app_t *app = iems_app_assemble(&loop, &cfg);
    assert_non_null(app);
    const iems_pcs_settings_t *settings = iems_app_get_pcs_settings(app);
    assert_non_null(settings);
    assert_true(settings->loaded);
    assert_int_equal(settings->ems.Pmlmax, 800);
    assert_float_equal(settings->ems.SocKeepActiveSOC, 40.0, 1e-9);
    assert_int_equal(settings->lc.PowerLimit, 80);
    assert_float_equal(settings->lc.singleMaxVoltage, 4.1, 1e-9);
    iems_app_destroy(app);
    /* 组装期启动的组件在 destroy 后仍可能留着已 init 的 handle（二进制进程直接退出
     * 不关）；单测里统一关掉再收 loop，避免 uv_loop_close 返回 EBUSY。 */
    uv_walk(&loop, close_live_handle, NULL);
    for (int i = 0; i < 200 && uv_loop_alive(&loop); i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

/* P2-02a r2: 合法 tou 项逐项加载：2 项 × 8 字段非零，slot 索引与 count 对齐。 */
static void test_ems_tou_entries_load(void **state) {
    (void)state;
    const char *ems =
        "{\"schema_version\":\"1.0.0\",\"tou\":["
        "{\"StHour\":1,\"StMin\":2,\"EnHour\":3,\"EnMin\":4,"
        "\"Power\":5,\"RePower\":6,\"SOCUpperLimit\":7,\"SOCLowerLimit\":8},"
        "{\"StHour\":9,\"StMin\":10,\"EnHour\":11,\"EnMin\":12,"
        "\"Power\":13,\"RePower\":14,\"SOCUpperLimit\":15,\"SOCLowerLimit\":16}"
        "]}";
    char path[512];
    snprintf(path, sizeof(path), "%s/ems_cfg.json", g_root);
    assert_int_equal(write_text(path, ems), 0);

    iems_pcs_settings_t s;
    memset(&s, 0, sizeof(s));
    char err[256];
    assert_int_equal(load_root(g_root, &s, err, sizeof(err)), 0);
    assert_string_equal(err, "");
    assert_int_equal(s.ems.tou_count, 2);

    const iems_ems_tou_t *first = &s.ems.tou[0];
    assert_int_equal(first->StHour, 1);
    assert_int_equal(first->StMin, 2);
    assert_int_equal(first->EnHour, 3);
    assert_int_equal(first->EnMin, 4);
    assert_int_equal(first->Power, 5);
    assert_int_equal(first->RePower, 6);
    assert_int_equal(first->SOCUpperLimit, 7);
    assert_int_equal(first->SOCLowerLimit, 8);

    const iems_ems_tou_t *second = &s.ems.tou[1];
    assert_int_equal(second->StHour, 9);
    assert_int_equal(second->StMin, 10);
    assert_int_equal(second->EnHour, 11);
    assert_int_equal(second->EnMin, 12);
    assert_int_equal(second->Power, 13);
    assert_int_equal(second->RePower, 14);
    assert_int_equal(second->SOCUpperLimit, 15);
    assert_int_equal(second->SOCLowerLimit, 16);

    /* 未用到的 slot 与未给出的标量保持零默认；lc 缺失 → loaded=false（不是“什么都没加载”） */
    assert_int_equal(s.ems.tou[2].StHour, 0);
    assert_int_equal(s.ems.Pmlmax, 0);
    assert_false(s.loaded);
}

/* P2-02a r2: device/port 合法但 ems 损坏 → iems_app_assemble 走 PCS fail-closed
 * 分支返回 NULL（不需要跑 loop/服务）。 */
static void test_assembler_refuses_corrupt_ems(void **state) {
    (void)state;
    assert_int_equal(copy_golden_device_port(), 0);
    char path[512];
    snprintf(path, sizeof(path), "%s/ems_cfg.json", g_root);
    assert_int_equal(write_text(path, "{\"schema_version\":\"1.0.0\",\"Bogus\":1}"), 0);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    char db[300], wal[300], snap[300], shm[64];
    snprintf(db, sizeof(db), "%s/refuse.db", g_root);
    snprintf(wal, sizeof(wal), "%s/refuse_wal.log", g_root);
    snprintf(snap, sizeof(snap), "%s/refuse_snap.bin", g_root);
    snprintf(shm, sizeof(shm), "IEMS_PCS_REFUSE_%d", (int)getpid());
    iems_app_config_t cfg = {
        .db_path = db,
        .shm_name = shm,
        .wal_path = wal,
        .snap_path = snap,
        .rest_port = 19000 + (int)(getpid() % 1000),
        .config_dir = g_root,
    };
    assert_null(iems_app_assemble(&loop, &cfg));
    uv_walk(&loop, close_live_handle, NULL);
    for (int i = 0; i < 200 && uv_loop_alive(&loop); i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_golden_files_load_every_retained_field, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_absent_files_keep_defaults, setup, teardown),
        cmocka_unit_test_setup_teardown(test_partial_files_load_independently, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bad_ems_file_fails_closed, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bad_lc_file_fails_closed_without_half_load, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_lc_unknown_key_and_bad_version_fail, setup, teardown),
        cmocka_unit_test_setup_teardown(test_ems_tou_entries_load, setup, teardown),
        cmocka_unit_test_setup_teardown(test_app_getter_returns_loaded_settings, setup, teardown),
        cmocka_unit_test_setup_teardown(test_assembler_refuses_corrupt_ems, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
