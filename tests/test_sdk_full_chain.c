#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "edge_log.h"
#include "edge_service.h"
#include "net_buf.h"
#include "rest_server.h"
#include "sqlite_service.h"
#include "stream_engine.h"

/* ---------------- context ---------------- */

typedef struct {
    uv_loop_t loop;  // 使用独立 Loop
    datahub_t hub;
    sqlite_svc_t *db;
    stream_engine_t *engine;
    service_t *svc;
} full_chain_ctx_t;

/* ---------------- setup / teardown ---------------- */

static int setup(void **state) {
    full_chain_ctx_t *ctx = calloc(1, sizeof(full_chain_ctx_t));
    uv_loop_init(&ctx->loop);

    edge_log_init(NULL, false);

    // 初始化地基
    datahub_init(&ctx->hub, &ctx->loop, "full_chain_v5", 256);

    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = ":memory:"};
    ctx->db = sqlite_svc_create(&ctx->loop, &db_cfg);

    // 初始化控制平面
    service_options_t opts = {.mode = "TDD_FULL_CHAIN"};
    ctx->svc = service_new(&opts);
    service_set_datahub(ctx->svc, &ctx->hub);
    service_set_storage(ctx->svc, ctx->db);

    // 初始化计算平面
    ctx->engine = stream_engine_create(&ctx->hub, NULL);
    service_set_stream_engine(ctx->svc, ctx->engine);

    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    full_chain_ctx_t *ctx = *state;

    stream_engine_destroy(ctx->engine);
    service_delete(ctx->svc);
    sqlite_svc_destroy(ctx->db);
    datahub_destroy(&ctx->hub);

    // 彻底排干 loop 上的残留回调 (如 uv_close)
    uv_run(&ctx->loop, UV_RUN_DEFAULT);
    uv_loop_close(&ctx->loop);

    edge_log_fini();
    free(ctx);
    return 0;
}

/* ---------------- Scenario Helpers ---------------- */

static int execution_order[2];
static int exec_index = 0;

static void on_cmd_start(void *user_data) {
    int id = (int)(intptr_t)user_data;
    if (exec_index < 2) {
        execution_order[exec_index++] = id;
    }
}

/* ---------------- Scenario Tests ---------------- */

/**
 * [S1, S2, S13] 影子服务测试
 */
static void test_scenario_shadow_service(void **state) {
    full_chain_ctx_t *ctx = *state;

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 4001, 220.5, uv_now(&ctx->loop));
    datahub_publish(&ctx->hub, &msg);

    uv_run(&ctx->loop, UV_RUN_NOWAIT);

    pipe_msg_t shadow;
    int ret = datahub_read_shadow(&ctx->hub, 4001, &shadow);
    assert_int_equal(ret, 0);
    assert_double_equal(shadow.payload.val.f, 220.5, 0.001);
}

/**
 * [S5, S6, S18] 指令优先级仲裁测试
 */
static void test_scenario_priority_arbitration(void **state) {
    full_chain_ctx_t *ctx = *state;
    exec_index = 0;
    memset(execution_order, 0, sizeof(execution_order));

    arbiter_t *arb;
    arbiter_init(&arb, &ctx->loop);
    int bus_id = 0;

    cmd_msg_t cmd_low;
    cmd_msg_init_write(&cmd_low, 4003, 1000.0, ARBITER_PRIO_LOW);
    cmd_low.on_start = on_cmd_start;
    cmd_low.user_data = (void *)1;

    cmd_msg_t cmd_high;
    cmd_msg_init_write(&cmd_high, 4003, 0.0, ARBITER_PRIO_EMERGENCY);
    cmd_high.on_start = on_cmd_start;
    cmd_high.user_data = (void *)2;

    arbiter_submit_cmd(arb, bus_id, &cmd_low);
    arbiter_submit_cmd(arb, bus_id, &cmd_high);

    // 驱动两轮调度
    for (int i = 0; i < 5; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        uv_sleep(10);
    }
    arbiter_on_io_done(arb, bus_id, &cmd_high);

    for (int i = 0; i < 5; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        uv_sleep(10);
    }

    assert_int_equal(execution_order[0], 2);
    assert_int_equal(execution_order[1], 1);

    arbiter_shutdown(arb);
}

