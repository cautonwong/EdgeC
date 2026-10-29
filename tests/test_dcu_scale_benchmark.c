/**
 * @file test_dcu_scale_benchmark.c
 * @brief Benchmark & E2E Verification for DCU Scale Polling & Request Pipeline (AEP-399 & AEP-401)
 *
 * Verifies:
 * 1. Benchmark: 4 buses x 500 meters x 3 OADs (2000 meters, 6000 OADs).
 *    Measures per-step latency and verifies P99 latency is bounded under 5ms.
 * 2. E2E Request/MasterEngine Flow:
 *    encode_request -> io_executor -> decode_response -> datahub_write_shadow
 */

#include <cmocka.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "net_buf.h"

#define NUM_BUSES 4
#define METERS_PER_BUS 500
#define TOTAL_METERS (NUM_BUSES * METERS_PER_BUS)
#define OADS_PER_METER 3

/*
 * ModbusRTU 契约注入：驱动靠 lookup_by_tag 把 tag 映射成寄存器地址与缩放系数。
 * 缺失时 encode_request 返回 -1（本用例走的是真实的单表驱动编码路径）。
 * 帧 0x0898 => 2200，scaler 0.1 => 220.0V。
 * 生产端已在 dcu_driver_adapter.c 提供强定义 edge_model_find 顶掉 IO 层弱符号，
 * 本用例用 `-Wl,--allow-multiple-definition` 让测试副本优先（见 tests/CMakeLists.txt）。
 */
static const modbus_map_t _mock_modbus_map = {
    .key = 0, .tag_id = 1001, .length = 1, .scaler = 0.1f};

static const void* _mock_lookup_by_tag(uint32_t tag_id) {
    return (tag_id == 1001) ? &_mock_modbus_map : NULL;
}

const driver_contract_injection_t* edge_model_find(const char* name) {
    static const driver_contract_injection_t inj = {.lookup_by_tag = _mock_lookup_by_tag};
    return (name && strcmp(name, "ModbusRTU") == 0) ? &inj : NULL;
}

/* 异步 mock：提交只登记回调，不立刻触发。
 *
 * 链式模型下 dcu_poller_step 的职责是"每条通道提交一项就返回"，
 * 所以这里要测的是**这一步阻塞事件循环多久**——用同步回调会把整轮都算进
 * 一步的时间里，测出来的不是事件循环延迟。 */
#define PEND_MAX 128
typedef struct {
    poller_read_done_cb cb;
    void *ud;
} pend_t;
static pend_t g_pend[PEND_MAX];
static int g_pend_n;
static uint64_t g_value_seq;

static int _mock_source_submit(void* ctx, uint32_t meter_idx, uint32_t tag_id,
                               poller_read_done_cb cb, void* ud) {
    (void)ctx;
    (void)tag_id;
    if (g_pend_n >= PEND_MAX) return -1;   /* 提交失败：链会当场结算并继续 */
    g_pend[g_pend_n].cb = cb;
    g_pend[g_pend_n].ud = ud;
    g_pend_n++;
    return 0;
}

/* 把所有在途回调放完：回调里会继续提交，直到链自己停下来 */
static void _drain(void) {
    int guard = 0;
    while (g_pend_n > 0 && guard++ < 100000) {
        pend_t p = g_pend[--g_pend_n];
        if (p.cb) p.cb(p.ud, 0, 220.0 + (double)(g_value_seq++ % 10));
    }
}

