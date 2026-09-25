#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>

#include "dcu_discovery.h"
#include "dcu_event_monitor.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dcu_security.h"
#include "dcu_storage.h"
#include "stream_engine.h"

static int _mock_count_freezes(void* ctx, uint16_t* out_count) {
    return dcu_storage_count_freezes((dcu_storage_t*)ctx, out_count);
}

static int _mock_read_shadow(void* ctx, uint32_t idx, void* out_msg) {
    return datahub_read_shadow_idx((datahub_t*)ctx, idx, (pipe_msg_t*)out_msg);
}

static int _mock_publish_realtime(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                                  uint64_t timestamp_ns) {
    (void)ctx;
    (void)tag_id;
    (void)meter_idx;
    (void)val;
    (void)timestamp_ns;
    return 0;
}

static int _test_archive_event(void* ctx, const char* addr, uint16_t event_code,
                               uint64_t start_time, uint64_t end_time, float snapshot_val) {
    return dcu_storage_write_event((dcu_storage_t*)ctx, addr, event_code, start_time, end_time,
                                   snapshot_val);
}

static int _test_archive_freeze(void* ctx, const char* addr, uint32_t freeze_oad,
                                uint64_t freeze_time, uint32_t src_oad, double value) {
    return dcu_storage_write_freeze((dcu_storage_t*)ctx, addr, freeze_oad, freeze_time, src_oad,
                                    value);
}

dcu_inventory_t g_dcu_inventory;

/**
 * @brief 集中器高级模块单元测试：ESAM 鉴权、搜表、流式事件判定与冻结落盘
 */