static stage_ret_t _malicious_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    (void)s;
    (void)ev;

    uv_sleep(100);  // 模拟耗时 100ms，超过 50ms 的阈值

    return STAGE_OK;
}

/**
 * [S17] 逻辑熔断测试
 */
static void test_scenario_circuit_breaker(void **state) {
    full_chain_ctx_t *ctx = *state;

    pipeline_t *p = stream_engine_add_pipeline(ctx->engine, "breaker_test", 5001);

    // 1. 注入流氓算子并获取句柄
    pipeline_stage_t *bad_stage =
        pipeline_add_stage(p, "bad_stage", _malicious_proc, NULL, NULL, NULL, NULL);

    // 2. 物理发布数据
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 5001, 1.0, uv_now(&ctx->loop));
    datahub_publish(&ctx->hub, &msg);

    // 3. 驱动执行 (预期 bad_stage 会执行 100ms)
    for (int i = 0; i < 15; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        uv_sleep(10);
    }

    // 4. 断言该算子已被熔断 (TIMEOUT)
    assert_int_equal(pipeline_stage_get_health(bad_stage), STAGE_HEALTH_TIMEOUT);
}

/* ---------------- main ---------------- */

static int ota_started = 0;

static int regular_started = 0;

static void on_ota_start(void *user_data) {
    (void)user_data;
    ota_started = 1;
}

static void on_regular_start(void *user_data) {
    (void)user_data;
    regular_started = 1;
}

/**

 * [S7] 总线独占锁定测试 (OTA 场景)

 */

static void test_scenario_ota_exclusive_lock(void **state) {
    full_chain_ctx_t *ctx = *state;

    ota_started = 0;

    regular_started = 0;

    arbiter_t *arb;

    arbiter_init(&arb, &ctx->loop);

    int bus_id = 0;

    // 1. 设置总线为独占模式 (S7 OTA 开始)

    arbiter_set_exclusive(arb, bus_id, true);

    // 2. 提交一个普通指令

    cmd_msg_t cmd_reg;

    cmd_msg_init_write(&cmd_reg, 4001, 1.0, ARBITER_PRIO_NORMAL);

    cmd_reg.on_start = on_regular_start;

    arbiter_submit_cmd(arb, bus_id, &cmd_reg);

    // 3. 提交一个 OTA 指令

    cmd_msg_t cmd_ota;

    cmd_msg_init_write(&cmd_ota, 0, 0, ARBITER_PRIO_EMERGENCY);

    cmd_ota.action = CMD_ACTION_OTA;  // 标记为 OTA

    cmd_ota.on_start = on_ota_start;

    arbiter_submit_cmd(arb, bus_id, &cmd_ota);

    // 4. 驱动调度

    for (int i = 0; i < 5; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);

        uv_sleep(10);
    }

    // 验证：在独占模式下，普通指令不应启动，OTA 指令应该启动

    assert_int_equal(ota_started, 1);

    assert_int_equal(regular_started, 0);

    // 5. 完成 OTA，解除独占

    arbiter_on_io_done(arb, bus_id, &cmd_ota);

    arbiter_set_exclusive(arb, bus_id, false);

    // 6. 再次驱动调度

    for (int i = 0; i < 5; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);

        uv_sleep(10);
    }

    // 验证：普通指令现在应该启动了

    assert_int_equal(regular_started, 1);

    arbiter_shutdown(arb);
}

static int sync_called = 0;

static void _mock_emergency_handler(void *user_data) {
    (void)user_data;

    sync_called = 1;
}

/**

 * [S14] 掉电保护测试

 * 验证：系统在收到停机信号时，能触发紧急同步钩子。

 */

