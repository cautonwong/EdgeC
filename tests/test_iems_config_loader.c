/* P6-04b: converted device/port families load into inventory/poll_tasks. */

#define _GNU_SOURCE

#include <cmocka.h>
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <math.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <uv.h>

#include "iems_config_loader.h"
#include "iems_driver_adapter.h"
#include "iems_point_model.h"
#include "iems_task_loader.h"
#include "modbus_codec.h"
#include "sqlite_service.h"

#ifndef IEMS_CONFIG_GOLDEN_DIR
#error "IEMS_CONFIG_GOLDEN_DIR must point at tools/migration/golden"
#endif

#define TMP_PREFIX "/tmp/iems_cfg_load_XXXXXX"

static const char *k_valid_port =
    "{\"schema_version\":\"1.0.0\",\"dev_cfg\":{"
    "\"lan\":{\"attr\":[{\"ip\":\"10.0.0.1\"}]},"
    "\"uart\":{\"attr\":[{\"name\":\"RS485_1\",\"baud\":9600,\"data_bit\":8,"
    "\"stop_bit\":1,\"parity\":\"N\"}]}}}";

static const char *k_valid_device =
    "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
    "\"no\":\"M1\",\"name\":\"m1\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
    "\"protocol\":\"modbus_rtu\",\"addr\":\"1\"}]}]}";

/* UART bus 0 and CAN bus 1 are both provisioned in the static connectivity. */
static const char *k_uart_can_port =
    "{\"schema_version\":\"1.0.0\",\"dev_cfg\":{"
    "\"lan\":{\"attr\":[{\"ip\":\"10.0.0.1\"}]},"
    "\"uart\":{\"attr\":[{\"name\":\"RS485_1\",\"baud\":9600,\"data_bit\":8,"
    "\"stop_bit\":1,\"parity\":\"N\"}]},"
    "\"can\":{\"attr\":[{\"name\":\"CAN0\",\"baud\":500000,\"filter\":[]}]}}}";

static const char *g_linked_root;

/* ------------------------------------------------------------------ */
/* Temp tree helpers                                                  */
/* ------------------------------------------------------------------ */

static int _remove_entry(const char *path, const struct stat *sb, int flag, struct FTW *ftw) {
    (void)sb;
    (void)flag;
    (void)ftw;
    return remove(path);
}

static void _rm_tree(const char *path) {
    if (path && path[0]) nftw(path, _remove_entry, 16, FTW_DEPTH | FTW_PHYS);
}

static int _copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }
    char buf[4096];
    size_t got;
    int rc = 0;
    while ((got = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, got, out) != got) {
            rc = -1;
            break;
        }
    }
    fclose(in);
    if (fclose(out) != 0) rc = -1;
    return rc;
}

static int _copy_tree(const char *src, const char *dst) {
    if (mkdir(dst, 0700) != 0 && errno != EEXIST) return -1;
    DIR *dir = opendir(src);
    if (!dir) return -1;
    int rc = 0;
    struct dirent *entry;
    while (rc == 0 && (entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char from[1024], to[1024];
        snprintf(from, sizeof(from), "%s/%s", src, entry->d_name);
        snprintf(to, sizeof(to), "%s/%s", dst, entry->d_name);
        struct stat st;
        if (stat(from, &st) != 0) {
            rc = -1;
        } else if (S_ISDIR(st.st_mode)) {
            rc = _copy_tree(from, to);
        } else {
            rc = _copy_file(from, to);
        }
    }
    closedir(dir);
    return rc;
}

static void _write_file(const char *dir, const char *name, const char *content) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    assert_non_null(f);
    size_t len = strlen(content);
    assert_int_equal(fwrite(content, 1, len, f), len);
    assert_int_equal(fclose(f), 0);
}

static void _write_bytes(const char *dir, const char *name, const void *data, size_t len) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    assert_non_null(f);
    assert_int_equal(fwrite(data, 1, len, f), len);
    assert_int_equal(fclose(f), 0);
}

static void _write_fill(const char *dir, const char *name, char value, size_t count) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    assert_non_null(f);
    for (size_t i = 0; i < count; i++) assert_int_equal(fputc(value, f), value);
    assert_int_equal(fclose(f), 0);
}

static void _mkdir_rel(const char *dir, const char *name) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    assert_int_equal(mkdir(path, 0700), 0);
}

static void _symlink_path(const char *target, const char *link_path) {
    assert_int_equal(symlink(target, link_path), 0);
}

static void _make_root(char *root, size_t cap, const char *port_json, const char *device_json) {
    char template[] = TMP_PREFIX;
    assert_non_null(mkdtemp(template));
    snprintf(root, cap, "%s", template);
    if (port_json) _write_file(root, "port_config.json", port_json);
    if (device_json) _write_file(root, "device_config.json", device_json);
}

static void _make_raw_root(char *root, size_t cap, const char *port_json, const void *device,
                           size_t device_len) {
    char template[] = TMP_PREFIX;
    assert_non_null(mkdtemp(template));
    snprintf(root, cap, "%s", template);
    if (port_json) _write_file(root, "port_config.json", port_json);
    if (device) _write_bytes(root, "device_config.json", device, device_len);
}