static void test_scale_benchmark_p99_latency(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);

    // 1. 注册 4 条总线 x 500 台电表 = 2000 台电表
    for (int b = 0; b < NUM_BUSES; b++) {
        edge_poll_policy_t policy = {
            .interval_ms = 0,
            .timeout_ms = 500,
            .max_in_flight = 1,
            .circuit_break_threshold = 5
        };
        dcu_poller_set_channel_policy(&poller, (uint32_t)b, &policy);

        for (int m = 0; m < METERS_PER_BUS; m++) {
            char addr[32];
            snprintf(addr, sizeof(addr), "M_%d_%04d", b, m);
            dcu_inventory_add(&inv, addr, "dlt698", 9600, (uint8_t)b, NULL);
        }
    }
    assert_int_equal(inv.count, TOTAL_METERS);

    // 2. 配置 3 个 OAD 的周期方案
    uint32_t csds[OADS_PER_METER] = {0x20000200, 0x20010200, 0x20020200}; // A/B/C 相电压
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, csds, OADS_PER_METER), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 0, 1), 0);

    poller_source_ops_t source;
    memset(&source, 0, sizeof(source));
    source.submit = _mock_source_submit;
    poller_bus_ops_t bus;
    poller_sink_ops_t sink;
    memset(&bus, 0, sizeof(bus));
    memset(&sink, 0, sizeof(sink));
    assert_int_equal(dcu_poller_start(&poller, source, bus, sink), 0);

    /* 3. 一步 = 每条通道提交一项就返回；随后 drain 把回调放完，让链推进到下一块表。
     *    这里量的是 step 本身的耗时，也就是它阻塞事件循环的时长。 */
    const int total_steps = 400;   /* 4 通道 × 500 表，一步推进一格，400 步覆盖多轮 */
    uint64_t latencies_ns[400];
    g_pend_n = 0;
    g_value_seq = 0;

    for (int step = 0; step < total_steps; step++) {
        uint64_t t0 = uv_hrtime();
        (void)dcu_poller_step(&poller, &inv, 0);
        uint64_t t1 = uv_hrtime();
        latencies_ns[step] = t1 - t0;
        _drain();
    }

    // 4. 统计 P99 与最大延迟
    for (int i = 0; i < total_steps - 1; i++) {
        for (int j = 0; j < total_steps - 1 - i; j++) {
            if (latencies_ns[j] > latencies_ns[j + 1]) {
                uint64_t tmp = latencies_ns[j];
                latencies_ns[j] = latencies_ns[j + 1];
                latencies_ns[j + 1] = tmp;
            }
        }
    }

    int p99_idx = (int)(total_steps * 0.99);
    if (p99_idx >= total_steps) p99_idx = total_steps - 1;

    double p99_ms = (double)latencies_ns[p99_idx] / 1000000.0;
    double max_ms = (double)latencies_ns[total_steps - 1] / 1000000.0;
    double avg_ms = 0.0;
    for (int i = 0; i < total_steps; i++) avg_ms += (double)latencies_ns[i] / 1000000.0;
    avg_ms /= (double)total_steps;

    printf("\n📊 [BENCHMARK] 4 Buses x 500 Meters (2000 Meters, 6000 OADs):\n");
    printf("   - Avg Step Latency: %.3f ms\n", avg_ms);
    printf("   - P99 Step Latency: %.3f ms (Threshold: < 5.0 ms)\n", p99_ms);
    printf("   - Max Step Latency: %.3f ms\n", max_ms);
    printf("   - Submits issued: %" PRIu64 "\n", g_value_seq);

    // 一步只提交"每通道一项"，不得阻塞事件循环
    assert_true(p99_ms < 5.0);
    assert_true(g_value_seq > 0);

    dcu_poller_destroy(&poller);
}

static void test_e2e_request_to_shadow(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);

    datahub_t hub;
    assert_int_equal(datahub_init(&hub, &loop, "bench_hub", 256, 1024, NULL, 0, NULL, NULL, NULL), 0);

    driver_manager_t* drv_mgr = driver_manager_create();
    assert_non_null(drv_mgr);

    // 注册驱动
    extern const edge_driver_t modbus_rtu_driver;
    driver_manager_register(drv_mgr, &modbus_rtu_driver);

    edge_orchestrator_cfg_t cfg = {
        .loop = &loop,
        .hub = &hub,
        .io_pool = datahub_get_io_pool(&hub),
        .max_channels = 4
    };
    assert_int_equal(driver_manager_start_auto_polling(drv_mgr, &cfg), 0);

    // 注册轮询目标 (Device 1, ModbusRTU)
    uint32_t tags[1] = {1001};
    edge_poll_policy_t policy = {.interval_ms = 1000, .timeout_ms = 500, .max_in_flight = 1};
    assert_int_equal(driver_manager_add_poll_target(drv_mgr, 1, "ModbusRTU", NULL, tags, 1, &policy), 0);

#include "modbus_codec.h"

    // 构造真实合规 Modbus RTU 响应帧：Slave=1, FC=03, ByteCount=2, Value=0x0898 (220.0V), CRC
    net_buf_pool_t* pool = net_buf_pool_new(16, 256, 0);
    assert_non_null(pool);
    net_buf_t* rx_buf = net_buf_alloc(pool, 0);
    assert_non_null(rx_buf);
    uint8_t raw_modbus_resp[7] = {0x01, 0x03, 0x02, 0x08, 0x98, 0x00, 0x00};
    uint16_t crc = modbus_calculate_crc(raw_modbus_resp, 5);
    raw_modbus_resp[5] = (uint8_t)(crc & 0xFF);
    raw_modbus_resp[6] = (uint8_t)(crc >> 8);
    // 触发主动编码请求 (encode_request)
    assert_int_equal(driver_manager_trigger_poll_now(drv_mgr, 1), 0);

    net_buf_simple_add_mem(&rx_buf->b, raw_modbus_resp, sizeof(raw_modbus_resp));

    // 喂入响应帧并触发解码与 DataHub 影子更新
    int rc = driver_manager_feed_response(drv_mgr, 1, rx_buf);
    assert_int_equal(rc, 0);

    // 检查统计指标更新
    edge_poll_stats_t stats;
    assert_int_equal(driver_manager_get_target_stats(drv_mgr, 1, &stats), 0);
    assert_int_equal(stats.success_count, 1);
    assert_int_equal(stats.consecutive_fails, 0);
    assert_false(stats.is_circuit_broken);

    // 验证 DataHub Shadow 状态已同步
    pipe_msg_t shadow_msg;
    assert_int_equal(datahub_read_shadow(&hub, 1001, &shadow_msg), 0);
    assert_float_equal(shadow_msg.payload.val.f, 220.0, 0.5);

    net_buf_unref(rx_buf);
    net_buf_pool_destroy(pool);
    driver_manager_destroy(drv_mgr);
    datahub_destroy(&hub);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_scale_benchmark_p99_latency),
        cmocka_unit_test(test_e2e_request_to_shadow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