static void test_scenario_power_off_protection(void **state) {
    full_chain_ctx_t *ctx = *state;

    sync_called = 0;

    // 1. 注册一个紧急同步钩子 (模拟 Persistence Manager 的行为)

    // 目前 service_t 尚未暴露此钩子，TDD 将驱动我们增加它

    service_set_emergency_hook(ctx->svc, _mock_emergency_handler, NULL);

    // 2. 模拟发送 SIGTERM (libuv 中通常映射为 stop 指令)

    // 或者直接调用 service_stop，它应该触发所有组件的紧急刷盘

    service_stop(ctx->svc);

    // 3. 驱动一轮循环处理 Close 回调

    uv_run(&ctx->loop, UV_RUN_NOWAIT);

    // 4. 断言紧急同步钩子被物理执行过

    assert_int_equal(sync_called, 1);
}

static int chunks_sent = 0;

static size_t bytes_sent = 0;

static void _mock_transfer_tx(net_buf_t *buf) {
    chunks_sent++;

    bytes_sent += buf->b.len;

    net_buf_unref(buf);
}

/**

 * [S15] 大文件非结构化传输测试

 * 验证：大文件能被物理分片为 net_buf 链，并逐个发送。

 */

static void test_scenario_bulk_file_transfer(void **state) {
    full_chain_ctx_t *ctx = *state;

    chunks_sent = 0;

    bytes_sent = 0;

    // 1. 准备一个 10KB 的虚拟文件内容

    size_t file_size = 10240;

    uint8_t *dummy_content = malloc(file_size);

    memset(dummy_content, 0x55, file_size);

    // 2. 初始化一个专门用于传输的 Buffer 池

    net_buf_pool_t tx_pool;

    net_buf_pool_init(&tx_pool, 16, 1024, 0, NULL, NULL);  // 每个分片 1KB

    // 3. 执行分片逻辑 (TDD 将驱动出 net_buf_from_mem 类似的工具)

    // 模拟从文件系统读取并分片

    size_t offset = 0;

    while (offset < file_size) {
        size_t chunk_len = (file_size - offset > 1024) ? 1024 : (file_size - offset);

        net_buf_t *nb = net_buf_alloc(&tx_pool, 0);

        assert_non_null(nb);

        net_buf_simple_add_mem(&nb->b, dummy_content + offset, chunk_len);

        _mock_transfer_tx(nb);

        offset += chunk_len;
    }

    // 4. 验证分片结果

    assert_int_equal(chunks_sent, 10);

    assert_int_equal(bytes_sent, 10240);

    net_buf_pool_destroy(&tx_pool);

    free(dummy_content);
}

static int link_switch_triggered = 0;

static void _mock_link_change_cb(net_link_t new_link, void *user_data) {
    (void)user_data;

    link_switch_triggered = (new_link == LINK_SECONDARY);
}

/**

 * [S19] 多链路冗余与切换测试

 * 验证：主链路断开后，系统自动切备用并改变策略。

 */

static void test_scenario_link_redundancy(void **state) {
    full_chain_ctx_t *ctx = *state;

    link_switch_triggered = 0;

    // 1. 设置链路感知回调

    service_set_link_monitor(ctx->svc, _mock_link_change_cb, NULL);

    // 2. 模拟主链路失效 (Signal Primary Down)

    service_report_link_status(ctx->svc, LINK_PRIMARY, false);

    // 3. 驱动 Loop

    uv_run(&ctx->loop, UV_RUN_NOWAIT);

    // 4. 断言：系统已切入备用链路

    assert_int_equal(link_switch_triggered, 1);
}

int main(void) {
    const struct CMUnitTest tests[] = {

        cmocka_unit_test_setup_teardown(test_scenario_shadow_service, setup, teardown),

        cmocka_unit_test_setup_teardown(test_scenario_priority_arbitration, setup, teardown),

        cmocka_unit_test_setup_teardown(test_scenario_circuit_breaker, setup, teardown),

        cmocka_unit_test_setup_teardown(test_scenario_ota_exclusive_lock, setup, teardown),

        cmocka_unit_test_setup_teardown(test_scenario_power_off_protection, setup, teardown),

        cmocka_unit_test_setup_teardown(test_scenario_bulk_file_transfer, setup, teardown),

        cmocka_unit_test_setup_teardown(test_scenario_link_redundancy, setup, teardown),

    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
