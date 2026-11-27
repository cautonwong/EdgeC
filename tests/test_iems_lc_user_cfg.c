/**
 * @file test_iems_lc_user_cfg.c
 * @brief P1-07a: per-LC typed lc_user_cfg.json loading. Golden values, the
 *        three-value source (OK/ABSENT/ERROR), atomic fail-closed, enumeration
 *        limits, and app boot wiring.
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
#include "iems_lc_user_cfg.h"

#ifndef IEMS_CONFIG_GOLDEN_DIR
#error "IEMS_CONFIG_GOLDEN_DIR must point at tools/migration/golden"
#endif

_Static_assert(IEMS_LC_MAX_LCS == 32, "enumerated LC dirs are capped at 32");
_Static_assert(IEMS_LC_VALUE_INT == 0 && IEMS_LC_VALUE_DOUBLE == 1,
               "type ids match the converter schema");

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

static void golden_lc_user_cfg(const char *lc, char *path, size_t cap) {
    snprintf(path, cap, "%s/lc/lc_devs/%s/lc_user_cfg.json", IEMS_CONFIG_GOLDEN_DIR, lc);
}

static int checked_snprintf(char *out, size_t cap, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out, cap, fmt, ap);
    va_end(ap);
    return (n < 0 || (size_t)n >= cap) ? -1 : 0;
}

/* Copies the golden lc_user_cfg.json into <root>/lc_devs/<lc>/ (mkdir as needed). */
static int copy_golden_lc(const char *lc) {
    char src[512];
    char dir[512];
    char dst[512];
    golden_lc_user_cfg(lc, src, sizeof(src));
    if (checked_snprintf(dir, sizeof(dir), "%s/lc_devs/%s", g_root, lc) != 0) return -1;
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) return -1;
    if (checked_snprintf(dst, sizeof(dst), "%s/lc_user_cfg.json", dir) != 0) return -1;
    return copy_file(src, dst);
}

static int write_lc_doc(const char *lc, const char *text) {
    char dir[512];
    char path[512];
    if (checked_snprintf(dir, sizeof(dir), "%s/lc_devs/%s", g_root, lc) != 0) return -1;
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) return -1;
    if (checked_snprintf(path, sizeof(path), "%s/lc_user_cfg.json", dir) != 0) return -1;
    return write_text(path, text);
}

static int load_root(const char *root, iems_lc_user_cfgs_t *out, char *err, size_t err_size) {
    iems_config_file_source_t fs;
    iems_config_file_source_init(&fs, root);
    iems_config_source_t source = iems_config_file_source_port(&fs);
    return iems_lc_user_cfgs_load(&source, out, err, err_size);
}

static int setup(void **state) {
    (void)state;
    snprintf(g_root, sizeof(g_root), "/tmp/iems_lc_user_cfg_%d", (int)getpid());
    remove_tree(g_root);
    char lc[512];
    snprintf(lc, sizeof(lc), "%s/lc_devs", g_root);
    if (mkdir(g_root, 0755) != 0 || mkdir(lc, 0755) != 0) return -1;
    return 0;
}

static int teardown(void **state) {
    (void)state;
    remove_tree(g_root);
    return 0;
}

/* A1: LC1..LC5 golden files load with sorted names and typed entries.
 * Expected values come from the golden lc_devs LC1..LC5 files. */
