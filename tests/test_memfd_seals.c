#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"

// 补齐 Seal 定义 (如果系统头文件太老)
#ifndef F_GET_SEALS
#define F_GET_SEALS (1034)
#endif

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static void cleanup_loop(uv_loop_t *loop) {
    uv_walk(loop, on_walk_close, NULL);
    while (uv_run(loop, UV_RUN_DEFAULT) != 0)
        ;
    uv_loop_close(loop);
}

static void test_memfd_sealing_logic(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;

    // 1. 初始化 DataHub (内部会创建带密封的 memfd)
    int ret = datahub_init(&hub, &loop, "test_seal", 64, 100, NULL, 0, NULL, NULL);
    assert_int_equal(ret, 0);
    assert_true(hub.shadow_fd >= 0);

    // 2. 验证 Seal 是否生效 (F_GET_SEALS)
    int seals = fcntl(hub.shadow_fd, F_GET_SEALS);
    assert_true(seals != -1);
    // 应该包含 SHRINK, GROW, SEAL
    assert_true(seals & 0x0002);  // F_SEAL_SHRINK
    assert_true(seals & 0x0004);  // F_SEAL_GROW

    // 3. 尝试非法操作：改变大小
    ret = ftruncate(hub.shadow_fd, (off_t)(hub.shadow_size + 4096));
    assert_int_equal(ret, -1);
    assert_int_equal(errno, EPERM);

    datahub_destroy(&hub);
    cleanup_loop(&loop);
}

static void test_shadow_table_sharing(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub_master;
    datahub_t hub_slave;

    // 1. Master 初始化
    datahub_init(&hub_master, &loop, "master", 64, 100, NULL, 0, NULL, NULL);

    // 2. Slave 从 FD 初始化
    int ring_fd = hub_master.ring.fd;
    size_t ring_size = hub_master.ring.map_size;
    int shadow_fd = hub_master.shadow_fd;
    size_t shadow_size = hub_master.shadow_size;

    int ret = datahub_init_from_fd(&hub_slave, &loop, ring_fd, ring_size, shadow_fd, shadow_size,
                                   100, NULL, 0, NULL, NULL);
    assert_int_equal(ret, 0);

    // 3. 在 Master 发布数据
    pipe_msg_t msg = {0};
    msg.tag_id = 10;
    msg.payload.val.f = 3.14159f;
    msg.ts = 12345678;

    datahub_publish(&hub_master, &msg);

    // Slave 读取
    pipe_msg_t out = {0};
    ret = datahub_read_shadow(&hub_slave, 10, &out);
    assert_int_equal(ret, 0);
    assert_float_equal(out.payload.val.f, 3.14159f, 0.0001f);

    datahub_destroy(&hub_master);
    datahub_destroy(&hub_slave);
    cleanup_loop(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_memfd_sealing_logic),
        cmocka_unit_test(test_shadow_table_sharing),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
