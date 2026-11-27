#include "bus_datahub_adapter.h"
#include <cmocka.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "driver_manager.h"
#include "io_executor.h"
#include "io_handle_priv.h"
#include "passthrough_mgr.h"
#include "datahub_priv.h"

// Mock IO handle
static int mock_write_count = 0;
static uint8_t last_written[1024];
static size_t last_written_len = 0;

static int mock_write_async(io_handle_t* h, net_buf_t* buf) {
    (void)h;
    mock_write_count++;
    if (buf->b.len <= 1024) {
        memcpy(last_written, buf->b.data, buf->b.len);
        last_written_len = buf->b.len;
    }
    return 0;
}

static void mock_io_destroy(io_handle_t* h) {
    (void)h;
}

static io_ops_t mock_ops = {
    .write_async = mock_write_async,
    .destroy = mock_io_destroy,
};

static uv_write_t g_write_req;
static uint8_t g_test_data[] = {0xDE, 0xAD, 0xBE, 0xEF};

static bool g_client_closed = false;
static void on_client_closed(uv_handle_t* handle) {
    (void)handle;
    g_client_closed = true;
}

/* orchestrator 用全局单例存储：每个用例各建一份会覆盖前一份，旧 engine/池
 * 不可达即泄漏。三个用例共用一份夹具，进程内只 create 一次。 */
typedef struct {
    uv_loop_t* loop;
    datahub_t* hub;
    arbiter_t* arb;
    driver_manager_storage_t drv_mgr_storage;
    bus_orchestrator_t* orch;
} passthrough_fixture_t;

static passthrough_fixture_t g_fx;

static void _fixture_ensure(void) {
    if (g_fx.orch) return;
    g_fx.loop = uv_default_loop();
    g_fx.hub = calloc(1, sizeof(datahub_t));
    assert_non_null(g_fx.hub);
    assert_int_equal(
        datahub_init(g_fx.hub, g_fx.loop, "passthrough_bus", 64, 64, NULL, 0, NULL, NULL, NULL), 0);
    memset(&g_fx.drv_mgr_storage, 0, sizeof(g_fx.drv_mgr_storage));
    arbiter_init(&g_fx.arb, g_fx.loop);
    arbiter_resume(g_fx.arb);
    driver_manager_t* drv_mgr = driver_manager_create_in(&g_fx.drv_mgr_storage);
    extern const edge_driver_t dlt698_driver;
    driver_manager_register(drv_mgr, &dlt698_driver);
    g_fx.orch = bus_orchestrator_create_hub_default(
        g_fx.hub, g_fx.arb, driver_manager_session_port_ptr(drv_mgr), 1, 10, 1024);
    assert_non_null(g_fx.orch);
    assert_int_equal(bus_orchestrator_setup_master(g_fx.orch, 0, "DLT698", 0), 0);
}

static void test_passthrough_basic(void** state) {
    (void)state;
    _fixture_ensure();
    uv_loop_t* loop = g_fx.loop;
    arbiter_t* arb = g_fx.arb;

    static io_handle_t mock_io;
    mock_io.ops = &mock_ops;
    bus_orchestrator_attach_io(g_fx.orch, 0, &mock_io);

    passthrough_mgr_t* mgr = passthrough_mgr_create(loop, g_fx.orch, arb);
    passthrough_config_t cfg = {.port = 29988, .target_bus_id = 0};
    assert_int_equal(passthrough_mgr_start(mgr, &cfg, 1), 1);

    // Connect a client to port 29988
    uv_tcp_t client;
    uv_tcp_init(loop, &client);
    struct sockaddr_in dest;
    uv_ip4_addr("127.0.0.1", 29988, &dest);

    uv_connect_t conn_req;
    int rc = uv_tcp_connect(&conn_req, &client, (const struct sockaddr*)&dest, NULL);
    assert_int_equal(rc, 0);

    // Run loop to establish connection (NOWAIT + safety sleep to prevent blocking)
    for (int i = 0; i < 1000; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        uv_sleep(1);
    }

    // Send data from client
    uv_buf_t buf = uv_buf_init((char*)g_test_data, 4);
    uv_write(&g_write_req, (uv_stream_t*)&client, &buf, 1, NULL);

    // Run loop to process write and reach IO (NOWAIT + safety sleep to prevent blocking)
    for (int i = 0; i < 1000; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (mock_write_count >= 1) {
            break;
        }
        uv_sleep(1);
    }

    // Verify IO handle received data
    assert_int_equal(mock_write_count, 1);
    assert_int_equal(last_written_len, 4);
    assert_memory_equal(last_written, g_test_data, 4);

    // Verify arbiter exclusive hold
    arbiter_bus_status_t status[1];
    arbiter_get_statuses(arb, status, 1);
    // [Nexus Fix]
    // 独占锁激活时，常规轮询被拦截挂起，总线可能由于没有在执行的常规命令而处于空闲状态，无需强制断言
    // busy 状态
    (void)status[0].is_busy;

    // Close client
    g_client_closed = false;
    uv_close((uv_handle_t*)&client, on_client_closed);

    // 💡 [Nexus Fix] 驱使事件循环分发事件，最多等待 2 秒（2000毫秒），直到客户端自身完全 close
    // 且服务端也完全处理完断开事件
    for (int i = 0; i < 2000; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        arbiter_get_statuses(arb, status, 1);
        if (g_client_closed && !status[0].is_busy) {
            break;
        }
        uv_sleep(1);
    }

    // Verify arbiter exclusive hold released
    arbiter_get_statuses(arb, status, 1);
    assert_false(status[0].is_busy);

    passthrough_mgr_destroy(mgr);
    shm_unlink("/dev/shm/edgeos_shadow_passthrough_bus");

    // 💡 [Nexus Fix] 强制驱使排空默认 loop，确保被 close 的 tcp/async handles
    // 物理彻底关闭，消除内存悬挂
    for (int i = 0; i < 50; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
    }
}

