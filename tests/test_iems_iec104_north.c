/**
 * @file test_iems_iec104_north.c
 * @brief 北向 IEC104 服务端：端口映射单测 + TCP 回环（STARTDT/TESTFR/GI/遥控）
 *
 * 回环测试连的是真实监听器与真实驱动，不做任何 socket mock；断言看的是
 * 主站能观察到的 APCI/ASDU 字节与最终落点，不是内部调用。
 */

#include <arpa/inet.h>
#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

#include "bus_datahub_adapter.h"
#include "driver_manager.h"
#include "iec_asdu.h"
#include "iems_driver_adapter.h"
#include "iems_node.h"
#include "iems_north_iec104.h"
#include "sqlite_service.h"
#include "thread_port_uv.h"

typedef struct {
    int calls;
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    double value;
} fake_exec_t;

typedef struct {
    uv_loop_t loop;
    iems_node_storage_t node_storage;
    iems_node_t *node;
    datahub_t *hub;
    arbiter_t *arb;
    bus_orchestrator_t *orch;
    driver_manager_t *mgr;
    driver_manager_storage_t mgr_storage;
    sqlite_svc_t *db;
    char db_path[64];
    iems_north_iec104_t listener;
    fake_exec_t exec;
} fixture_t;

static int fake_write(void *ctx, int bus_id, uint32_t slave_id, uint32_t tag_id, double value,
                      const char *proto, iems_write_done_fn on_done, void *on_done_user) {
    (void)proto;
    (void)on_done;
    (void)on_done_user;
    fake_exec_t *exec = (fake_exec_t *)ctx;
    exec->calls++;
    exec->bus_id = bus_id;
    exec->slave_id = slave_id;
    exec->tag_id = tag_id;
    exec->value = value;
    return 0;
}

static int g_setup_seq = 0;

static int setup(void **state) {
    fixture_t *f = calloc(1, sizeof(fixture_t));
    assert_non_null(f);
    assert_int_equal(uv_loop_init(&f->loop), 0);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "IEMS_IEC104_TEST_%d_%d", (int)getpid(), g_setup_seq++);
    iems_node_config_t node_cfg = {
        .product_name = "IEMS_IEC104_TEST",
        .shm_name = shm_name,
        .datahub_capacity = 1024,
        .max_pcs_power_kw = 500.0,
        .transformer_capacity_kva = 630.0,
        .demand_limit_kw = 500.0,
    };
    f->node = iems_node_create_in(&f->node_storage, &f->loop, &node_cfg);
    assert_non_null(f->node);
    f->hub = iems_node_get_datahub(f->node);
    assert_non_null(f->hub);
    assert_int_equal(
        iems_node_set_execution_port(
            f->node, (iems_execution_port_t){.ctx = &f->exec, .write_tag = fake_write}),
        0);

    f->mgr = driver_manager_create_in(&f->mgr_storage);
    assert_non_null(f->mgr);
    extern int iec104_driver_register(driver_manager_t * mgr);
    assert_int_equal(iec104_driver_register(f->mgr), 0);

    assert_int_equal(arbiter_init(&f->arb, &f->loop), 0);
    arbiter_resume(f->arb);
    f->orch = bus_orchestrator_create_hub_default(
        f->hub, f->arb, driver_manager_session_port_ptr(f->mgr), 64, 256, 2048);
    assert_non_null(f->orch);
    assert_int_equal(datahub_bind_io_pool(f->hub, bus_orchestrator_get_pool(f->orch)), 0);

    snprintf(f->db_path, sizeof(f->db_path), "test_iems_iec104_north_%d_%d.db", (int)getpid(),
             g_setup_seq);
    unlink(f->db_path);
    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = f->db_path};
    f->db = sqlite_svc_create(&f->loop, &db_cfg);
    assert_non_null(f->db);
    sqlite3 *raw = (sqlite3 *)sqlite_svc_get_db_handle(f->db);
    assert_non_null(raw);
    const char *sql =
        "CREATE TABLE IF NOT EXISTS inventory (id INTEGER PRIMARY KEY, addr TEXT, protocol TEXT);"
        "CREATE TABLE IF NOT EXISTS poll_tasks (bus_id INTEGER, device_id INTEGER, tag_id INTEGER);"
        "INSERT INTO inventory (id, addr, protocol) VALUES (1, '7', 'MODBUS');"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 1, 1003);";
    assert_int_equal(sqlite3_exec(raw, sql, NULL, NULL, NULL), SQLITE_OK);

    /* 静态点表的影子值；GI 应逐点取这些值。 */
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, IEMS_TAG_GRID_POWER, 1234.5, 0);
    assert_int_equal(datahub_publish(f->hub, &msg), 0);
    pipe_msg_init_scalar(&msg, IEMS_TAG_VOLTAGE_PHASE_A, 231.7, 0);
    assert_int_equal(datahub_publish(f->hub, &msg), 0);
    pipe_msg_init_scalar(&msg, IEMS_TAG_CURRENT_PHASE_A, 42.25, 0);
    assert_int_equal(datahub_publish(f->hub, &msg), 0);
    pipe_msg_init_scalar(&msg, IEMS_TAG_BATTERY_POWER, -500.5, 0);
    assert_int_equal(datahub_publish(f->hub, &msg), 0);

    /* 契约表里有 type=IEC104 的北向条目；端口 0 让内核分配，端口从监听器读回。 */
    assert_int_equal(iems_north_iec104_init(&f->listener, &f->loop, f->hub, f->orch, f->node, f->db,
                                            "127.0.0.1", 0),
                     0);
    assert_true(f->listener.port > 0);
    assert_true(f->listener.bus_id >= 0);

    *state = f;
    return 0;
}