static void test_golden_lcs_load_typed_entries(void **state) {
    (void)state;
    const char *lcs[] = {"LC1", "LC2", "LC3", "LC4", "LC5"};
    for (size_t i = 0; i < sizeof(lcs) / sizeof(lcs[0]); i++) {
        assert_int_equal(copy_golden_lc(lcs[i]), 0);
    }

    iems_lc_user_cfgs_t model;
    memset(&model, 0, sizeof(model));
    char err[256];
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), 0);
    assert_string_equal(err, "");
    assert_int_equal(model.count, 5);
    assert_true(model.loaded);
    for (size_t i = 0; i < 5; i++) {
        assert_string_equal(model.lcs[i].name, lcs[i]);
        assert_int_equal(model.lcs[i].state, IEMS_LC_FILE_LOADED);
    }

    /* LC1 golden: PowerLimit type1 double 80, EnProtectDOD type0 int 1,
     * Installedcab_coeff type1 double 80. Both type branches covered. */
    const iems_lc_user_cfg_t *lc1 = iems_lc_user_cfgs_find(&model, "LC1");
    assert_non_null(lc1);
    assert_int_equal(lc1->count, 3);

    const iems_lc_entry_t *entry = iems_lc_user_cfg_find(lc1, "PowerLimit");
    assert_non_null(entry);
    assert_int_equal(entry->type, IEMS_LC_VALUE_DOUBLE);
    assert_float_equal(entry->double_value, 80.0, 1e-9);

    entry = iems_lc_user_cfg_find(lc1, "EnProtectDOD");
    assert_non_null(entry);
    assert_int_equal(entry->type, IEMS_LC_VALUE_INT);
    assert_int_equal(entry->int_value, 1);

    entry = iems_lc_user_cfg_find(lc1, "Installedcab_coeff");
    assert_non_null(entry);
    assert_int_equal(entry->type, IEMS_LC_VALUE_DOUBLE);
    assert_float_equal(entry->double_value, 80.0, 1e-9);

    assert_null(iems_lc_user_cfg_find(lc1, "Missing"));

    /* LC2..LC5 golden: PowerLimit + Installedcab_coeff, both type1 double 80. */
    for (size_t i = 1; i < sizeof(lcs) / sizeof(lcs[0]); i++) {
        const iems_lc_user_cfg_t *cfg = iems_lc_user_cfgs_find(&model, lcs[i]);
        assert_non_null(cfg);
        assert_int_equal(cfg->state, IEMS_LC_FILE_LOADED);
        assert_int_equal(cfg->count, 2);

        entry = iems_lc_user_cfg_find(cfg, "PowerLimit");
        assert_non_null(entry);
        assert_int_equal(entry->type, IEMS_LC_VALUE_DOUBLE);
        assert_float_equal(entry->double_value, 80.0, 1e-9);

        entry = iems_lc_user_cfg_find(cfg, "Installedcab_coeff");
        assert_non_null(entry);
        assert_int_equal(entry->type, IEMS_LC_VALUE_DOUBLE);
        assert_float_equal(entry->double_value, 80.0, 1e-9);
    }
    assert_null(iems_lc_user_cfgs_find(&model, "LC9"));
}

/* A1: v3 vlc_cfg.c leaves the value at its memset zero when the key is absent. */
static void test_missing_value_defaults_to_zero(void **state) {
    (void)state;
    assert_int_equal(write_lc_doc("LC1",
                                  "{\"schema_version\":\"1.0.0\",\"config\":["
                                  "{\"type\":0,\"name\":\"NoInt\"},"
                                  "{\"type\":1,\"name\":\"NoDouble\"}]}"),
                     0);
    iems_lc_user_cfgs_t model;
    memset(&model, 0, sizeof(model));
    char err[256];
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), 0);
    assert_int_equal(model.count, 1);
    assert_int_equal(model.lcs[0].count, 2);

    const iems_lc_entry_t *entry = iems_lc_user_cfg_find(&model.lcs[0], "NoInt");
    assert_non_null(entry);
    assert_int_equal(entry->type, IEMS_LC_VALUE_INT);
    assert_int_equal(entry->int_value, 0);

    entry = iems_lc_user_cfg_find(&model.lcs[0], "NoDouble");
    assert_non_null(entry);
    assert_int_equal(entry->type, IEMS_LC_VALUE_DOUBLE);
    assert_float_equal(entry->double_value, 0.0, 1e-9);

    /* v3's 125 default for PowerLimit/Installedcab_coeff (vlc_cfg.c:30-33) is
     * not synthesized here: an omitted name stays missing for the consumer. */
    assert_null(iems_lc_user_cfg_find(&model.lcs[0], "PowerLimit"));
    assert_null(iems_lc_user_cfg_find(&model.lcs[0], "Installedcab_coeff"));
}