/* Golden device tree plus the port family sibling, exactly the v3 layout. */
static void _make_golden_root(char *root, size_t cap) {
    char template[] = TMP_PREFIX;
    assert_non_null(mkdtemp(template));
    snprintf(root, cap, "%s", template);
    char device_src[1024], port_src[1024], port_dst[1024];
    snprintf(device_src, sizeof(device_src), "%s/device", IEMS_CONFIG_GOLDEN_DIR);
    snprintf(port_src, sizeof(port_src), "%s/port/port_config.json", IEMS_CONFIG_GOLDEN_DIR);
    snprintf(port_dst, sizeof(port_dst), "%s/port_config.json", root);
    assert_int_equal(_copy_tree(device_src, root), 0);
    assert_int_equal(_copy_file(port_src, port_dst), 0);
}

static int _load_root(const char *root, iems_config_model_t *model, char *err, size_t err_size) {
    iems_config_file_source_t fs;
    iems_config_file_source_init(&fs, root);
    iems_config_source_t source = iems_config_file_source_port(&fs);
    return iems_config_load(&source, iems_config_static_catalog(), model, err, err_size);
}

static const iems_config_device_t *_device_at(const iems_config_model_t *model, size_t index) {
    assert_true(index < model->device_count);
    return &model->devices[index];
}

static const iems_config_point_t *_find_point(const iems_config_device_t *device, uint32_t tag) {
    for (uint16_t i = 0; i < device->point_count; i++) {
        if (device->points[i].tag_id == tag) return &device->points[i];
    }
    return NULL;
}

static const iems_config_port_t *_find_port(const iems_config_model_t *model, const char *name) {
    for (size_t i = 0; i < model->port_count; i++) {
        if (strcmp(model->ports[i].name, name) == 0) return &model->ports[i];
    }
    return NULL;
}

static void _assert_point(const iems_config_device_t *device, uint32_t tag, const char *name,
                          uint16_t dense, double scale, uint32_t native_id) {
    const iems_config_point_t *point = _find_point(device, tag);
    assert_non_null(point);
    assert_string_equal(point->name, name);
    assert_int_equal(point->dense_idx, dense);
    assert_true(fabs(point->scale - scale) < 1e-6);
    assert_int_equal(point->native_id, native_id);
}

/* ------------------------------------------------------------------ */
/* Positive tests                                                     */
/* ------------------------------------------------------------------ */

static void test_golden_families_load(void **state) {
    (void)state;
    char root[512];
    _make_golden_root(root, sizeof(root));
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    assert_int_equal(rc, 0);
    assert_string_equal(err, "");

    assert_int_equal(model->device_count, 18);
    assert_int_equal(model->port_count, 37);

    const iems_config_device_t *d = _device_at(model, 0);
    assert_int_equal(d->device_id, 1);
    assert_string_equal(d->class_type, "METER");
    assert_string_equal(d->device_type, "GRID");
    assert_string_equal(d->no, "METER_A");
    assert_string_equal(d->name, "siteA");
    assert_string_equal(d->channel, "RS485_1");
    assert_string_equal(d->template_name, "METER_96j_MODBUS.json");
    assert_string_equal(d->user_def, "frame_wgap=100;");
    assert_string_equal(d->config_protocol, "modbus_rtu");
    assert_string_equal(d->protocol, "ModbusRTUHiCRC");
    assert_int_equal(d->addr, 1);
    assert_int_equal(d->bus_id, 0);
    assert_int_equal(d->point_count, 8);
    assert_string_equal(d->source_path, "device_config.json");
    _assert_point(d, 1001, "VOLTAGE_PHASE_A", 0, 0.1, MODBUS_NATIVE(0, 1));
    _assert_point(d, 1003, "GRID_POWER", 2, 1.0, MODBUS_NATIVE(12, 2));
    _assert_point(d, 4003, "BATTERY_POWER", 9, 0.01, MODBUS_NATIVE(15, 1));
    _assert_point(d, 4001, "BATTERY_SOC", 7, 1.0, MODBUS_NATIVE(261, 1));
    /* 槽位公式：device_id * 16 + dense_idx（P6-04b1 起 ingest 也按这个键落槽）。 */
    assert_int_equal(iems_tag_map_fn(1001, d->device_id), 16);

    /* CAN 设备走 bus 1，协议 token can_raw 归一为 CAN。 */
    d = _device_at(model, 4);
    assert_string_equal(d->class_type, "SWB");
    assert_string_equal(d->no, "SWB");
    assert_string_equal(d->channel, "CAN0");
    assert_string_equal(d->config_protocol, "can_raw");
    assert_string_equal(d->protocol, "CAN");
    assert_int_equal(d->bus_id, 1);
    assert_int_equal(d->point_count, 2);
    _assert_point(d, 4001, "BATTERY_SOC", 7, 0.1, 0x18FF50E5u);
    _assert_point(d, 4002, "BATTERY_CURRENT", 8, 0.1, 0x18FF50E6u);
    assert_int_equal(iems_tag_map_fn(4001, d->device_id), 5 * 16 + 7);

    /* 合法端口但未开通总线: UNCHECKED, 不生成轮询。 */
    d = _device_at(model, 5);
    assert_string_equal(d->no, "PV");
    assert_string_equal(d->channel, "RS485_6");
    assert_int_equal(d->bus_id, -1);
    assert_int_equal(d->point_count, 0);

    /* host:port 端点同样 UNCHECKED，协议仍规范化。 */
    d = _device_at(model, 6);
    assert_string_equal(d->no, "DIDO");
    assert_string_equal(d->channel, "10.0.0.1:503");
    assert_string_equal(d->protocol, "ModbusTCP");
    assert_int_equal(d->bus_id, -1);

    /* lc 与 lc_devs 变体按文件名排序进入同一模型。 */
    d = _device_at(model, 7);
    assert_string_equal(d->no, "DIDO");
    assert_string_equal(d->name, "dry_contact");
    assert_string_equal(d->source_path, "lc/device_config.json");
    d = _device_at(model, 8);
    assert_string_equal(d->name, "pcsA");
    assert_string_equal(d->source_path, "lc_devs/LC1/device_config.json");
    d = _device_at(model, 9);
    assert_string_equal(d->name, "bmsA");
    assert_string_equal(d->device_type, "BCU");
    d = _device_at(model, 17);
    assert_string_equal(d->name, "bmsE");
    assert_string_equal(d->source_path, "lc_devs/LC5/device_config.json");

    const iems_config_port_t *port = _find_port(model, "RS485_1");
    assert_non_null(port);
    assert_int_equal(port->kind, IEMS_CONFIG_PORT_UART);
    assert_int_equal(port->baud, 9600);
    assert_int_equal(port->data_bit, 8);
    assert_int_equal(port->stop_bit, 1);
    assert_int_equal(port->parity, 'N');
    port = _find_port(model, "CAN0");
    assert_non_null(port);
    assert_int_equal(port->kind, IEMS_CONFIG_PORT_CAN);
    assert_int_equal(port->baud, 500000);
    assert_int_equal(port->filter_count, 11);
    port = _find_port(model, "wan1");
    assert_non_null(port);
    assert_int_equal(port->kind, IEMS_CONFIG_PORT_WAN);
    assert_string_equal(port->type, "static");
    assert_string_equal(port->metric, "100");
    port = _find_port(model, "DI16");
    assert_non_null(port);
    assert_int_equal(port->kind, IEMS_CONFIG_PORT_DI);
    assert_string_equal(port->disp_name, "DI16");
    port = _find_port(model, "lan");
    assert_non_null(port);
    assert_string_equal(port->ip, "10.0.0.254");

    free(model);
}