static void force_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int teardown(void **state) {
    fixture_t *f = (fixture_t *)*state;
    if (!f) return 0;

    iems_north_iec104_close(&f->listener);
    for (int i = 0; i < 20; i++) uv_run(&f->loop, UV_RUN_NOWAIT);

    if (f->db) sqlite_svc_destroy(f->db);
    unlink(f->db_path);
    bus_orchestrator_destroy(f->orch);
    if (f->arb) arbiter_destroy(f->arb);
    if (f->mgr) driver_manager_destroy(f->mgr);
    if (f->node) iems_node_destroy(f->node);

    uv_walk(&f->loop, force_close, NULL);
    for (int i = 0; i < 200 && uv_loop_alive(&f->loop); i++) uv_run(&f->loop, UV_RUN_NOWAIT);
    uv_loop_close(&f->loop);
    free(f);
    return 0;
}

/* ==================== 源端口映射 ==================== */

static void test_source_ops_map_static_points(void **state) {
    fixture_t *f = (fixture_t *)*state;
    const edge_slave_source_ops_t *ops = iems_north_iec104_source_ops();
    assert_non_null(ops->resolve);
    assert_non_null(ops->accept);
    assert_non_null(ops->enumerate);

    /* IOA 16386 → tag 1001（电压）；resolve 读影子值。 */
    edge_point_value_t v;
    memset(&v, 0, sizeof(v));
    assert_int_equal(
        ops->resolve(&f->listener.src, IEC_NATIVE_ID(IEC_TYPE_M_ME_NC_1, 16386), NULL, 0, &v), 0);
    assert_true(fabs(v.value - 231.7) < 1e-9);

    /* 未映射的 IOA 不猜，直接拒绝。 */
    assert_int_not_equal(
        ops->resolve(&f->listener.src, IEC_NATIVE_ID(IEC_TYPE_M_ME_NC_1, 99), NULL, 0, &v), 0);

    /* 按点死区来自静态点表：电压 0.5、有功 1.0；未映射返回负值。 */
    assert_non_null(ops->deadband);
    assert_true(fabs(ops->deadband(&f->listener.src, IEC_NATIVE_ID(IEC_TYPE_M_ME_NC_1, 16386)) -
                     0.5) < 1e-9);
    assert_true(fabs(ops->deadband(&f->listener.src, IEC_NATIVE_ID(IEC_TYPE_M_ME_NC_1, 16385)) -
                     1.0) < 1e-9);
    assert_true(ops->deadband(&f->listener.src, IEC_NATIVE_ID(IEC_TYPE_M_ME_NC_1, 99)) < 0.0);

    /* v3 的控制地址段：RemoteCtrl 24577+20n、RemoteRegu 25089+500n。 */
    assert_int_equal(iems_iec104_control_space(24577), IEC104_CONTROL_SPACE_REMOTE_CTRL);
    assert_int_equal(iems_iec104_control_space(24596), IEC104_CONTROL_SPACE_REMOTE_CTRL);
    assert_int_equal(iems_iec104_control_space(25088), IEC104_CONTROL_SPACE_REMOTE_CTRL);
    assert_int_equal(iems_iec104_control_space(25089), IEC104_CONTROL_SPACE_REMOTE_REGU);
    assert_int_equal(iems_iec104_control_space(25588), IEC104_CONTROL_SPACE_REMOTE_REGU);
    assert_int_equal(iems_iec104_control_space(16385), IEC104_CONTROL_SPACE_NONE);
    assert_null(iems_lookup_iec104_control(24577));
    assert_null(iems_lookup_iec104_control(25089));

    /* 遥测地址不是控制点：拒绝，不落写。 */
    assert_int_not_equal(
        ops->accept(&f->listener.src, IEC_NATIVE_ID(IEC_TYPE_C_SC_NA_1, 16385), 1.0), 0);
    assert_int_equal(f->exec.calls, 0);

    /* 控制段内但点表未入库：同样拒绝。 */
    assert_int_not_equal(
        ops->accept(&f->listener.src, IEC_NATIVE_ID(IEC_TYPE_C_SC_NA_1, 24577), 1.0), 0);
    assert_int_not_equal(
        ops->accept(&f->listener.src, IEC_NATIVE_ID(IEC_TYPE_C_SE_NC_1, 25089), 12.0), 0);
    assert_int_equal(f->exec.calls, 0);

    /* 段外 IOA：拒绝。 */
    assert_int_not_equal(ops->accept(&f->listener.src, IEC_NATIVE_ID(IEC_TYPE_C_SC_NA_1, 99), 1.0),
                         0);
    assert_int_equal(f->exec.calls, 0);
}