/* A2: a missing file is ABSENT, not fatal; the LC stays enumerated and empty. */
static void test_absent_files_keep_empty_lc_and_continue(void **state) {
    (void)state;
    assert_int_equal(copy_golden_lc("LC1"), 0);
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/lc_devs/LC2", g_root);
    assert_int_equal(mkdir(dir, 0755), 0);

    iems_lc_user_cfgs_t model;
    memset(&model, 0, sizeof(model));
    char err[256];
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), 0);
    assert_string_equal(err, "");
    assert_int_equal(model.count, 2);
    assert_false(model.loaded);
    assert_string_equal(model.lcs[0].name, "LC1");
    assert_int_equal(model.lcs[0].state, IEMS_LC_FILE_LOADED);
    assert_int_equal(model.lcs[0].count, 3);
    assert_string_equal(model.lcs[1].name, "LC2");
    assert_int_equal(model.lcs[1].state, IEMS_LC_FILE_ABSENT);
    assert_int_equal(model.lcs[1].count, 0);

    /* Whole lc_devs absent: no LCs, still rc 0. */
    remove_tree(g_root);
    assert_int_equal(mkdir(g_root, 0755), 0);
    memset(&model, 0xA5, sizeof(model));
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), 0);
    assert_int_equal(model.count, 0);
    assert_false(model.loaded);
    assert_null(iems_lc_user_cfgs_find(&model, "LC1"));
}

static void expect_bad_document(const char *text, const char *want_field) {
    assert_int_equal(write_lc_doc("LC1", text), 0);
    iems_lc_user_cfgs_t model;
    iems_lc_user_cfgs_t before;
    memset(&model, 0xA5, sizeof(model));
    memcpy(&before, &model, sizeof(model));
    char err[256];
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), -1);
    assert_non_null(strstr(err, "lc_devs/LC1/lc_user_cfg.json"));
    if (want_field) assert_non_null(strstr(err, want_field));
    assert_memory_equal(&model, &before, sizeof(model));
}

/* A3: every invalid shape is -1 and leaves the caller model byte-identical. */
static void test_bad_documents_fail_closed_atomically(void **state) {
    (void)state;
    expect_bad_document("{", NULL);
    expect_bad_document("{\"config\":[]}", "schema_version");
    expect_bad_document("{\"schema_version\":\"2.0.0\",\"config\":[]}", "schema_version");
    expect_bad_document("{\"schema_version\":\"1.0.0\"}", "config");
    expect_bad_document("{\"schema_version\":\"1.0.0\",\"config\":{}}", "config");
    expect_bad_document("{\"schema_version\":\"1.0.0\",\"Bogus\":1,\"config\":[]}", "Bogus");
    /* type/value mismatch: type 0 only allows int_value. */
    expect_bad_document("{\"schema_version\":\"1.0.0\",\"config\":["
                        "{\"type\":0,\"name\":\"X\",\"double_value\":1.0}]}",
                        "double_value");
    expect_bad_document("{\"schema_version\":\"1.0.0\",\"config\":["
                        "{\"type\":1,\"name\":\"X\",\"int_value\":1}]}",
                        "int_value");
    /* unknown item key */
    expect_bad_document("{\"schema_version\":\"1.0.0\",\"config\":["
                        "{\"type\":0,\"name\":\"X\",\"int_value\":1,\"bogus\":2}]}",
                        "bogus");
    /* type outside 0/1 */
    expect_bad_document("{\"schema_version\":\"1.0.0\",\"config\":["
                        "{\"type\":2,\"name\":\"X\"}]}",
                        "type");
    /* empty name */
    expect_bad_document("{\"schema_version\":\"1.0.0\",\"config\":["
                        "{\"type\":0,\"name\":\"\",\"int_value\":1}]}",
                        "name");
    /* non-finite double (cJSON parses 1e999 as +inf) */
    expect_bad_document("{\"schema_version\":\"1.0.0\",\"config\":["
                        "{\"type\":1,\"name\":\"X\",\"double_value\":1e999}]}",
                        "double_value");
    /* duplicate name within one file: rejected instead of v3 last-wins. */
    expect_bad_document("{\"schema_version\":\"1.0.0\",\"config\":["
                        "{\"type\":0,\"name\":\"Dup\",\"int_value\":1},"
                        "{\"type\":0,\"name\":\"Dup\",\"int_value\":2}]}",
                        "Dup");
}

/* A3: a failure late in the enumeration must not leak earlier parsed LCs. */
static void test_late_failure_keeps_whole_model_untouched(void **state) {
    (void)state;
    assert_int_equal(copy_golden_lc("LC1"), 0);
    assert_int_equal(write_lc_doc("LC2", "{\"schema_version\":\"1.0.0\",\"config\":[{}]}"), 0);

    iems_lc_user_cfgs_t model;
    iems_lc_user_cfgs_t before;
    memset(&model, 0xA5, sizeof(model));
    memcpy(&before, &model, sizeof(model));
    char err[256];
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), -1);
    assert_non_null(strstr(err, "lc_devs/LC2/lc_user_cfg.json"));
    assert_memory_equal(&model, &before, sizeof(model));
}

