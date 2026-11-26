#define _GNU_SOURCE

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_datahub_adapter.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "datahub_priv.h"
#include "driver_manager.h"
#include "edge_api.h"
#include "iems_driver_adapter.h"
#include "iems_execution_adapter.h"
#include "iems_north_adapter.h"
#include "io_handle_priv.h"

/* B5 回归：仲裁器已接受（write 返回 0）但引擎在真正上线前失败（无 IO 的总线），
 * 请求完成回调必须恰好一次。旧实现只把 arbiter 置 IO_ERROR、不调 on_finish，
 * 执行适配器与北向结果槽永不释放：16 条后两个池全满。 */

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    arbiter_t *arb;
    bus_orchestrator_t *orch;
    driver_manager_t *drv_mgr;
    driver_manager_storage_t drv_mgr_storage;
} fixture_t;

static _Atomic int g_done;
static _Atomic int g_failed;

static void _done(void *user, iems_write_done_status_t status) {
    (void)user;
    atomic_fetch_add(&g_done, 1);
    if (status == IEMS_WRITE_DONE_FAILED) atomic_fetch_add(&g_failed, 1);
}

static void _close_handle(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int setup(void **state) {
    fixture_t *f = calloc(1, sizeof(*f));
    assert_non_null(f);
    assert_int_equal(uv_loop_init(&f->loop), 0);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "iems_noio_done_%d", (int)getpid());
    assert_int_equal(datahub_init(&f->hub, &f->loop, shm_name, 16, 32, NULL, 0, NULL, NULL, NULL),
                     0);
    assert_int_equal(arbiter_init(&f->arb, &f->loop), 0);
    arbiter_resume(f->arb);

    f->drv_mgr = driver_manager_create_in(&f->drv_mgr_storage);
    assert_non_null(f->drv_mgr);
    extern int modbus_driver_register(driver_manager_t * mgr);
    assert_int_equal(modbus_driver_register(f->drv_mgr), 0);

    f->orch = bus_orchestrator_create_hub_default(
        &f->hub, f->arb, driver_manager_session_port_ptr(f->drv_mgr), 2, 8, 256);
    assert_non_null(f->orch);
    datahub_bind_io_pool(&f->hub, bus_orchestrator_get_pool(f->orch));
    edge_api_init(f->orch);

    /* 生产形态：SOUTHBOUND 已配置、引擎已建，但 UART 打开失败 → 从未 attach_io。 */
    assert_int_equal(bus_orchestrator_setup_master(f->orch, 0, "ModbusRTU", 0), 0);

    *state = f;
    return 0;
}

static int teardown(void **state) {
    fixture_t *f = (fixture_t *)*state;
    if (!f) return 0;
    edge_api_init(NULL);
    bus_orchestrator_destroy(f->orch);
    arbiter_destroy(f->arb);
    driver_manager_destroy(f->drv_mgr);
    uv_walk(&f->loop, _close_handle, NULL);
    for (int i = 0; i < 200 && uv_loop_alive(&f->loop); i++) uv_run(&f->loop, UV_RUN_NOWAIT);
    datahub_destroy(&f->hub);
    uv_loop_close(&f->loop);
    free(f);
    return 0;
}

static void test_no_io_bus_completes_every_write(void **state) {
    fixture_t *f = (fixture_t *)*state;
    iems_execution_port_t port = iems_execution_port_create(NULL);
    assert_non_null(port.write_tag);

    atomic_store(&g_done, 0);
    atomic_store(&g_failed, 0);

    /* 超过执行/北向两个池的容量，全部必须带回调完成（旧实现一条都不回调）。 */
    const int count = IEMS_NORTH_RESULT_SLOTS + 1;
    for (int i = 0; i < count; i++) {
        assert_int_equal(port.write_tag(port.ctx, 0, 1, IEMS_TAG_BATTERY_SOC, 55.0, "ModbusRTU",
                                        _done, NULL),
                         0);
    }
    for (int i = 0; i < 100 && atomic_load(&g_done) < count; i++) {
        uv_run(&f->loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(atomic_load(&g_done), count); /* 恰好一次/条，不重复 */
    assert_int_equal(atomic_load(&g_failed), count);

    /* 池没有泄漏：第 18 条带回调的写仍被接受并完成。 */
    assert_int_equal(port.write_tag(port.ctx, 0, 1, IEMS_TAG_BATTERY_SOC, 55.0, "ModbusRTU",
                                    _done, NULL),
                     0);
    for (int i = 0; i < 100 && atomic_load(&g_done) < count + 1; i++) {
        uv_run(&f->loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(atomic_load(&g_done), count + 1);

    /* 不带回调的写行为不变。 */
    assert_int_equal(
        port.write_tag(port.ctx, 0, 1, IEMS_TAG_BATTERY_SOC, 55.0, "ModbusRTU", NULL, NULL), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_no_io_bus_completes_every_write, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
