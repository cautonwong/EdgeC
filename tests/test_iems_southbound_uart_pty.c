#define _GNU_SOURCE

#include <cmocka.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "bus_datahub_adapter.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "datahub_priv.h"
#include "iems_southbound_adapter.h"
#include "io_handle_priv.h"

/* 端到端约束：南向 UART 的 attach 路径此前只有纯函数与 IO 层单测间接覆盖。
 * PTY 让 iems_southbound_connect 走真实 open/tcsetattr/uv_poll，再把 handle 挂到
 * orchestrator；这条链任何一环断开，本用例都会红。 */

static void _close_handle(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static void test_uart_bus_attaches_pty(void **state) {
    (void)state;

    int master = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert_true(master >= 0);
    assert_int_equal(grantpt(master), 0);
    assert_int_equal(unlockpt(master), 0);
    const char *slave_name = ptsname(master);
    assert_non_null(slave_name);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    datahub_t hub;
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "iems_sb_uart_pty_%d", (int)getpid());
    assert_int_equal(datahub_init(&hub, &loop, shm_name, 16, 32, NULL, 0, NULL, NULL, NULL), 0);

    bus_orchestrator_t *orch = bus_orchestrator_create_hub_default(&hub, NULL, NULL, 2, 8, 256);
    assert_non_null(orch);
    /* 与 iems_app 的装配顺序一致：IO 池由 orchestrator 建，再交给 DataHub */
    datahub_bind_io_pool(&hub, bus_orchestrator_get_pool(orch));

    assert_int_equal(setenv("OMNIS_BUS_0_DEV", slave_name, 1), 0);
    assert_int_equal(setenv("OMNIS_BUS_0_BAUD", "115200", 1), 0);

    bus_conn_state_t bus_state[IEMS_MAX_VBUS] = {BUS_STATE_DISCONNECTED};
    int configured = 0;
    iems_southbound_connect(&loop, &hub, orch, bus_state, &configured);

    assert_true(configured >= 1);
    assert_int_equal(bus_state[0], BUS_STATE_CONNECTED);
    io_handle_t *h = bus_orchestrator_get_io(orch, 0);
    assert_non_null(h);

    /* 没有驱动会话，帧到 IO 层后丢弃；这里只要求数据路径不把 handle 弄坏 */
    const uint8_t frame[] = {0x01, 0x03, 0x00, 0x00};
    assert_int_equal(write(master, frame, sizeof(frame)), (ssize_t)sizeof(frame));
    for (int i = 0; i < 100; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_ptr_equal(bus_orchestrator_get_io(orch, 0), h);

    unsetenv("OMNIS_BUS_0_DEV");
    unsetenv("OMNIS_BUS_0_BAUD");
    close(master);

    /* orchestrator 不拥有 IO 生命周期，先自己关，再销毁编排器 */
    for (int i = 0; i < IEMS_MAX_VBUS; i++) {
        io_handle_t *attached = bus_orchestrator_get_io(orch, i);
        if (attached) attached->ops->destroy(attached);
    }
    for (int i = 0; i < 200 && uv_loop_alive(&loop); i++) uv_run(&loop, UV_RUN_NOWAIT);

    net_buf_pool_t *pool = bus_orchestrator_get_pool(orch);
    bus_orchestrator_destroy(orch);
    net_buf_pool_destroy(pool);
    datahub_destroy(&hub);

    /* audit_timer 在 create 时初始化而 destroy 不关；剩余句柄统一关闭后
     * uv_loop_close 才可能归零 */
    uv_walk(&loop, _close_handle, NULL);
    for (int i = 0; i < 200 && uv_loop_alive(&loop); i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

/* 非法 OMNIS_BUS_n_BAUD 必须拒绝整条总线: 9600abc 旧实现会被 strtoul 截成
 * 9600, 数值合法但未支持的速率则会被驱动静默退回 9600, 两者都改错 wire speed。 */
static void test_uart_bus_rejects_invalid_baud(void **state) {
    (void)state;

    int master = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert_true(master >= 0);
    assert_int_equal(grantpt(master), 0);
    assert_int_equal(unlockpt(master), 0);
    const char *slave_name = ptsname(master);
    assert_non_null(slave_name);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    datahub_t hub;
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "iems_sb_uart_baud_%d", (int)getpid());
    assert_int_equal(datahub_init(&hub, &loop, shm_name, 16, 32, NULL, 0, NULL, NULL, NULL), 0);

    bus_orchestrator_t *orch = bus_orchestrator_create_hub_default(&hub, NULL, NULL, 2, 8, 256);
    assert_non_null(orch);
    datahub_bind_io_pool(&hub, bus_orchestrator_get_pool(orch));

    assert_int_equal(setenv("OMNIS_BUS_0_DEV", slave_name, 1), 0);

    bus_conn_state_t bus_state[IEMS_MAX_VBUS] = {BUS_STATE_DISCONNECTED};
    int configured = 0;

    assert_int_equal(setenv("OMNIS_BUS_0_BAUD", "9600abc", 1), 0);
    iems_southbound_connect(&loop, &hub, orch, bus_state, &configured);
    assert_true(configured >= 1);
    assert_int_equal(bus_state[0], BUS_STATE_DISCONNECTED);
    assert_null(bus_orchestrator_get_io(orch, 0));

    assert_int_equal(setenv("OMNIS_BUS_0_BAUD", "1234", 1), 0);
    iems_southbound_connect(&loop, &hub, orch, bus_state, &configured);
    assert_int_equal(bus_state[0], BUS_STATE_DISCONNECTED);
    assert_null(bus_orchestrator_get_io(orch, 0));

    unsetenv("OMNIS_BUS_0_DEV");
    unsetenv("OMNIS_BUS_0_BAUD");
    close(master);

    net_buf_pool_t *pool = bus_orchestrator_get_pool(orch);
    bus_orchestrator_destroy(orch);
    net_buf_pool_destroy(pool);
    datahub_destroy(&hub);

    uv_walk(&loop, _close_handle, NULL);
    for (int i = 0; i < 200 && uv_loop_alive(&loop); i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_uart_bus_attaches_pty),
        cmocka_unit_test(test_uart_bus_rejects_invalid_baud),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