static int g_loaded_tasks;
static void _count_loaded_task(const iems_loaded_task_t *task, void *user_data) {
    (void)task;
    (void)user_data;
    g_loaded_tasks++;
}

static void test_apply_sqlite_and_routes(void **state) {
    (void)state;
    char root[512];
    _make_golden_root(root, sizeof(root));
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    assert_int_equal(_load_root(root, model, err, sizeof(err)), 0);
    _rm_tree(root);

    char db_path[128];
    snprintf(db_path, sizeof(db_path), "/tmp/iems_cfg_loader_%d.db", (int)getpid());
    unlink(db_path);
    uv_loop_t *loop = uv_default_loop();
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);

    assert_int_equal(iems_config_apply_sqlite(model, svc, err, sizeof(err)), 0);
    /* 重复应用幂等：同一输入不叠加行。 */
    assert_int_equal(iems_config_apply_sqlite(model, svc, err, sizeof(err)), 0);

    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    assert_non_null(db);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM inventory;", -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int(stmt, 0), 18);
    sqlite3_finalize(stmt);
    assert_int_equal(sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM poll_tasks;", -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int(stmt, 0), 10);
    sqlite3_finalize(stmt);

    /* 既有产品 API 直接看到加载结果。 */
    g_loaded_tasks = 0;
    assert_int_equal(iems_poll_tasks_load(svc, _count_loaded_task, NULL), 10);
    assert_int_equal(g_loaded_tasks, 10);

    iems_device_route_t route;
    memset(&route, 0, sizeof(route));
    assert_int_equal(iems_poll_task_route(svc, 1001, &route), 0);
    assert_int_equal(route.bus_id, 0);
    assert_int_equal(route.slave_id, 1);
    assert_string_equal(route.protocol, "ModbusRTUHiCRC");
    memset(&route, 0, sizeof(route));
    assert_int_equal(iems_poll_task_route(svc, 4002, &route), 0);
    assert_int_equal(route.bus_id, 1);
    assert_int_equal(route.slave_id, 1);
    assert_string_equal(route.protocol, "CAN");
    /* 4001 同时出现在 Modbus 与 CAN 点表，两个设备都映射到总线：
     * 既有歧义守卫必须拒绝控制写。 */
    assert_int_equal(iems_poll_task_route(svc, 4001, &route), -2);
    assert_int_equal(iems_poll_task_route(svc, 2001, &route), -1);

    sqlite_svc_destroy(svc);
    unlink(db_path);
    free(model);
}

static void test_load_apply_entry(void **state) {
    (void)state;
    char root[512];
    _make_golden_root(root, sizeof(root));
    char db_path[128];
    snprintf(db_path, sizeof(db_path), "/tmp/iems_cfg_apply_%d.db", (int)getpid());
    unlink(db_path);
    uv_loop_t *loop = uv_default_loop();
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    char err[256];
    assert_int_equal(iems_config_load_apply(root, svc, err, sizeof(err)), 0);
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM poll_tasks;", -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int(stmt, 0), 10);
    sqlite3_finalize(stmt);
    sqlite_svc_destroy(svc);
    unlink(db_path);
    _rm_tree(root);
}