/* ==================== TCP 回环 ==================== */

static int tcp_connect_loopback(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(fd >= 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    assert_int_equal(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr), 1);
    assert_int_equal(connect(fd, (struct sockaddr *)&addr, sizeof(addr)), 0);
    int flags = fcntl(fd, F_GETFL, 0);
    assert_true(flags >= 0);
    assert_int_equal(fcntl(fd, F_SETFL, flags | O_NONBLOCK), 0);
    return fd;
}

static void send_all(int fd, const uint8_t *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = send(fd, buf + off, len - off, MSG_NOSIGNAL);
        assert_true(n > 0);
        off += (size_t)n;
    }
}

/* 客户端与服务器共用一条线程：靠事件循环推进 accept/读/写，再从 socket 收。
 * timeout_ms 是泵事件循环的上限；收满 want 字节立即返回。 */
static size_t recv_until(uv_loop_t *loop, int fd, uint8_t *out, size_t cap, size_t want,
                         int timeout_ms) {
    size_t got = 0;
    for (int i = 0; i < timeout_ms && got < want; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        ssize_t n = recv(fd, out + got, cap - got, MSG_DONTWAIT);
        if (n > 0) {
            got += (size_t)n;
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            break;
        }
        usleep(1000);
    }
    return got;
}

/* GI 响应 = ACT_CON(16) + 一条 4 对象 INTROGEN ASDU(44) + ACT_TERM(16)。 */
#define GI_RESPONSE_LEN 76u