/* A4: enumeration bounds and lc_devs that is not a directory. */
static void test_enumeration_limits_and_non_directory(void **state) {
    (void)state;
    char lc_devs[512];
    snprintf(lc_devs, sizeof(lc_devs), "%s/lc_devs", g_root);
    remove_tree(lc_devs);
    assert_int_equal(write_text(lc_devs, "not a directory"), 0);

    iems_lc_user_cfgs_t model;
    char err[256];
    memset(&model, 0, sizeof(model));
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), -1);
    assert_non_null(strstr(err, "lc_devs"));

    unlink(lc_devs);
    assert_int_equal(mkdir(lc_devs, 0755), 0);
    char lc[512];
    for (int i = 0; i < 33; i++) {
        snprintf(lc, sizeof(lc), "L%02d", i);
        assert_int_equal(write_lc_doc(lc, "{\"schema_version\":\"1.0.0\",\"config\":[]}"), 0);
    }
    memset(&model, 0, sizeof(model));
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), -1);
    assert_non_null(strstr(err, "TOO_MANY_FILES"));
}

/* A4: per-file entry cap keeps the fixed-size model bounded. */
static void test_too_many_entries_in_one_file(void **state) {
    (void)state;
    char doc[4096];
    size_t used = (size_t)snprintf(doc, sizeof(doc), "{\"schema_version\":\"1.0.0\",\"config\":[");
    for (int i = 0; i < 33; i++) {
        used += (size_t)snprintf(doc + used, sizeof(doc) - used,
                                 "%s{\"type\":0,\"name\":\"E%d\",\"int_value\":%d}",
                                 i == 0 ? "" : ",", i, i);
    }
    snprintf(doc + used, sizeof(doc) - used, "]}");
    assert_int_equal(write_lc_doc("LC1", doc), 0);

    iems_lc_user_cfgs_t model;
    memset(&model, 0, sizeof(model));
    char err[256];
    assert_int_equal(load_root(g_root, &model, err, sizeof(err)), -1);
    assert_non_null(strstr(err, "TOO_MANY_ITEMS"));
}

/* A4: explicit EDGE_CONFIG_DIR + bad lc_user_cfg.json refuses to boot. App
 * getter returns the startup model when every LC file is valid. */
static int setup_assembly_root(void) {
    char dst[512];
    snprintf(dst, sizeof(dst), "%s/port_config.json", g_root);
    if (copy_file(IEMS_CONFIG_GOLDEN_DIR "/port/port_config.json", dst) != 0) return -1;
    snprintf(dst, sizeof(dst), "%s/device_config.json", g_root);
    if (copy_file(IEMS_CONFIG_GOLDEN_DIR "/device/device_config.json", dst) != 0) return -1;
    snprintf(dst, sizeof(dst), "%s/lc_devs/LC1", g_root);
    if (mkdir(dst, 0755) != 0 && errno != EEXIST) return -1;
    /* The device loader requires a device_config.json in every lc_devs dir;
     * an empty class keeps the root device set untouched. */
    snprintf(dst, sizeof(dst), "%s/lc_devs/LC1/device_config.json", g_root);
    return write_text(dst, "{\"schema_version\":\"1.0.0\",\"dev_class\":[]}");
}

static void set_app_env(void) {
    char appctl[300];
    snprintf(appctl, sizeof(appctl), "%s/appctl.sock", g_root);
    assert_int_equal(setenv("EDGEC_APPCTL_SOCK", appctl, 1), 0);
    assert_int_equal(setenv("IEMS_IEC104_HOST", "127.0.0.1", 1), 0);
    assert_int_equal(setenv("IEMS_IEC104_PORT", "0", 1), 0);
}

static iems_app_t *assemble_root(uv_loop_t *loop, const char *db_name) {
    char db[300], wal[300], snap[300], ui[300], north[300], shm[64];
    snprintf(db, sizeof(db), "%s/%s.db", g_root, db_name);
    snprintf(wal, sizeof(wal), "%s/%s_wal.log", g_root, db_name);
    snprintf(snap, sizeof(snap), "%s/%s_snap.bin", g_root, db_name);
    snprintf(ui, sizeof(ui), "%s/ui.sock", g_root);
    snprintf(north, sizeof(north), "%s/north.sock", g_root);
    snprintf(shm, sizeof(shm), "IEMS_LC_%d_%s", (int)getpid(), db_name);
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
    return iems_app_assemble(loop, &cfg);
}

