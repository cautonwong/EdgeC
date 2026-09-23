#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>

#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "net_buf.h"

// --- Mock Driver Implementation ---

static int mock_driver_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids,
                                      int tag_count, net_buf_t *out) {
    (void)ctx;
    if (!out) return -1;
    net_buf_reset(out);
    // 写入请求头与测点列表
    net_buf_simple_add_u8(&out->b, 0xAA);  // 帧头
    net_buf_simple_add_u8(&out->b, (uint8_t)tag_count);
    for (int i = 0; i < tag_count; i++) {
        net_buf_simple_add_be32(&out->b, tag_ids[i]);
    }
    return 0;
}

static int mock_driver_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                       pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)ctx;
    (void)resp_out;
    if (!in || !out_msgs || !msg_count || !consumed) return -1;
    if (in->b.len < 10) return -2;  // 最小帧长校验

    uint8_t header = net_buf_simple_pull_u8(&in->b);
    if (header != 0x55) return -3;  // 响应帧头

    uint8_t count = net_buf_simple_pull_u8(&in->b);
    *msg_count = 0;

    for (int i = 0; i < count && in->b.len >= 8; i++) {
        uint32_t tag_id = net_buf_simple_pull_be32(&in->b);
        uint32_t raw_val = net_buf_simple_pull_be32(&in->b);

        out_msgs[*msg_count].tag_id = tag_id;
        out_msgs[*msg_count].type = PIPEMSG_TYPE_SCALAR;
        out_msgs[*msg_count].payload.val.f = (float)raw_val / 10.0f;
        out_msgs[*msg_count].ts = 12345678ULL;
        (*msg_count)++;
    }

    *consumed = 2 + (size_t)(*msg_count) * 8;
    return 0;
}

static const edge_driver_t g_mock_sensor_driver = {
    .name = "MockSensor",
    .ctx_size = 32,
    .on_init = NULL,
    .on_cleanup = NULL,
    .is_ready = NULL,
    .on_tick = NULL,
    .encode_request = mock_driver_encode_request,
    .encode_write = NULL,
    .decode_response = mock_driver_decode_response,
    .handle_rx = NULL,
};

// --- Mock IO Handle ---

typedef struct {
    io_handle_t base;
    int tx_count;
    net_buf_t *last_tx_buf;
    bool read_enabled;
} mock_io_handle_t;

static int mock_io_enable_read(io_handle_t *h, bool enable) {
    mock_io_handle_t *mio = (mock_io_handle_t *)h;
    mio->read_enabled = enable;
    return 0;
}

static int mock_io_write_async(io_handle_t *h, net_buf_t *buf) {
    mock_io_handle_t *mio = (mock_io_handle_t *)h;
    mio->tx_count++;
    if (mio->last_tx_buf) {
        net_buf_unref(mio->last_tx_buf);
    }
    mio->last_tx_buf = net_buf_ref(buf);
    net_buf_unref(buf);  // 释放管理器传过来的引用
    return 0;
}

static const io_ops_t g_mock_io_ops = {
    .enable_read = mock_io_enable_read,
    .write_async = mock_io_write_async,
    .ioctl = NULL,
    .destroy = NULL,
};

// --- Test Cases ---

static void test_orchestrator_init_and_registration(void **state) {
    (void)state;
    driver_manager_t *mgr = driver_manager_create();
    assert_non_null(mgr);

    int rc = driver_manager_register(mgr, &g_mock_sensor_driver);
    assert_int_equal(rc, 0);

    const edge_driver_t *found = driver_manager_find(mgr, "MockSensor");
    assert_ptr_equal(found, &g_mock_sensor_driver);

    driver_manager_destroy(mgr);
}