static void test_tcp_loopback_iec104_session(void **state) {
    fixture_t *f = (fixture_t *)*state;
    int fd = tcp_connect_loopback(f->listener.port);
    uint8_t rsp[512];

    static const uint8_t startdt_act[6] = {0x68, 0x04, 0x07, 0x00, 0x00, 0x00};
    static const uint8_t startdt_con[6] = {0x68, 0x04, 0x0B, 0x00, 0x00, 0x00};
    send_all(fd, startdt_act, sizeof(startdt_act));
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(startdt_con), 3000), 6);
    assert_memory_equal(rsp, startdt_con, sizeof(startdt_con));

    static const uint8_t testfr_act[6] = {0x68, 0x04, 0x43, 0x00, 0x00, 0x00};
    static const uint8_t testfr_con[6] = {0x68, 0x04, 0x83, 0x00, 0x00, 0x00};
    send_all(fd, testfr_act, sizeof(testfr_act));
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(testfr_con), 3000), 6);
    assert_memory_equal(rsp, testfr_con, sizeof(testfr_con));

    /* C_IC_NA_1 ACT, CASDU 1, QOI 20 */
    static const uint8_t gi_act[16] = {0x68, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x64, 0x01,
                                       0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14};
    send_all(fd, gi_act, sizeof(gi_act));
    size_t got = recv_until(&f->loop, fd, rsp, sizeof(rsp), GI_RESPONSE_LEN, 3000);
    assert_int_equal(got, GI_RESPONSE_LEN);

    /* 确认帧回显请求 ASDU：带 IOA(0) 与 QOI(20)，COT 6→7 / 6→10。 */
    static const uint8_t gi_con[16] = {0x68, 0x0E, 0x00, 0x00, 0x02, 0x00, 0x64, 0x01,
                                       0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14};
    static const uint8_t gi_term[16] = {0x68, 0x0E, 0x04, 0x00, 0x02, 0x00, 0x64, 0x01,
                                        0x0A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14};
    /* v3 按类型分组：一条 ASDU 带 4 个对象（VSQ=4），顺序为点表顺序。 */
    static const uint8_t gi_data[44] = {
        0x68, 0x2A, 0x02, 0x00, 0x02, 0x00, 0x0D, 0x04, 0x14, 0x00, 0x01, 0x00,
        0x01, 0x40, 0x00, 0x00, 0x50, 0x9A, 0x44, 0x00, 0x02, 0x40, 0x00, 0x33,
        0xB3, 0x67, 0x43, 0x00, 0x03, 0x40, 0x00, 0x00, 0x00, 0x29, 0x42, 0x00,
        0x04, 0x40, 0x00, 0x00, 0x40, 0xFA, 0xC3, 0x00,
    };
    assert_memory_equal(rsp, gi_con, sizeof(gi_con));
    assert_memory_equal(rsp + 16, gi_data, sizeof(gi_data));
    assert_memory_equal(rsp + 60, gi_term, sizeof(gi_term));

    /* QOI 21（组召唤）v3 不做全站召唤，回负 ACT_CON（COT=7 + P/N 位，线上首字节
     * 0x47），无数据、无 ACT_TERM。 */
    static const uint8_t gi_qoi21[16] = {0x68, 0x0E, 0x02, 0x00, 0x06, 0x00, 0x64, 0x01,
                                         0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x15};
    static const uint8_t gi_qoi21_con[16] = {0x68, 0x0E, 0x06, 0x00, 0x04, 0x00, 0x64, 0x01,
                                             0x47, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x15};
    send_all(fd, gi_qoi21, sizeof(gi_qoi21));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(gi_qoi21_con), 3000);
    assert_int_equal(got, sizeof(gi_qoi21_con));
    assert_memory_equal(rsp, gi_qoi21_con, sizeof(gi_qoi21_con));
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), 1, 600), 0);

    /* CASDU 2 不在配置里：v3 先回正 ACT_CON，再直接 ACT_TERM，不发数据。 */
    static const uint8_t gi_ca2[16] = {0x68, 0x0E, 0x04, 0x00, 0x08, 0x00, 0x64, 0x01,
                                       0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x14};
    static const uint8_t gi_ca2_rsp[32] = {
        0x68, 0x0E, 0x08, 0x00, 0x06, 0x00, 0x64, 0x01, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x14,
        0x68, 0x0E, 0x0A, 0x00, 0x06, 0x00, 0x64, 0x01, 0x0A, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x14,
    };
    send_all(fd, gi_ca2, sizeof(gi_ca2));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(gi_ca2_rsp), 3000);
    assert_int_equal(got, sizeof(gi_ca2_rsp));
    assert_memory_equal(rsp, gi_ca2_rsp, sizeof(gi_ca2_rsp));
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), 1, 600), 0);

    close(fd);
    uv_run(&f->loop, UV_RUN_NOWAIT);
}