static void close_live_handle(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static void close_loop(uv_loop_t *loop) {
    uv_walk(loop, close_live_handle, NULL);
    for (int i = 0; i < 200 && uv_loop_alive(loop); i++) uv_run(loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(loop), 0);
}

static void test_app_getter_returns_loaded_lcs(void **state) {
    (void)state;
    assert_int_equal(setup_assembly_root(), 0);
    assert_int_equal(copy_golden_lc("LC1"), 0);
    set_app_env();

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    iems_app_t *app = assemble_root(&loop, "getter");
    assert_non_null(app);

    const iems_lc_user_cfgs_t *cfgs = iems_app_get_lc_user_cfgs(app);
    assert_non_null(cfgs);
    assert_int_equal(cfgs->count, 1);
    assert_true(cfgs->loaded);
    const iems_lc_user_cfg_t *lc1 = iems_lc_user_cfgs_find(cfgs, "LC1");
    assert_non_null(lc1);
    assert_int_equal(lc1->state, IEMS_LC_FILE_LOADED);
    const iems_lc_entry_t *entry = iems_lc_user_cfg_find(lc1, "PowerLimit");
    assert_non_null(entry);
    assert_float_equal(entry->double_value, 80.0, 1e-9);

    iems_app_destroy(app);
    close_loop(&loop);
}

/* A2: app-level counterpart of the loader ABSENT case: a converted root whose
 * lc_devs/LC1 has no lc_user_cfg.json still boots, and the LC is enumerated
 * empty. Only a present-but-bad file refuses boot. */
static void test_app_starts_with_absent_lc_user_cfg(void **state) {
    (void)state;
    assert_int_equal(setup_assembly_root(), 0);
    set_app_env();

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    iems_app_t *app = assemble_root(&loop, "absent");
    assert_non_null(app);

    const iems_lc_user_cfgs_t *cfgs = iems_app_get_lc_user_cfgs(app);
    assert_non_null(cfgs);
    assert_int_equal(cfgs->count, 1);
    assert_false(cfgs->loaded);
    assert_string_equal(cfgs->lcs[0].name, "LC1");
    assert_int_equal(cfgs->lcs[0].state, IEMS_LC_FILE_ABSENT);
    assert_int_equal(cfgs->lcs[0].count, 0);

    iems_app_destroy(app);
    close_loop(&loop);
}

static void test_assembler_refuses_bad_lc_user_cfg(void **state) {
    (void)state;
    assert_int_equal(setup_assembly_root(), 0);
    assert_int_equal(copy_golden_lc("LC1"), 0);
    set_app_env();

    /* Same directory with a valid file boots; only the corrupt file refuses. */
    uv_loop_t good_loop;
    assert_int_equal(uv_loop_init(&good_loop), 0);
    iems_app_t *app = assemble_root(&good_loop, "good");
    assert_non_null(app);
    iems_app_destroy(app);
    close_loop(&good_loop);

    assert_int_equal(write_lc_doc("LC1", "{\"schema_version\":\"1.0.0\",\"config\":["
                                        "{\"type\":0,\"name\":\"X\",\"double_value\":1.0}]}"),
                     0);
    uv_loop_t bad_loop;
    assert_int_equal(uv_loop_init(&bad_loop), 0);
    assert_null(assemble_root(&bad_loop, "bad"));
    close_loop(&bad_loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_golden_lcs_load_typed_entries, setup, teardown),
        cmocka_unit_test_setup_teardown(test_missing_value_defaults_to_zero, setup, teardown),
        cmocka_unit_test_setup_teardown(test_absent_files_keep_empty_lc_and_continue, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_bad_documents_fail_closed_atomically, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_late_failure_keeps_whole_model_untouched, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_enumeration_limits_and_non_directory, setup, teardown),
        cmocka_unit_test_setup_teardown(test_too_many_entries_in_one_file, setup, teardown),
        cmocka_unit_test_setup_teardown(test_app_getter_returns_loaded_lcs, setup, teardown),
        cmocka_unit_test_setup_teardown(test_app_starts_with_absent_lc_user_cfg, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_assembler_refuses_bad_lc_user_cfg, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