static void test_orchestrator_manual_trigger_and_feed(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "orch_shm_test", 64, 0x10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    net_buf_pool_t *pool = net_buf_pool_new(16, 512, 0);
    assert_non_null(pool);

    driver_manager_t *mgr = driver_manager_create();
    assert_non_null(mgr);
    driver_manager_register(mgr, &g_mock_sensor_driver);

    edge_orchestrator_cfg_t cfg = {
        .loop = NULL,
        .hub = &hub,
        .io_pool = pool,
        .max_channels = 16,
    };
    rc = driver_manager_start_auto_polling(mgr, &cfg);
    assert_int_equal(rc, 0);

    mock_io_handle_t mio = {0};
    mio.base.ops = &g_mock_io_ops;

    uint32_t tag_list[] = {0x2001, 0x2002};
    edge_poll_policy_t policy = {
        .interval_ms = 1000,
        .timeout_ms = 500,
        .max_retries = 3,
        .mtt_wall_clock_align = false,
        .priority = 0,
    };

    rc = driver_manager_add_poll_target(mgr, 101, "MockSensor", (io_handle_t *)&mio, tag_list, 2,
                                        &policy);
    assert_int_equal(rc, 0);
    assert_true(mio.read_enabled);

    // 1. 手动触发一次轮询
    rc = driver_manager_trigger_poll_now(mgr, 101);
    assert_int_equal(rc, 0);
    assert_int_equal(mio.tx_count, 1);
    assert_non_null(mio.last_tx_buf);
    assert_int_equal(mio.last_tx_buf->b.data[0], 0xAA);  // 校验编码帧头
    assert_int_equal(mio.last_tx_buf->b.data[1], 2);     // 2 个测点

    edge_poll_stats_t stats = {0};
    rc = driver_manager_get_target_stats(mgr, 101, &stats);
    assert_int_equal(rc, 0);
    assert_int_equal(stats.poll_count, 1);
    assert_int_equal(stats.success_count, 0);

    // 2. 构造响应帧并喂入
    net_buf_t *rx_buf = net_buf_alloc(pool, 0);
    assert_non_null(rx_buf);
    net_buf_simple_add_u8(&rx_buf->b, 0x55);  // 响应帧头
    net_buf_simple_add_u8(&rx_buf->b, 2);     // 2 个测点响应
    net_buf_simple_add_be32(&rx_buf->b, 0x2001);
    net_buf_simple_add_be32(&rx_buf->b, 2205);  // 220.5V
    net_buf_simple_add_be32(&rx_buf->b, 0x2002);
    net_buf_simple_add_be32(&rx_buf->b, 500);  // 50.0Hz

    rc = driver_manager_feed_response(mgr, 101, rx_buf);
    assert_int_equal(rc, 0);
    net_buf_unref(rx_buf);

    // 3. 验证入库 DataHub 影子表
    pipe_msg_t read_msg1 = {0};
    pipe_msg_t read_msg2 = {0};
    rc = datahub_read_shadow(&hub, 0x2001, &read_msg1);
    assert_int_equal(rc, 0);
    assert_float_equal(read_msg1.payload.val.f, 220.5f, 0.01f);

    rc = datahub_read_shadow(&hub, 0x2002, &read_msg2);
    assert_int_equal(rc, 0);
    assert_float_equal(read_msg2.payload.val.f, 50.0f, 0.01f);

    // 4. 验证统计指标更新
    rc = driver_manager_get_target_stats(mgr, 101, &stats);
    assert_int_equal(rc, 0);
    assert_int_equal(stats.poll_count, 1);
    assert_int_equal(stats.success_count, 1);
    assert_int_equal(stats.consecutive_fails, 0);

    if (mio.last_tx_buf) {
        net_buf_unref(mio.last_tx_buf);
        mio.last_tx_buf = NULL;
    }

    driver_manager_destroy(mgr);
    net_buf_pool_destroy(pool);
    datahub_destroy(&hub);
}

static void test_orchestrator_periodic_timer_and_timeout(void **state) {
    (void)state;
    uv_loop_t *loop = uv_loop_new();
    assert_non_null(loop);

    datahub_t hub;
    int rc = datahub_init(&hub, loop, "orch_timer_test", 64, 0x10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    net_buf_pool_t *pool = net_buf_pool_new(16, 512, 0);
    assert_non_null(pool);

    driver_manager_t *mgr = driver_manager_create();
    assert_non_null(mgr);
    driver_manager_register(mgr, &g_mock_sensor_driver);

    edge_orchestrator_cfg_t cfg = {
        .loop = loop,
        .hub = &hub,
        .io_pool = pool,
        .max_channels = 16,
    };
    rc = driver_manager_start_auto_polling(mgr, &cfg);
    assert_int_equal(rc, 0);

    mock_io_handle_t mio = {0};
    mio.base.ops = &g_mock_io_ops;

    uint32_t tag_list[] = {0x2001};
    edge_poll_policy_t policy = {
        .interval_ms = 20,
        .timeout_ms = 10,
        .max_retries = 3,
        .mtt_wall_clock_align = true,
        .priority = 1,
    };

    rc = driver_manager_add_poll_target(mgr, 202, "MockSensor", (io_handle_t *)&mio, tag_list, 1,
                                        &policy);
    assert_int_equal(rc, 0);

    // 运行事件循环若干次让定时器触发
    for (int i = 0; i < 5; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        struct timespec req = {0, 15000000L};  // 15ms
        nanosleep(&req, NULL);
        uv_run(loop, UV_RUN_NOWAIT);
    }

    edge_poll_stats_t stats = {0};
    rc = driver_manager_get_target_stats(mgr, 202, &stats);
    assert_int_equal(rc, 0);
    assert_true(stats.poll_count >= 1);
    assert_true(stats.timeout_count >= 1);  // 未应答故触发超时

    // 测试暂停与恢复
    rc = driver_manager_set_target_active(mgr, 202, false);
    assert_int_equal(rc, 0);
    uint64_t polls_before = stats.poll_count;

    for (int i = 0; i < 3; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        struct timespec req = {0, 15000000L};
        nanosleep(&req, NULL);
        uv_run(loop, UV_RUN_NOWAIT);
    }

    rc = driver_manager_get_target_stats(mgr, 202, &stats);
    assert_int_equal(rc, 0);
    assert_int_equal(stats.poll_count, polls_before);  // 暂停期间无新轮询

    rc = driver_manager_set_target_active(mgr, 202, true);
    assert_int_equal(rc, 0);

    if (mio.last_tx_buf) {
        net_buf_unref(mio.last_tx_buf);
        mio.last_tx_buf = NULL;
    }

    driver_manager_stop_auto_polling(mgr);
    driver_manager_destroy(mgr);
    net_buf_pool_destroy(pool);
    datahub_destroy(&hub);

    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    free(loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_orchestrator_init_and_registration),
        cmocka_unit_test(test_orchestrator_manual_trigger_and_feed),
        cmocka_unit_test(test_orchestrator_periodic_timer_and_timeout),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