/* 命令确认：回显请求 ASDU（带 IOA 与命令值），COT 按 v3 的规则改写。
 * 新连接，序号从 0 起。 */
static void test_tcp_loopback_command_confirm(void **state) {
    fixture_t *f = (fixture_t *)*state;
    int fd = tcp_connect_loopback(f->listener.port);
    uint8_t rsp[64];

    static const uint8_t startdt_act[6] = {0x68, 0x04, 0x07, 0x00, 0x00, 0x00};
    static const uint8_t startdt_con[6] = {0x68, 0x04, 0x0B, 0x00, 0x00, 0x00};
    send_all(fd, startdt_act, sizeof(startdt_act));
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(startdt_con), 3000), 6);
    assert_memory_equal(rsp, startdt_con, sizeof(startdt_con));

    /* C_SC_NA_1 ACT, IOA 16385 是遥测地址，不在 v3 遥控段：按未映射回 COT=47，
     * 不落写。 */
    static const uint8_t cmd_act[16] = {0x68, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x01,
                                        0x06, 0x00, 0x01, 0x00, 0x01, 0x40, 0x00, 0x01};
    static const uint8_t cmd_act_con[16] = {0x68, 0x0E, 0x00, 0x00, 0x02, 0x00, 0x2D, 0x01,
                                            0x2F, 0x00, 0x01, 0x00, 0x01, 0x40, 0x00, 0x01};
    send_all(fd, cmd_act, sizeof(cmd_act));
    size_t got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(cmd_act_con), 3000);
    assert_int_equal(got, sizeof(cmd_act_con));
    assert_memory_equal(rsp, cmd_act_con, sizeof(cmd_act_con));
    assert_int_equal(f->exec.calls, 0);

    /* v3 遥控段首地址 24577：地址段内，但静态控制点表没有该点，同样回 47。 */
    static const uint8_t cmd_rc[16] = {0x68, 0x0E, 0x02, 0x00, 0x02, 0x00, 0x2D, 0x01,
                                       0x06, 0x00, 0x01, 0x00, 0x01, 0x60, 0x00, 0x01};
    static const uint8_t cmd_rc_con[16] = {0x68, 0x0E, 0x02, 0x00, 0x04, 0x00, 0x2D, 0x01,
                                           0x2F, 0x00, 0x01, 0x00, 0x01, 0x60, 0x00, 0x01};
    send_all(fd, cmd_rc, sizeof(cmd_rc));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(cmd_rc_con), 3000);
    assert_int_equal(got, sizeof(cmd_rc_con));
    assert_memory_equal(rsp, cmd_rc_con, sizeof(cmd_rc_con));
    assert_int_equal(f->exec.calls, 0);

    /* v3 遥调段首地址 25089，C_SE_NC 同样未映射 → 47。 */
    static const uint8_t se_rr[20] = {0x68, 0x12, 0x04, 0x00, 0x04, 0x00, 0x32, 0x01, 0x06, 0x00,
                                      0x01, 0x00, 0x01, 0x62, 0x00, 0x00, 0x00, 0xA0, 0x41, 0x00};
    static const uint8_t se_rr_con[20] = {0x68, 0x12, 0x04, 0x00, 0x06, 0x00, 0x32, 0x01, 0x2F, 0x00,
                                          0x01, 0x00, 0x01, 0x62, 0x00, 0x00, 0x00, 0xA0, 0x41, 0x00};
    send_all(fd, se_rr, sizeof(se_rr));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(se_rr_con), 3000);
    assert_int_equal(got, sizeof(se_rr_con));
    assert_memory_equal(rsp, se_rr_con, sizeof(se_rr_con));
    assert_int_equal(f->exec.calls, 0);

    /* 完全在控制段之外的 IOA：同样未映射 → 47。 */
    static const uint8_t cmd_unmapped[16] = {0x68, 0x0E, 0x06, 0x00, 0x06, 0x00, 0x2D, 0x01,
                                             0x06, 0x00, 0x01, 0x00, 0x63, 0x00, 0x00, 0x01};
    static const uint8_t cmd_unmapped_con[16] = {0x68, 0x0E, 0x06, 0x00, 0x08, 0x00, 0x2D, 0x01,
                                                 0x2F, 0x00, 0x01, 0x00, 0x63, 0x00, 0x00, 0x01};
    send_all(fd, cmd_unmapped, sizeof(cmd_unmapped));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(cmd_unmapped_con), 3000);
    assert_int_equal(got, sizeof(cmd_unmapped_con));
    assert_memory_equal(rsp, cmd_unmapped_con, sizeof(cmd_unmapped_con));
    assert_int_equal(f->exec.calls, 0);

    /* C_DC_NA_1 ON(2)：v3 没有 C_DC 分支，回 COT=44；DCO 原样回显（不许截成 0）。 */
    static const uint8_t dc_act[16] = {0x68, 0x0E, 0x08, 0x00, 0x08, 0x00, 0x2E, 0x01,
                                       0x06, 0x00, 0x01, 0x00, 0x01, 0x40, 0x00, 0x02};
    static const uint8_t dc_con[16] = {0x68, 0x0E, 0x08, 0x00, 0x0A, 0x00, 0x2E, 0x01,
                                       0x2C, 0x00, 0x01, 0x00, 0x01, 0x40, 0x00, 0x02};
    send_all(fd, dc_act, sizeof(dc_act));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(dc_con), 3000);
    assert_int_equal(got, sizeof(dc_con));
    assert_memory_equal(rsp, dc_con, sizeof(dc_con));
    assert_int_equal(f->exec.calls, 0);

    /* C_DC_NA_1 OFF(1)：同样不执行，回 COT=44。 */
    static const uint8_t dc_off_act[16] = {0x68, 0x0E, 0x0A, 0x00, 0x0A, 0x00, 0x2E, 0x01,
                                           0x06, 0x00, 0x01, 0x00, 0x01, 0x40, 0x00, 0x01};
    static const uint8_t dc_off_con[16] = {0x68, 0x0E, 0x0A, 0x00, 0x0C, 0x00, 0x2E, 0x01,
                                           0x2C, 0x00, 0x01, 0x00, 0x01, 0x40, 0x00, 0x01};
    send_all(fd, dc_off_act, sizeof(dc_off_act));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(dc_off_con), 3000);
    assert_int_equal(got, sizeof(dc_off_con));
    assert_memory_equal(rsp, dc_off_con, sizeof(dc_off_con));
    assert_int_equal(f->exec.calls, 0);

    /* 非 ACT 的命令不执行，回 COT=45。 */
    static const uint8_t cmd_wrong_cot[16] = {0x68, 0x0E, 0x0C, 0x00, 0x0C, 0x00, 0x2D, 0x01,
                                              0x03, 0x00, 0x01, 0x00, 0x01, 0x40, 0x00, 0x01};
    static const uint8_t cmd_wrong_cot_con[16] = {0x68, 0x0E, 0x0C, 0x00, 0x0E, 0x00, 0x2D, 0x01,
                                                  0x2D, 0x00, 0x01, 0x00, 0x01, 0x40, 0x00, 0x01};
    send_all(fd, cmd_wrong_cot, sizeof(cmd_wrong_cot));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(cmd_wrong_cot_con), 3000);
    assert_int_equal(got, sizeof(cmd_wrong_cot_con));
    assert_memory_equal(rsp, cmd_wrong_cot_con, sizeof(cmd_wrong_cot_con));
    assert_int_equal(f->exec.calls, 0);

    /* CASDU 2 未配置：v3 在 COT 检查之后回 COT=46，不落写。 */
    static const uint8_t cmd_ca2[16] = {0x68, 0x0E, 0x0E, 0x00, 0x0E, 0x00, 0x2D, 0x01,
                                        0x06, 0x00, 0x02, 0x00, 0x01, 0x40, 0x00, 0x01};
    static const uint8_t cmd_ca2_con[16] = {0x68, 0x0E, 0x0E, 0x00, 0x10, 0x00, 0x2D, 0x01,
                                            0x2E, 0x00, 0x02, 0x00, 0x01, 0x40, 0x00, 0x01};
    send_all(fd, cmd_ca2, sizeof(cmd_ca2));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(cmd_ca2_con), 3000);
    assert_int_equal(got, sizeof(cmd_ca2_con));
    assert_memory_equal(rsp, cmd_ca2_con, sizeof(cmd_ca2_con));

    static const uint8_t se_ca2[20] = {0x68, 0x12, 0x10, 0x00, 0x10, 0x00, 0x32, 0x01, 0x06, 0x00,
                                       0x02, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x48, 0x41, 0x00};
    static const uint8_t se_ca2_con[20] = {0x68, 0x12, 0x10, 0x00, 0x12, 0x00, 0x32, 0x01, 0x2E, 0x00,
                                           0x02, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x48, 0x41, 0x00};
    send_all(fd, se_ca2, sizeof(se_ca2));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(se_ca2_con), 3000);
    assert_int_equal(got, sizeof(se_ca2_con));
    assert_memory_equal(rsp, se_ca2_con, sizeof(se_ca2_con));

    static const uint8_t dc_ca2[16] = {0x68, 0x0E, 0x12, 0x00, 0x12, 0x00, 0x2E, 0x01,
                                       0x06, 0x00, 0x02, 0x00, 0x01, 0x40, 0x00, 0x02};
    static const uint8_t dc_ca2_con[16] = {0x68, 0x0E, 0x12, 0x00, 0x14, 0x00, 0x2E, 0x01,
                                           0x2E, 0x00, 0x02, 0x00, 0x01, 0x40, 0x00, 0x02};
    send_all(fd, dc_ca2, sizeof(dc_ca2));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(dc_ca2_con), 3000);
    assert_int_equal(got, sizeof(dc_ca2_con));
    assert_memory_equal(rsp, dc_ca2_con, sizeof(dc_ca2_con));

    /* COT 与 CA 同时不对：v3 先看 COT，回 45 而不是 46。 */
    static const uint8_t cmd_ca2_wrong_cot[16] = {0x68, 0x0E, 0x14, 0x00, 0x14, 0x00, 0x2D, 0x01,
                                                  0x03, 0x00, 0x02, 0x00, 0x01, 0x40, 0x00, 0x01};
    static const uint8_t cmd_ca2_wrong_cot_con[16] = {0x68, 0x0E, 0x14, 0x00, 0x16, 0x00, 0x2D, 0x01,
                                                      0x2D, 0x00, 0x02, 0x00, 0x01, 0x40, 0x00, 0x01};
    send_all(fd, cmd_ca2_wrong_cot, sizeof(cmd_ca2_wrong_cot));
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(cmd_ca2_wrong_cot_con), 3000);
    assert_int_equal(got, sizeof(cmd_ca2_wrong_cot_con));
    assert_memory_equal(rsp, cmd_ca2_wrong_cot_con, sizeof(cmd_ca2_wrong_cot_con));
    assert_int_equal(f->exec.calls, 0);

    close(fd);
    uv_run(&f->loop, UV_RUN_NOWAIT);
}