/* ------------------------------------------------------------------ */
/* Ports and catalog                                                  */
/* ------------------------------------------------------------------ */

static void test_protocol_from_config(void **state) {
    (void)state;
    static const struct {
        const char *token;
        const char *canonical;
    } good[] = {
        {"modbus_rtu", "ModbusRTUHiCRC"},
        {"modbus_rtu_rcrc", "ModbusRTU"},
        {"modbus_tcp", "ModbusTCP"},
        {"dlt645_2007", "DLT645"},
        {"can_raw", "CAN"},
        {"iec104", "IEC104"},
        {"ydt1363_2014", "YDT1363"},
    };
    char out[16];
    for (size_t i = 0; i < sizeof(good) / sizeof(good[0]); i++) {
        memset(out, 0, sizeof(out));
        assert_int_equal(iems_driver_protocol_from_config(good[i].token, out), 0);
        assert_string_equal(out, good[i].canonical);
    }
    static const char *bad[] = {"none", "script", "ems", "ups", "bmser_bin",
                                "opcua", "lc", "custom", "dlt645_1997", "", "MODBUS_RTU"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        assert_int_equal(iems_driver_protocol_from_config(bad[i], out), -1);
    }
    assert_int_equal(iems_driver_protocol_from_config(NULL, out), -1);
    assert_int_equal(iems_driver_protocol_from_config("modbus_rtu", NULL), -1);
}

static void test_secret_reference_is_preserved(void **state) {
    (void)state;
    char root[512];
    _make_root(root, sizeof(root), k_valid_port,
               "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
               "\"no\":\"M1\",\"name\":\"m1\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
               "\"protocol\":\"modbus_rtu\",\"user_def\":\"$secret:site.user\"}]}]}");
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    assert_int_equal(rc, 0);
    assert_int_equal(model->device_count, 1);
    assert_string_equal(model->devices[0].user_def, "$secret:site.user");
    free(model);
}

/* ------------------------------------------------------------------ */
/* Negative tests                                                     */
/* ------------------------------------------------------------------ */

static void _expect_load_error(const char *port_json, const char *device_json, const char *needle,
                               const char *forbidden) {
    char root[512];
    _make_root(root, sizeof(root), port_json, device_json);
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    free(model);
    assert_int_equal(rc, -1);
    if (needle) {
        assert_non_null(strstr(err, needle));
    }
    if (forbidden) {
        assert_null(strstr(err, forbidden));
    }
}

static void _expect_raw_error(const char *port_json, const void *device, size_t len,
                              const char *needle, const char *forbidden) {
    char root[512];
    _make_raw_root(root, sizeof(root), port_json, device, len);
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    free(model);
    assert_int_equal(rc, -1);
    if (needle) assert_non_null(strstr(err, needle));
    if (forbidden) assert_null(strstr(err, forbidden));
}

static int _load_device_json(const char *port_json, const char *device_json,
                             iems_config_model_t *model, char *err, size_t err_size) {
    char root[512];
    _make_root(root, sizeof(root), port_json, device_json);
    int rc = _load_root(root, model, err, err_size);
    _rm_tree(root);
    return rc;
}

static int _count_rows(sqlite3 *db, const char *table) {
    char sql[64];
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM %s;", table);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL), SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

/* 同一组字节分别过 loader 与官方 validator。validator 拒绝时 loader 必须拒绝；
 * loader 可以更严（地址/传输/空集），所以不要求反向相等。 */
static void _differential_case(const void *device, size_t len, int expect_loader_accept) {
    char root[512];
    _make_raw_root(root, sizeof(root), k_uart_can_port, device, len);
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int loader_rc = _load_root(root, model, err, sizeof(err));
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "%s -I %s --mode converted --root %s >/dev/null 2>&1",
             IEMS_CONFIG_PYTHON, IEMS_CONFIG_SCHEMA_TOOL, root);
    int validator_rc = system(cmd);
    _rm_tree(root);
    free(model);
    if (validator_rc != 0) {
        assert_int_equal(loader_rc, -1);
    }
    if (expect_loader_accept) {
        assert_int_equal(loader_rc, 0);
    }
}

static void test_negative_missing_schema_version(void **state) {
    (void)state;
    _expect_load_error(k_valid_port, "{\"dev_class\":[]}", "MISSING_SCHEMA_VERSION", NULL);
}

static void test_negative_unsupported_schema_version(void **state) {
    (void)state;
    _expect_load_error(k_valid_port, "{\"schema_version\":\"2.0.0\",\"dev_class\":[]}",
                       "UNSUPPORTED_SCHEMA_VERSION", NULL);
}

static void test_negative_unknown_key(void **state) {
    (void)state;
    _expect_load_error(
        k_valid_port,
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"m1\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\",\"unexpected\":1}]}]}",
        "UNKNOWN_KEY", NULL);
}

static void test_negative_bad_reference(void **state) {
    (void)state;
    _expect_load_error(
        k_valid_port,
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"m1\",\"channel\":\"RS485_99\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\"}]}]}",
        "BAD_REFERENCE", NULL);
}

static void test_negative_plaintext_secret(void **state) {
    (void)state;
    _expect_load_error(
        k_valid_port,
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"m1\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\",\"password\":\"hunter2\"}]}]}",
        "PLAINTEXT_SECRET", "hunter2");
}