static void test_dcu_advanced_features_dispatch(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();

    // ==============================================================================
    // 1. 初始化底座及专属业务模块单例
    // ==============================================================================
    dcu_inventory_init(&g_dcu_inventory);
    dcu_inventory_add(&g_dcu_inventory, "10000001", "DLT698", 9600, 1, NULL);

    dcu_poller_t poller;
    dcu_poller_init(&poller);

    dcu_storage_t storage;
    int ret = dcu_storage_init(&storage, loop, ":memory:");  // 使用内存数据库测试，秒开秒关
    assert_int_equal(ret, 0);

    datahub_t hub;
    datahub_init(&hub, loop, "dcu_test_bus", 1024, 1024, NULL, 0, NULL, NULL, NULL);
    hub.map_fn_v2 = dcu_tag_map_fn;

    dcu_esam_t esam;
    dcu_security_init(&esam);

    dcu_discovery_t discovery;
    dcu_discovery_init(&discovery);

    dcu_event_obj_t evt_3000;
    evt_3000.oi = OI_VOLTAGE_DROP;
    evt_3000.record_count = 0;
    memset(evt_3000.records, 0, sizeof(evt_3000.records));
    evt_3000.snapshot_count = 1;
    evt_3000.snapshot_oads[0] = OAD_VOLTAGE;
    evt_3000.threshold = 176.0f;
    evt_3000.delay_s = 5;
    evt_3000.report_flag = 1;
    evt_3000.is_enabled = true;
    evt_3000.last_start_time = 0;
    evt_3000.last_end_time = 0;

    dcu_event_obj_t evt_3005;
    evt_3005.oi = OI_OVER_CURRENT;
    evt_3005.record_count = 0;
    memset(evt_3005.records, 0, sizeof(evt_3005.records));
    evt_3005.snapshot_count = 1;
    evt_3005.snapshot_oads[0] = OAD_CURRENT;
    evt_3005.threshold = 5.0f;
    evt_3005.delay_s = 3;
    evt_3005.report_flag = 1;
    evt_3005.is_enabled = true;
    evt_3005.last_start_time = 0;
    evt_3005.last_end_time = 0;

    router_sink_ops_t router_sink = {.ctx = &storage,
                                     .record_telemetry = (void*)dcu_storage_write_telemetry,
                                     .count_freezes = _mock_count_freezes};

    router_bus_ops_t router_bus = {.ctx = &hub,
                                   .msg_size = sizeof(pipe_msg_t),
                                   .publish_realtime = _mock_publish_realtime,
                                   .read_shadow_idx = _mock_read_shadow};

    // 2. 初始化核心对象字典契约路由器
    ret = dcu_router_init_ex(&g_dcu_inventory, &poller, router_sink, &esam, &discovery, &evt_3000,
                             &evt_3005, router_bus);
    assert_int_equal(ret, 0);

    // ==============================================================================
    // 2. 验证 ESAM 安全鉴权机制 (OI_ESAM_SECURITY 方法调用)
    // ==============================================================================
    uint8_t master_challenge[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t esam_response[8] = {0};

    // 验证方法 1 路由调用 (OMD: OI_ESAM_SECURITY 方法 1)
    ret = dcu_router_action(((uint32_t)OI_ESAM_SECURITY << 16) | 0x0100, master_challenge,
                            esam_response);
    assert_int_equal(ret, 0);
    assert_true(esam.is_authenticated);  // 身份应鉴权成功

    // 比对响应密文
    for (int i = 0; i < 8; i++) {
        assert_int_equal(esam_response[i], (uint8_t)(master_challenge[i] ^ 0xA5 ^ (uint8_t)i));
    }

    // 验证 MAC 报文防篡改签名校验
    uint8_t test_payload[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t correct_mac[4] = {0};
    for (int i = 0; i < 4; i++) {
        correct_mac[i % 4] ^= (uint8_t)(test_payload[i] ^ esam.session_key[i % 16]);
    }

    bool mac_valid = dcu_security_verify_mac(&esam, test_payload, 4, correct_mac);
    assert_true(mac_valid);  // 正确 MAC 应验证通过

    uint8_t invalid_mac[4] = {0, 0, 0, 0};
    mac_valid = dcu_security_verify_mac(&esam, test_payload, 4, invalid_mac);
    assert_false(mac_valid);  // 篡改后的 MAC 应拒绝通过

    // ==============================================================================
    // 3. 验证外设 RS485 参数动态微调 (OI_CHANNEL_CONFIG OAD 读写)
    // ==============================================================================
    uint32_t baudrate = 0;
    ret = dcu_router_get_attr(((uint32_t)OI_CHANNEL_CONFIG << 16) | 0x0200,
                              &baudrate);  // 获取默认波特率 (9600)
    assert_int_equal(ret, 0);
    assert_int_equal(baudrate, 9600);

    uint32_t new_baud = 2400;
    ret = dcu_router_set_attr(((uint32_t)OI_CHANNEL_CONFIG << 16) | 0x0200,
                              &new_baud);  // SET 波特率修改为 2400
    assert_int_equal(ret, 0);

    baudrate = 0;
    ret = dcu_router_get_attr(((uint32_t)OI_CHANNEL_CONFIG << 16) | 0x0200,
                              &baudrate);  // 重新 GET 校验修改结果
    assert_int_equal(ret, 0);
    assert_int_equal(baudrate, 2400);

    // ==============================================================================
    // 4. 验证下行自动搜表机制 (6002 OMD 启动, 6003 OAD 召测结果)
    // ==============================================================================
    uint8_t disc_status = 99;
    ret = dcu_router_get_attr(((uint32_t)OI_METER_DISCOVERY << 16) | 0x0200,
                              &disc_status);  // GET 搜表状态，应为 IDLE (0)
    assert_int_equal(ret, 0);
    assert_int_equal(disc_status, DISCOVERY_STATUS_IDLE);

    uint32_t duration = 10;
    int action_res = -1;
    ret = dcu_router_action(((uint32_t)OI_METER_DISCOVERY << 16) | 0x7F00, &duration,
                            &action_res);  // OMD 方法 127 实时启动扫表
    assert_int_equal(ret, 0);
    assert_int_equal(action_res, 0);

    ret = dcu_router_get_attr(((uint32_t)OI_METER_DISCOVERY << 16) | 0x0200,
                              &disc_status);  // 应切换为 SCANNING (1)
    assert_int_equal(ret, 0);
    assert_int_equal(disc_status, DISCOVERY_STATUS_SCANNING);

    // 模拟后台物理广播探测帧捕获
    dcu_discovery_tick_simulate(&discovery);

    ret = dcu_router_get_attr(((uint32_t)OI_METER_DISCOVERY << 16) | 0x0200,
                              &disc_status);  // 扫表完成后应变为 COMPLETED (2)
    assert_int_equal(ret, 0);
    assert_int_equal(disc_status, DISCOVERY_STATUS_COMPLETED);

    uint16_t found_count = 0;
    ret = dcu_router_get_attr(((uint32_t)OI_METER_DISCOVERY << 16) | 0x0300,
                              &found_count);  // 发现野生电表数应为 1
    assert_int_equal(ret, 0);
    assert_int_equal(found_count, 1);

    char discovered_addr[32] = {0};
    ret = dcu_router_get_attr(((uint32_t)OI_DISCOVERY_RESULT << 16) | 0x0200,
                              discovered_addr);  // 从 6003 结果中召测捕获的地址
    assert_int_equal(ret, 0);
    assert_string_equal(discovered_addr, "10000003");  // 地址匹配

    // ==============================================================================
    // 5. 验证流式失压与过流事件监控管线 (Class 7 Event Monitor & Logger Pipeline)
    // ==============================================================================
    // (hub已在上方初始化)

    stream_engine_t* stream_engine = stream_engine_create(&hub, loop, NULL, NULL);
    pipeline_t* evt_pipeline = stream_engine_add_pipeline(stream_engine, "TEST_EVT_PIPELINE", 0);

    // 装配越限判定与快照记录管线
    pipeline_stage_t* mon_stage = stage_event_monitor_create(&evt_3000, &evt_3005);
    pipeline_add_stage(evt_pipeline, mon_stage->name, mon_stage->process, mon_stage->priv_state,
                       mon_stage->cleanup_state, NULL, NULL);

    event_sink_ops_t event_sink = {.ctx = &storage,
                                   .archive_event = _test_archive_event,
                                   .archive_freeze = _test_archive_freeze};
    pipeline_stage_t* log_stage = stage_event_logger_create(&evt_3000, &evt_3005, &hub, event_sink);
    pipeline_add_stage(evt_pipeline, log_stage->name, log_stage->process, log_stage->priv_state,
                       log_stage->cleanup_state, NULL, NULL);

    uint64_t base_ts = 1716500000ULL * 1000000000ULL;

    // 💡 A. 注入 220V 电压标量，一切应正常，无失压记录产生
    pipe_msg_t scalar_msg;
    memset(&scalar_msg, 0, sizeof(pipe_msg_t));
    scalar_msg.tag_id = OAD_VOLTAGE;
    scalar_msg.type = PIPEMSG_TYPE_SCALAR;
    scalar_msg.ts = base_ts;
    scalar_msg.payload.val.f = 220.0f;
    stream_engine_push(stream_engine, &scalar_msg);

    assert_int_equal(evt_3000.record_count, 0);  // 记录数应为 0

    // 💡 B. 电压骤降为 150V (<176.0V 阈值)
    // 模拟高频越限持续时间 1s -> 2s -> 3s -> 4s -> 5s (失压延时 5 秒触发)
    for (int i = 0; i <= 6; i++) {
        scalar_msg.ts = base_ts + (uint64_t)i * 1000000000ULL;
        scalar_msg.payload.val.f = 150.0f;
        stream_engine_push(stream_engine, &scalar_msg);

        // 💡 物理驱动：强制运行主循环以处理异步 Pipeline 任务
        uv_run(loop, UV_RUN_NOWAIT);
    }

    // 给异步线程一点点时间处理
    for (int i = 0; i < 50; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (evt_3000.record_count > 0) break;
        usleep(1000);
    }

    // 断言第 5 秒及以后应在失压对象中生成首条失压事件记录！
    assert_int_equal(evt_3000.record_count, 1);

    dcu_event_record_t first_evt_rec;
    ret = dcu_router_get_attr(((uint32_t)OI_VOLTAGE_DROP << 16) | 0x0200,
                              &first_evt_rec);  // 通过 OAD 召测首条事件记录
    assert_int_equal(ret, 0);
    assert_int_equal(first_evt_rec.index, 1);
    assert_float_equal(first_evt_rec.snapshot_value, 150.0f, 0.01f);  // 快照电压 150V
    assert_true(first_evt_rec.is_active);                             // 事件处于发生状态
    assert_int_equal(first_evt_rec.start_time, base_ts + 5ULL * 1000000000ULL);  // 触发点正确

    // 💡 C. 注入电压恢复 220.5V 标量，失压事件应消除
    scalar_msg.ts = base_ts + 10ULL * 1000000000ULL;
    scalar_msg.payload.val.f = 220.5f;
    stream_engine_push(stream_engine, &scalar_msg);

    // 消除时记录序号不变，但 is_active 状态应改变，且时标填充
    ret = dcu_router_get_attr(((uint32_t)OI_VOLTAGE_DROP << 16) | 0x0200, &first_evt_rec);
    assert_int_equal(ret, 0);
    assert_false(first_evt_rec.is_active);  // 状态成功变更为恢复 (false)
    assert_int_equal(first_evt_rec.end_time, base_ts + 10ULL * 1000000000ULL);  // 消除时标正确

    // ==============================================================================
    // 6. 验证日冻结管线与 SQLite RSD 持久化 (Class 9 Freezing & Query)
    // ==============================================================================
    pipeline_t* freeze_pipeline =
        stream_engine_add_pipeline(stream_engine, "TEST_FREEZE_PIPELINE", 0);
    pipeline_stage_t* freeze_stage = stage_periodic_freeze_create_ex(&hub, event_sink);
    pipeline_add_stage(freeze_pipeline, freeze_stage->name, freeze_stage->process,
                       freeze_stage->priv_state, freeze_stage->cleanup_state, NULL, NULL);

    // 往 DataHub 总线写入累计电能 OAD_ACTIVE_ENERGY 的最新实时影子数据
    pipe_msg_t energy_msg;
    memset(&energy_msg, 0, sizeof(pipe_msg_t));
    energy_msg.tag_id = OAD_ACTIVE_ENERGY;
    energy_msg.type = PIPEMSG_TYPE_SCALAR;
    energy_msg.payload.val.f = 12543.21f;
    datahub_publish(&hub, &energy_msg);

    // 模拟时钟触发 OAD_DAILY_FREEZE 定期日冻结事件信号
    pipe_msg_t trigger_msg;
    memset(&trigger_msg, 0, sizeof(pipe_msg_t));
    trigger_msg.tag_id = OAD_DAILY_FREEZE;
    trigger_msg.type = PIPEMSG_TYPE_SCALAR;
    trigger_msg.ts = base_ts;
    stream_engine_push(stream_engine, &trigger_msg);

    // 校验冻结数据总数应为 3 (电能量、电压、电流快照)
    uint16_t freeze_count = 0;
    ret = dcu_router_get_attr(OAD_DAILY_FREEZE, &freeze_count);
    assert_int_equal(ret, 0);
    assert_int_equal(freeze_count, 3);

    // 模拟北向发起 RSD 选择性历史召测 (GET.Record 时间区间内检索)
    dcu_freeze_record_t query_results[5];
    uint16_t query_count = 0;
    ret = dcu_storage_query_freeze(&storage, "10000001", OAD_DAILY_FREEZE,
                                   base_ts - 10ULL * 1000000000ULL, base_ts + 10ULL * 1000000000ULL,
                                   query_results, 5, &query_count);
    assert_int_equal(ret, 0);
    assert_int_equal(query_count, 3);  // 成功过滤查询出 3 条历史断面纪录

    // 检验快照中累计电能数值是否匹配
    bool found_energy_snapshot = false;
    for (int i = 0; i < 3; i++) {
        if (query_results[i].source_oad == OAD_ACTIVE_ENERGY) {
            assert_float_equal(query_results[i].frozen_value, 12543.21f, 0.01f);
            found_energy_snapshot = true;
        }
    }
    assert_true(found_energy_snapshot);

    // ==============================================================================
    // 7. 回收与干净释放
    // ==============================================================================
    stream_engine_destroy(stream_engine);
    datahub_destroy(&hub);

    dcu_discovery_destroy(&discovery);
    dcu_router_destroy();
    dcu_storage_destroy(&storage);
    dcu_inventory_destroy(&g_dcu_inventory);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_advanced_features_dispatch),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