/* 从站 tick 已接上：影子值变化 → 自发 ASDU（COT=3）。 */
static void test_tcp_loopback_spontaneous_report(void **state) {
    fixture_t *f = (fixture_t *)*state;
    int fd = tcp_connect_loopback(f->listener.port);
    uint8_t rsp[64];

    static const uint8_t startdt_act[6] = {0x68, 0x04, 0x07, 0x00, 0x00, 0x00};
    static const uint8_t startdt_con[6] = {0x68, 0x04, 0x0B, 0x00, 0x00, 0x00};
    send_all(fd, startdt_act, sizeof(startdt_act));
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(startdt_con), 3000), 6);
    assert_memory_equal(rsp, startdt_con, sizeof(startdt_con));

    /* 至少两个 tick 建基线（电压 231.7、有功 1234.5），不应有下行帧。 */
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), 1, 2500), 0);

    /* 电压死区 0.5：+0.6 越界；有功死区 1.0：+0.7 在死区内。
     * 同一次采样只有电压成帧——死区按点生效。 */
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, IEMS_TAG_VOLTAGE_PHASE_A, 232.3, 0);
    assert_int_equal(datahub_publish(f->hub, &msg), 0);
    pipe_msg_init_scalar(&msg, IEMS_TAG_GRID_POWER, 1235.2, 0);
    assert_int_equal(datahub_publish(f->hub, &msg), 0);

    static const uint8_t spont_v[20] = {0x68, 0x12, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x01, 0x03, 0x00,
                                        0x01, 0x00, 0x02, 0x40, 0x00, 0xCD, 0x4C, 0x68, 0x43, 0x00};
    size_t got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(spont_v), 3000);
    assert_int_equal(got, sizeof(spont_v));
    assert_memory_equal(rsp, spont_v, sizeof(spont_v));
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), 1, 600), 0);

    /* 反向：电压 +0.1 在死区内；有功再 +1.1 越界。只有有功成帧。 */
    pipe_msg_init_scalar(&msg, IEMS_TAG_VOLTAGE_PHASE_A, 232.4, 0);
    assert_int_equal(datahub_publish(f->hub, &msg), 0);
    pipe_msg_init_scalar(&msg, IEMS_TAG_GRID_POWER, 1236.3, 0);
    assert_int_equal(datahub_publish(f->hub, &msg), 0);

    static const uint8_t spont_p[20] = {0x68, 0x12, 0x02, 0x00, 0x00, 0x00, 0x0D, 0x01, 0x03, 0x00,
                                        0x01, 0x00, 0x01, 0x40, 0x00, 0x9A, 0x89, 0x9A, 0x44, 0x00};
    got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(spont_p), 3000);
    assert_int_equal(got, sizeof(spont_p));
    assert_memory_equal(rsp, spont_p, sizeof(spont_p));
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), 1, 600), 0);

    close(fd);
    uv_run(&f->loop, UV_RUN_NOWAIT);
}