static void test_negative_missing_required(void **state) {
    (void)state;
    _expect_load_error(
        k_valid_port,
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"m1\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\"}]}]}",
        "MISSING_REQUIRED", NULL);
}

static void test_negative_envelope(void **state) {
    (void)state;
    _expect_load_error(k_valid_port, "{\"schema_version\":\"1.0.0\",\"config\":[]}",
                       "ENVELOPE_NOT_ALLOWED", NULL);
}

static void test_negative_unsupported_protocol(void **state) {
    (void)state;
    _expect_load_error(
        k_valid_port,
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"m1\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
        "\"protocol\":\"opcua\"}]}]}",
        "UNSUPPORTED_PROTOCOL", NULL);
}

static void test_negative_invalid_addr(void **state) {
    (void)state;
    _expect_load_error(
        k_valid_port,
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"m1\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\",\"addr\":\"abc\"}]}]}",
        "INVALID_ADDR", NULL);
}

static void test_negative_missing_port_file(void **state) {
    (void)state;
    char root[512];
    _make_root(root, sizeof(root), NULL, k_valid_device);
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    free(model);
    assert_int_equal(rc, -1);
    assert_non_null(strstr(err, "READ_FAILED port_config.json"));
}

static void test_negative_port_unknown_key(void **state) {
    (void)state;
    const char *bad_port =
        "{\"schema_version\":\"1.0.0\",\"dev_cfg\":{"
        "\"lan\":{\"attr\":[{\"ip\":\"10.0.0.1\"}]},"
        "\"uart\":{\"attr\":[{\"name\":\"RS485_1\",\"baud\":9600,\"data_bit\":8,"
        "\"stop_bit\":1,\"parity\":\"N\",\"unexpected\":1}]}}}";
    _expect_load_error(bad_port, k_valid_device, "UNKNOWN_KEY", NULL);
}

static void test_negative_port_missing_lan(void **state) {
    (void)state;
    const char *bad_port =
        "{\"schema_version\":\"1.0.0\",\"dev_cfg\":{"
        "\"uart\":{\"attr\":[{\"name\":\"RS485_1\",\"baud\":9600,\"data_bit\":8,"
        "\"stop_bit\":1,\"parity\":\"N\"}]}}}";
    _expect_load_error(bad_port, k_valid_device, "MISSING_REQUIRED", NULL);
}

/* ------------------------------------------------------------------ */
/* R1: strict JSON precheck                                           */
/* ------------------------------------------------------------------ */

static void test_strict_json_nul_escapes(void **state) {
    (void)state;
    const char *nul_version = "{\"schema_version\":\"1.0.0\\u0000evil\",\"dev_class\":[]}";
    _expect_load_error(k_valid_port, nul_version, "NUL", NULL);
    const char *nul_key =
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"m1\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\",\"type\\u0000password\":\"hunter2\"}]}]}";
    _expect_load_error(k_valid_port, nul_key, "NUL", "hunter2");
}

static void test_strict_json_duplicate_key(void **state) {
    (void)state;
    const char *dup =
        "{\"schema_version\":\"1.0.0\",\"schema_version\":\"2.0.0\","
        "\"dev_class\":[{\"type\":\"METER\",\"devs\":[]}]}";
    _expect_load_error(k_valid_port, dup, "DUPLICATE_KEY", NULL);
}

static void test_strict_json_bom_and_trailing(void **state) {
    (void)state;
    char bom[512];
    bom[0] = (char)0xEF;
    bom[1] = (char)0xBB;
    bom[2] = (char)0xBF;
    size_t valid_len = strlen(k_valid_device);
    memcpy(bom + 3, k_valid_device, valid_len);
    _expect_raw_error(k_valid_port, bom, valid_len + 3, "BOM", NULL);

    char trailing[512];
    snprintf(trailing, sizeof(trailing), "%sx", k_valid_device);
    _expect_raw_error(k_valid_port, trailing, strlen(trailing), "trailing", NULL);
}

static void test_strict_json_raw_bytes(void **state) {
    (void)state;
    const char *prefix =
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"A";
    const char *suffix =
        "B\",\"channel\":\"RS485_1\",\"template\":\"M.json\",\"protocol\":\"modbus_rtu\"}]}]}";
    char raw[512];
    size_t used;
    size_t prefix_len = strlen(prefix);
    size_t suffix_len = strlen(suffix);

    memcpy(raw, prefix, prefix_len);
    used = prefix_len;
    raw[used++] = 0x0A;
    memcpy(raw + used, suffix, suffix_len);
    used += suffix_len;
    _expect_raw_error(k_valid_port, raw, used, "CONTROL_CHARACTER", NULL);

    memcpy(raw, prefix, prefix_len);
    used = prefix_len;
    raw[used++] = (char)0xFF;
    memcpy(raw + used, suffix, suffix_len);
    used += suffix_len;
    _expect_raw_error(k_valid_port, raw, used, "invalid UTF-8", NULL);
}

static void test_strict_json_escaped_control(void **state) {
    (void)state;
    const char *escaped_newline =
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"A\\nB\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\"}]}]}";
    _expect_load_error(k_valid_port, escaped_newline, "CONTROL_CHARACTER", NULL);
}

