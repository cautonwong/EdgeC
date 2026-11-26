#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "dcu_heartbeat.h"
#include "dcu_storage.h"

static void test_stop_closes_timers_and_is_idempotent(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    dcu_heartbeat_t hb;
    memset(&hb, 0, sizeof(hb));
    dcu_heartbeat_start(&hb, &loop, NULL, NULL, NULL);
    assert_true(uv_is_active((uv_handle_t *)&hb.freeze_timer));

    dcu_heartbeat_stop(&hb);
    assert_false(uv_is_active((uv_handle_t *)&hb.freeze_timer));
    assert_false(uv_is_active((uv_handle_t *)&hb.control_timer));
    dcu_heartbeat_stop(&hb);

    int cycles = hb.test_cycles;
    uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(hb.test_cycles, cycles);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

/* teardown 顺序回归：先停心跳，再销毁 storage，再跑循环，不得触发已销毁 storage 的 flush。 */
static void test_stop_before_storage_destroy(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    char path[] = "/tmp/dcu_heartbeat_stop_XXXXXX";
    int fd = mkstemp(path);
    assert_true(fd >= 0);
    close(fd);
    dcu_storage_t storage;
    assert_int_equal(dcu_storage_init(&storage, &loop, path), 0);

    dcu_heartbeat_t hb;
    memset(&hb, 0, sizeof(hb));
    dcu_heartbeat_start(&hb, &loop, NULL, NULL, NULL);
    dcu_heartbeat_stop(&hb);
    dcu_storage_destroy(&storage);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
    unlink(path);
    char side[80];
    snprintf(side, sizeof(side), "%s-wal", path);
    unlink(side);
    snprintf(side, sizeof(side), "%s-shm", path);
    unlink(side);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_stop_closes_timers_and_is_idempotent),
        cmocka_unit_test(test_stop_before_storage_destroy),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