/* 从站 tick 已接上：空闲 t3(20s) 后主动发 TESTFR ACT；客户端用 S 帧确认 t1。 */
static void test_tcp_loopback_testfr_keepalive(void **state) {
    fixture_t *f = (fixture_t *)*state;
    int fd = tcp_connect_loopback(f->listener.port);
    uint8_t rsp[128];

    static const uint8_t startdt_act[6] = {0x68, 0x04, 0x07, 0x00, 0x00, 0x00};
    static const uint8_t startdt_con[6] = {0x68, 0x04, 0x0B, 0x00, 0x00, 0x00};
    send_all(fd, startdt_act, sizeof(startdt_act));
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(startdt_con), 3000), 6);
    assert_memory_equal(rsp, startdt_con, sizeof(startdt_con));

    /* GI 让服务端发出 I 帧，t3 从最后一条 I 帧起算。 */
    static const uint8_t gi_act[16] = {0x68, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x64, 0x01,
                                       0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14};
    send_all(fd, gi_act, sizeof(gi_act));
    assert_int_equal(recv_until(&f->loop, fd, rsp, sizeof(rsp), GI_RESPONSE_LEN, 3000),
                     GI_RESPONSE_LEN);

    /* S 帧 N(R)=3 确认服务端 3 条 I 帧；不确认的话 t1 会先到期。 */
    static const uint8_t s_ack[6] = {0x68, 0x04, 0x01, 0x00, 0x06, 0x00};
    send_all(fd, s_ack, sizeof(s_ack));

    static const uint8_t testfr_act[6] = {0x68, 0x04, 0x43, 0x00, 0x00, 0x00};
    size_t got = recv_until(&f->loop, fd, rsp, sizeof(rsp), sizeof(testfr_act), 30000);
    assert_int_equal(got, sizeof(testfr_act));
    assert_memory_equal(rsp, testfr_act, sizeof(testfr_act));

    close(fd);
    uv_run(&f->loop, UV_RUN_NOWAIT);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    edge_thread_port_install_uv();

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_source_ops_map_static_points, setup, teardown),
        cmocka_unit_test_setup_teardown(test_tcp_loopback_iec104_session, setup, teardown),
        cmocka_unit_test_setup_teardown(test_tcp_loopback_command_confirm, setup, teardown),
        cmocka_unit_test_setup_teardown(test_tcp_loopback_spontaneous_report, setup, teardown),
        cmocka_unit_test_setup_teardown(test_tcp_loopback_testfr_keepalive, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