static void test_strict_json_chinese_name_ok(void **state) {
    (void)state;
    const char *chinese =
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"站点甲\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\",\"user_def\":\"站点乙\"}]}]}";
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    assert_int_equal(_load_device_json(k_valid_port, chinese, model, err, sizeof(err)), 0);
    assert_string_equal(model->devices[0].name, "站点甲");
    assert_string_equal(model->devices[0].user_def, "站点乙");
    free(model);
}

/* ------------------------------------------------------------------ */
/* R2: source OK / ABSENT / ERROR                                     */
/* ------------------------------------------------------------------ */

static void test_source_oversized_optional_file(void **state) {
    (void)state;
    char root[512];
    _make_golden_root(root, sizeof(root));
    _write_fill(root, "lc/device_config.json", ' ', 2u * 1024u * 1024u);
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    free(model);
    assert_int_equal(rc, -1);
    assert_non_null(strstr(err, "READ_FAILED"));
    assert_non_null(strstr(err, "exceeds 1 MiB"));
}

static void test_source_lc_devs_not_directory(void **state) {
    (void)state;
    char root[512];
    _make_golden_root(root, sizeof(root));
    char path[1024];
    snprintf(path, sizeof(path), "%s/lc_devs", root);
    _rm_tree(path);
    _write_file(root, "lc_devs", "x");
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    free(model);
    assert_int_equal(rc, -1);
    assert_non_null(strstr(err, "SOURCE_ERROR"));
    assert_non_null(strstr(err, "not a directory"));
}

/* "lc" as a regular file makes lc/device_config.json unreadable with
 * ENOTDIR; that must be SOURCE_ERROR, never a silent ABSENT. */
static void test_source_lc_is_regular_file(void **state) {
    (void)state;
    char root[512];
    _make_golden_root(root, sizeof(root));
    char path[1024];
    snprintf(path, sizeof(path), "%s/lc", root);
    _rm_tree(path);
    _write_file(root, "lc", "x");
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    free(model);
    assert_int_equal(rc, -1);
    assert_non_null(strstr(err, "SOURCE_ERROR"));
}

static void test_source_optional_file_is_directory(void **state) {
    (void)state;
    char root[512];
    _make_golden_root(root, sizeof(root));
    char path[1024];
    snprintf(path, sizeof(path), "%s/lc/device_config.json", root);
    _rm_tree(path);
    _mkdir_rel(root, "lc/device_config.json");
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    free(model);
    assert_int_equal(rc, -1);
    assert_non_null(strstr(err, "is a directory"));
}

static void test_source_optional_absent_is_ok(void **state) {
    (void)state;
    char root[512];
    _make_golden_root(root, sizeof(root));
    char path[1024];
    snprintf(path, sizeof(path), "%s/lc", root);
    _rm_tree(path);
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    assert_int_equal(rc, 0);
    assert_int_equal(model->device_count, 17);
    free(model);
}

/* ------------------------------------------------------------------ */
/* R3: empty device set                                               */
/* ------------------------------------------------------------------ */

static void test_negative_empty_device_set(void **state) {
    (void)state;
    _expect_load_error(k_valid_port, "{\"schema_version\":\"1.0.0\",\"dev_class\":[]}",
                       "EMPTY_DEVICE_SET", NULL);
}

/* ------------------------------------------------------------------ */
/* R4/R5: protocol capability table                                   */
/* ------------------------------------------------------------------ */

static void _device_json(char *buf, size_t cap, const char *protocol, const char *channel,
                         const char *addr) {
    snprintf(buf, cap,
             "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
             "\"no\":\"M1\",\"name\":\"m1\",\"channel\":\"%s\",\"template\":\"M.json\","
             "\"protocol\":\"%s\",\"addr\":\"%s\"}]}]}",
             channel, protocol, addr);
}

static void test_addr_width_table(void **state) {
    (void)state;
    static const struct {
        const char *protocol;
        const char *channel;
        const char *addr;
        int accept;
    } cases[] = {
        {"modbus_rtu", "RS485_1", "247", 1},
        {"modbus_rtu", "RS485_1", "248", 0},
        {"modbus_rtu", "RS485_1", "256", 0},
        {"modbus_tcp", "10.0.0.1:502", "255", 1},
        {"modbus_tcp", "10.0.0.1:502", "256", 0},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char device[512];
        _device_json(device, sizeof(device), cases[i].protocol, cases[i].channel, cases[i].addr);
        iems_config_model_t *model = calloc(1, sizeof(*model));
        assert_non_null(model);
        char err[256];
        int rc = _load_device_json(k_uart_can_port, device, model, err, sizeof(err));
        if (cases[i].accept) {
            assert_int_equal(rc, 0);
            assert_int_equal(model->devices[0].addr, atoi(cases[i].addr));
        } else {
            assert_int_equal(rc, -1);
            assert_non_null(strstr(err, "ADDR_OUT_OF_RANGE"));
        }
        free(model);
    }
}

static void test_protocol_transport_table(void **state) {
    (void)state;
    static const struct {
        const char *protocol;
        const char *channel;
    } cases[] = {
        {"modbus_rtu", "CAN0"},
        {"can_raw", "RS485_1"},
        {"dlt645_2007", "CAN0"},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char device[512];
        _device_json(device, sizeof(device), cases[i].protocol, cases[i].channel, "1");
        iems_config_model_t *model = calloc(1, sizeof(*model));
        assert_non_null(model);
        char err[256];
        int rc = _load_device_json(k_uart_can_port, device, model, err, sizeof(err));
        free(model);
        assert_int_equal(rc, -1);
        assert_non_null(strstr(err, "PROTOCOL_TRANSPORT_MISMATCH"));
    }
}