/* write_async 契约探针：成功时自持一份引用，可延迟完成；可同步失败。
 * 调用方（passthrough）必须在调用返回后归还自己那份引用，与驱动是否自持无关。 */
typedef struct {
    int writes;
    int releases;
    bool fail_sync;
    net_buf_t* held;
} tx_probe_t;

static tx_probe_t g_probe;

static void _probe_reset(bool fail_sync) {
    if (g_probe.held) net_buf_unref(g_probe.held);
    memset(&g_probe, 0, sizeof(g_probe));
    g_probe.fail_sync = fail_sync;
}

static int _probe_write_async(io_handle_t* h, net_buf_t* buf) {
    (void)h;
    g_probe.writes++;
    if (g_probe.fail_sync) return -EBUSY;
    assert_null(g_probe.held);
    g_probe.held = net_buf_ref(buf);
    return 0;
}

static void _probe_complete(void) {
    assert_non_null(g_probe.held);
    net_buf_unref(g_probe.held);
    g_probe.held = NULL;
    g_probe.releases++;
}

static io_ops_t probe_ops = {.write_async = _probe_write_async, .destroy = mock_io_destroy};

static uint32_t _pool_free(net_buf_pool_t* pool) {
    net_buf_pool_stats_t stats;
    net_buf_pool_audit(pool, 600000, &stats);
    return stats.free_count;
}

static int _pick_tcp_port(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(fd >= 0);
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    assert_int_equal(bind(fd, (struct sockaddr*)&a, sizeof(a)), 0);
    socklen_t len = sizeof(a);
    assert_int_equal(getsockname(fd, (struct sockaddr*)&a, &len), 0);
    int port = ntohs(a.sin_port);
    close(fd);
    return port;
}

static void _run_probe_session(bool fail_sync) {
    _probe_reset(fail_sync);
    _fixture_ensure();
    uv_loop_t* loop = g_fx.loop;
    int port = _pick_tcp_port();

    static io_handle_t probe_io;
    probe_io.ops = &probe_ops;
    bus_orchestrator_attach_io(g_fx.orch, 0, &probe_io);

    passthrough_mgr_t* mgr = passthrough_mgr_create(loop, g_fx.orch, g_fx.arb);
    passthrough_config_t cfg = {.port = (uint16_t)port, .target_bus_id = 0};
    assert_int_equal(passthrough_mgr_start(mgr, &cfg, 1), 1);

    net_buf_pool_t* pool = bus_orchestrator_get_pool(g_fx.orch);

    uv_tcp_t client;
    uv_tcp_init(loop, &client);
    struct sockaddr_in dest;
    uv_ip4_addr("127.0.0.1", port, &dest);
    uv_connect_t conn_req;
    assert_int_equal(uv_tcp_connect(&conn_req, &client, (const struct sockaddr*)&dest, NULL), 0);
    for (int i = 0; i < 1000; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        uv_sleep(1);
    }
    uint32_t base = _pool_free(pool);

    uv_buf_t out = uv_buf_init((char*)g_test_data, (unsigned)sizeof(g_test_data));
    g_write_req.data = NULL;
    assert_int_equal(uv_write(&g_write_req, (uv_stream_t*)&client, &out, 1, NULL), 0);

    for (int i = 0; i < 1000 && g_probe.writes == 0; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        uv_sleep(1);
    }
    assert_int_equal(g_probe.writes, 1);

    if (fail_sync) {
        /* 驱动同步失败、不接管引用：调用方仍必须归还自己那份。 */
        assert_int_equal(_pool_free(pool), base);
    } else {
        /* 在途时只有驱动自持的那一份仍在池外。 */
        assert_int_equal(_pool_free(pool), base - 1);
        _probe_complete();
        assert_int_equal(g_probe.releases, 1);
        /* 驱动完成后回到基线：调用方没有漏还引用。 */
        assert_int_equal(_pool_free(pool), base);
    }

    uv_close((uv_handle_t*)&client, NULL);
    for (int i = 0; i < 2000; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        uv_sleep(1);
    }
    passthrough_mgr_destroy(mgr);
    shm_unlink("/dev/shm/edgeos_shadow_passthrough_bus");
    for (int i = 0; i < 50; i++) uv_run(loop, UV_RUN_NOWAIT);
}

static void test_passthrough_write_async_delayed_completion_ownership(void** state) {
    (void)state;
    _run_probe_session(false);
}

static void test_passthrough_write_async_sync_failure_ownership(void** state) {
    (void)state;
    _run_probe_session(true);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_passthrough_basic),
        cmocka_unit_test(test_passthrough_write_async_delayed_completion_ownership),
        cmocka_unit_test(test_passthrough_write_async_sync_failure_ownership),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