/* ------------------------------------------------------------------ */
/* R6: symlink escape                                                 */
/* ------------------------------------------------------------------ */

static void _make_outside_device(char *dir, size_t cap) {
    char template[] = TMP_PREFIX;
    assert_non_null(mkdtemp(template));
    snprintf(dir, cap, "%s", template);
    _write_file(dir, "device_config.json", k_valid_device);
}

static void test_symlink_escape_lc_dir(void **state) {
    (void)state;
    char root[512], outside[512];
    _make_golden_root(root, sizeof(root));
    _make_outside_device(outside, sizeof(outside));
    char link[1024];
    snprintf(link, sizeof(link), "%s/lc_devs/LC1", root);
    _rm_tree(link);
    _symlink_path(outside, link);
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    _rm_tree(outside);
    free(model);
    assert_int_equal(rc, -1);
    assert_non_null(strstr(err, "UNSAFE_PATH"));
}

static void test_symlink_escape_root_file(void **state) {
    (void)state;
    char root[512], outside[512];
    _make_golden_root(root, sizeof(root));
    _make_outside_device(outside, sizeof(outside));
    char link[1024], target[1024];
    snprintf(link, sizeof(link), "%s/device_config.json", root);
    snprintf(target, sizeof(target), "%s/device_config.json", outside);
    assert_int_equal(remove(link), 0);
    _symlink_path(target, link);
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    int rc = _load_root(root, model, err, sizeof(err));
    _rm_tree(root);
    _rm_tree(outside);
    free(model);
    assert_int_equal(rc, -1);
    assert_non_null(strstr(err, "UNSAFE_PATH"));
}

/* ------------------------------------------------------------------ */
/* R7: rollback guard                                                 */
/* ------------------------------------------------------------------ */

static void test_apply_rollback_on_failure(void **state) {
    (void)state;
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    assert_int_equal(_load_device_json(k_valid_port, k_valid_device, model, err, sizeof(err)), 0);

    char db_path[128];
    snprintf(db_path, sizeof(db_path), "/tmp/iems_cfg_rollback_%d.db", (int)getpid());
    unlink(db_path);
    uv_loop_t *loop = uv_default_loop();
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    assert_int_equal(iems_config_apply_sqlite(model, svc, err, sizeof(err)), 0);

    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    int inventory = _count_rows(db, "inventory");
    int tasks = _count_rows(db, "poll_tasks");
    assert_int_equal(inventory, 1);
    assert_int_equal(tasks, 8);

    assert_int_equal(sqlite3_exec(db,
                                  "CREATE TRIGGER fail_insert BEFORE INSERT ON poll_tasks "
                                  "BEGIN SELECT RAISE(ABORT, 'injected rollback'); END;",
                                  NULL, NULL, NULL),
                     SQLITE_OK);
    assert_int_equal(iems_config_apply_sqlite(model, svc, err, sizeof(err)), -1);
    assert_non_null(strstr(err, "SQL_FAILED"));
    assert_int_equal(_count_rows(db, "inventory"), inventory);
    assert_int_equal(_count_rows(db, "poll_tasks"), tasks);

    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(db, "SELECT protocol FROM inventory LIMIT 1;", -1, &stmt,
                                        NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_string_equal((const char *)sqlite3_column_text(stmt, 0), "ModbusRTUHiCRC");
    sqlite3_finalize(stmt);

    assert_int_equal(sqlite3_exec(db, "DROP TRIGGER fail_insert;", NULL, NULL, NULL), SQLITE_OK);
    assert_int_equal(sqlite_svc_begin_trans(svc), 0);
    assert_int_equal(sqlite_svc_commit_trans(svc), 0);

    sqlite_svc_destroy(svc);
    unlink(db_path);
    free(model);
}

/* ------------------------------------------------------------------ */
/* Differential corpus: official validator vs loader                  */
/* ------------------------------------------------------------------ */

static void test_differential_validator_corpus(void **state) {
    (void)state;
    const char *nul_version = "{\"schema_version\":\"1.0.0\\u0000evil\",\"dev_class\":[]}";
    _differential_case(nul_version, strlen(nul_version), 0);

    const char *nul_key =
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"m1\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\",\"type\\u0000password\":\"hunter2\"}]}]}";
    _differential_case(nul_key, strlen(nul_key), 0);

    const char *dup =
        "{\"schema_version\":\"1.0.0\",\"schema_version\":\"2.0.0\","
        "\"dev_class\":[{\"type\":\"METER\",\"devs\":[]}]}";
    _differential_case(dup, strlen(dup), 0);

    const char *trailing = "{\"schema_version\":\"1.0.0\",\"dev_class\":[]}x";
    _differential_case(trailing, strlen(trailing), 0);

    char bom[512];
    bom[0] = (char)0xEF;
    bom[1] = (char)0xBB;
    bom[2] = (char)0xBF;
    memcpy(bom + 3, k_valid_device, strlen(k_valid_device));
    _differential_case(bom, strlen(k_valid_device) + 3, 0);

    const char *raw_prefix =
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"A";
    const char *raw_suffix =
        "B\",\"channel\":\"RS485_1\",\"template\":\"M.json\",\"protocol\":\"modbus_rtu\"}]}]}";
    char raw[512];
    memcpy(raw, raw_prefix, strlen(raw_prefix));
    raw[strlen(raw_prefix)] = 0x0A;
    memcpy(raw + strlen(raw_prefix) + 1, raw_suffix, strlen(raw_suffix));
    _differential_case(raw, strlen(raw_prefix) + 1 + strlen(raw_suffix), 0);

    memcpy(raw, raw_prefix, strlen(raw_prefix));
    raw[strlen(raw_prefix)] = (char)0xFF;
    memcpy(raw + strlen(raw_prefix) + 1, raw_suffix, strlen(raw_suffix));
    _differential_case(raw, strlen(raw_prefix) + 1 + strlen(raw_suffix), 0);

    const char *escaped_newline =
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"A\\nB\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\"}]}]}";
    _differential_case(escaped_newline, strlen(escaped_newline), 0);

    const char *chinese =
        "{\"schema_version\":\"1.0.0\",\"dev_class\":[{\"type\":\"METER\",\"devs\":[{"
        "\"no\":\"M1\",\"name\":\"站点甲\",\"channel\":\"RS485_1\",\"template\":\"M.json\","
        "\"protocol\":\"modbus_rtu\"}]}]}";
    _differential_case(chinese, strlen(chinese), 1);

    char device[512];
    _device_json(device, sizeof(device), "modbus_rtu", "RS485_1", "248");
    _differential_case(device, strlen(device), 0);
    _device_json(device, sizeof(device), "modbus_tcp", "10.0.0.1:502", "256");
    _differential_case(device, strlen(device), 0);
    _device_json(device, sizeof(device), "can_raw", "RS485_1", "1");
    _differential_case(device, strlen(device), 0);
    _device_json(device, sizeof(device), "modbus_rtu", "CAN0", "1");
    _differential_case(device, strlen(device), 0);
}

/* ------------------------------------------------------------------ */
/* Converter linkage: python3 convert_config.py -> loader             */
/* ------------------------------------------------------------------ */

static void test_linked_converted_output(void **state) {
    (void)state;
    assert_non_null(g_linked_root);
    iems_config_model_t *model = calloc(1, sizeof(*model));
    assert_non_null(model);
    char err[256];
    assert_int_equal(_load_root(g_linked_root, model, err, sizeof(err)), 0);
    assert_int_equal(model->device_count, 18);
    assert_int_equal(model->port_count, 37);
    const iems_config_device_t *d = _device_at(model, 0);
    assert_string_equal(d->protocol, "ModbusRTUHiCRC");
    assert_int_equal(d->bus_id, 0);
    assert_int_equal(d->point_count, 8);
    d = _device_at(model, 4);
    assert_string_equal(d->protocol, "CAN");
    assert_int_equal(d->bus_id, 1);
    assert_int_equal(d->point_count, 2);
    free(model);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        g_linked_root = argv[1];
        const struct CMUnitTest linked[] = {
            cmocka_unit_test(test_linked_converted_output),
        };
        return cmocka_run_group_tests(linked, NULL, NULL);
    }

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_golden_families_load),
        cmocka_unit_test(test_apply_sqlite_and_routes),
        cmocka_unit_test(test_load_apply_entry),
        cmocka_unit_test(test_protocol_from_config),
        cmocka_unit_test(test_secret_reference_is_preserved),
        cmocka_unit_test(test_negative_missing_schema_version),
        cmocka_unit_test(test_negative_unsupported_schema_version),
        cmocka_unit_test(test_negative_unknown_key),
        cmocka_unit_test(test_negative_bad_reference),
        cmocka_unit_test(test_negative_plaintext_secret),
        cmocka_unit_test(test_negative_missing_required),
        cmocka_unit_test(test_negative_envelope),
        cmocka_unit_test(test_negative_unsupported_protocol),
        cmocka_unit_test(test_negative_invalid_addr),
        cmocka_unit_test(test_negative_missing_port_file),
        cmocka_unit_test(test_negative_port_unknown_key),
        cmocka_unit_test(test_negative_port_missing_lan),
        cmocka_unit_test(test_strict_json_nul_escapes),
        cmocka_unit_test(test_strict_json_duplicate_key),
        cmocka_unit_test(test_strict_json_bom_and_trailing),
        cmocka_unit_test(test_strict_json_raw_bytes),
        cmocka_unit_test(test_strict_json_escaped_control),
        cmocka_unit_test(test_strict_json_chinese_name_ok),
        cmocka_unit_test(test_source_oversized_optional_file),
        cmocka_unit_test(test_source_lc_devs_not_directory),
        cmocka_unit_test(test_source_lc_is_regular_file),
        cmocka_unit_test(test_source_optional_file_is_directory),
        cmocka_unit_test(test_source_optional_absent_is_ok),
        cmocka_unit_test(test_negative_empty_device_set),
        cmocka_unit_test(test_addr_width_table),
        cmocka_unit_test(test_protocol_transport_table),
        cmocka_unit_test(test_symlink_escape_lc_dir),
        cmocka_unit_test(test_symlink_escape_root_file),
        cmocka_unit_test(test_apply_rollback_on_failure),
        cmocka_unit_test(test_differential_validator_corpus),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
