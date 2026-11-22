#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "datahub_priv.h"
#include "dcu_698_axdr.h"
#include "dcu_discovery.h"
#include "dcu_event_monitor.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dcu_security.h"
#include "dcu_storage.h"
#include "stream_bus_datahub_adapter.h"
#include "stream_engine.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))

/* date_time_s 是设备本地民用时间：固定东八区，线上下发与回读才有确定语义。 */
static char g_saved_tz[128];
static int g_had_tz;

static int setup_timezone(void** state) {
    (void)state;
    const char* tz = getenv("TZ");
    g_had_tz = tz != NULL;
    if (g_had_tz) snprintf(g_saved_tz, sizeof(g_saved_tz), "%s", tz);
    setenv("TZ", "CST-8", 1);
    tzset();
    return 0;
}

static int teardown_timezone(void** state) {
    (void)state;
    if (g_had_tz) {
        setenv("TZ", g_saved_tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
    return 0;
}

/* 测试只读 event_history 的实际落库值，验证存储单位；产品侧没有事件读取路径。 */
static void _event_row_times(dcu_storage_t* storage, uint16_t oi, uint64_t* start, uint64_t* end) {
    *start = 0;
    *end = 0;
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(storage->db,
                           "SELECT start_time, end_time FROM event_history WHERE oi = ?;", -1,
                           &stmt, NULL) != SQLITE_OK) {
        return;
    }
    sqlite3_bind_int(stmt, 1, oi);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *start = (uint64_t)sqlite3_column_int64(stmt, 0);
        *end = (uint64_t)sqlite3_column_int64(stmt, 1);
    }
    sqlite3_finalize(stmt);
}

/* 同 OI 可能有多条历史行，按地址精确取最近一条。 */
static void _event_row_times_for(dcu_storage_t* storage, const char* addr, uint16_t oi,
                                 uint64_t* start, uint64_t* end) {
    *start = 0;
    *end = 0;
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(storage->db,
                           "SELECT start_time, end_time FROM event_history WHERE meter_address = ? "
                           "AND oi = ? ORDER BY rowid DESC LIMIT 1;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return;
    }
    sqlite3_bind_text(stmt, 1, addr, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, oi);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *start = (uint64_t)sqlite3_column_int64(stmt, 0);
        *end = (uint64_t)sqlite3_column_int64(stmt, 1);
    }
    sqlite3_finalize(stmt);
}

static int _mock_count_freezes(void* ctx, uint16_t* out_count) {
    return dcu_storage_count_freezes((dcu_storage_t*)ctx, out_count);
}

static int _mock_read_shadow(void* ctx, uint32_t tag_id, uint32_t device_id, void* out_msg) {
    return datahub_read_shadow_device((datahub_t*)ctx, tag_id, device_id, (pipe_msg_t*)out_msg);
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

/**
 * @brief 集中器高级模块单元测试：ESAM 鉴权、搜表、流式事件判定与冻结落盘
 */
static void test_dcu_advanced_features_dispatch(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();

    // ==============================================================================
    // 1. 初始化底座及专属业务模块单例
    // ==============================================================================
    dcu_inventory_t test_inv;
    dcu_inventory_init(&test_inv);
    dcu_inventory_add(&test_inv, "10000001", "DLT698", 9600, 1);

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
    evt_3000.last_start_time = (dcu_event_stamp_t){0};
    evt_3000.last_end_time = (dcu_event_stamp_t){0};

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
    evt_3005.last_start_time = (dcu_event_stamp_t){0};
    evt_3005.last_end_time = (dcu_event_stamp_t){0};

    router_sink_ops_t router_sink = {.ctx = &storage,
                                     .record_telemetry = (void*)dcu_storage_write_telemetry,
                                     .count_freezes = _mock_count_freezes};

    router_bus_ops_t router_bus = {.ctx = &hub,
                                   .msg_size = sizeof(pipe_msg_t),
                                   .publish_realtime = _mock_publish_realtime,
                                   .read_shadow = _mock_read_shadow};

    // 2. 初始化核心对象字典契约路由器
    ret = dcu_router_init_ex(&test_inv, &poller, router_sink, &esam, &discovery, &evt_3000,
                             &evt_3005, router_bus);
    assert_int_equal(ret, 0);

    // ==============================================================================
    // 2. 验证 ESAM 安全鉴权机制 (OI_ESAM_SECURITY 方法调用)
    // ==============================================================================
    uint8_t master_challenge[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t esam_response[8] = {0};

    /* 鉴权是安全模块的单元能力；OI F100 方法 1 是标准复位，不是鉴权。 */
    assert_int_equal(dcu_security_authenticate(&esam, master_challenge, esam_response), 0);
    assert_true(esam.is_authenticated);
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
    assert_false(mac_valid);

    ret = dcu_router_action(((uint32_t)OI_ESAM_SECURITY << 16) | 0x0100, NULL, NULL);
    assert_int_equal(ret, 0);
    assert_false(esam.is_authenticated);
    for (int i = 0; i < 16; i++) assert_int_equal(esam.session_key[i], 0);

    // 3. F200 的 A2/A3/A4 类已定义但未广告（DAR 5）；没有真实串口重配路径，全部不对外
    uint32_t baudrate = 0;
    ret = dcu_router_get_attr(((uint32_t)OI_CHANNEL_CONFIG << 16) | 0x0200, &baudrate);
    assert_int_equal(ret, -1);
    uint32_t new_baud = 2400;
    ret = dcu_router_set_attr(((uint32_t)OI_CHANNEL_CONFIG << 16) | 0x0200, &new_baud);
    assert_int_equal(ret, -1);

    // ==============================================================================
    // 4. 验证下行自动搜表机制 (6002 只暴露计数, 6003 是结构不是对象)
    // ==============================================================================
    uint8_t disc_status = 99;
    ret = dcu_router_get_attr(((uint32_t)OI_METER_DISCOVERY << 16) | 0x0200,
                              &disc_status);  // 6002 A2 是搜表结果数组，当前关闭
    assert_int_equal(ret, -1);
    uint32_t duration = 10;
    int action_res = -1;
    ret = dcu_router_action(((uint32_t)OI_METER_DISCOVERY << 16) | 0x7F00, &duration,
                            &action_res);  // OMD 方法 127 实时启动扫表
    assert_int_equal(ret, 0);
    assert_int_equal(action_res, 0);
    assert_int_equal(discovery.status, DISCOVERY_STATUS_SCANNING);

    // 模拟后台物理广播探测帧捕获
    dcu_discovery_tick_simulate(&discovery);
    assert_int_equal(discovery.status, DISCOVERY_STATUS_COMPLETED);

    uint16_t found_count = 0;
    ret = dcu_router_get_attr(((uint32_t)OI_METER_DISCOVERY << 16) | 0x0300,
                              &found_count);  // 6002 A3 是搜表结果记录数
    assert_int_equal(ret, 0);
    assert_int_equal(found_count, 1);

    char discovered_addr[32] = {0};
    ret = dcu_router_get_attr(((uint32_t)OI_DISCOVERY_RESULT << 16) | 0x0200,
                              discovered_addr);  // 6003 是结果结构，不是对象
    assert_int_equal(ret, -1);
    assert_string_equal(discovery.discovered_tsas[0], "10000003");  // 地址匹配

    // ==============================================================================
    // 5. 验证流式失压与过流事件监控管线 (Class 7 Event Monitor & Logger Pipeline)
    // ==============================================================================
    // (hub已在上方初始化)

    stream_engine_t* stream_engine = stream_engine_create_datahub(&hub, loop, NULL, NULL);
    pipeline_t* evt_pipeline = stream_engine_add_pipeline(stream_engine, "TEST_EVT_PIPELINE", 0);

    // 装配越限判定与快照记录管线
    pipeline_stage_t* mon_stage = stage_event_monitor_create(&test_inv, &evt_3000, &evt_3005);
    pipeline_add_stage(evt_pipeline, mon_stage->name, mon_stage->process, mon_stage->priv_state,
                       mon_stage->cleanup_state, NULL, NULL);

    event_sink_ops_t event_sink = {.ctx = &storage,
                                   .archive_event = _test_archive_event,
                                   .archive_freeze = _test_archive_freeze};
    pipeline_stage_t* log_stage =
        stage_event_logger_create(&test_inv, &evt_3000, &evt_3005, &hub, event_sink);
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
        scalar_msg.ts = base_ts + (uint64_t)i * (uint64_t)1000000000ULL;
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

    // Class 24 的 A6 走 A-XDR；旧标量入口对复杂属性仍拒绝。
    dcu_event_record_t first_evt_rec = evt_3000.records[0];
    ret = dcu_router_get_attr(((uint32_t)OI_VOLTAGE_DROP << 16) | 0x0200, &first_evt_rec);
    assert_int_equal(ret, -2);

    first_evt_rec = evt_3000.records[0];
    assert_int_equal(first_evt_rec.index, 1);
    assert_float_equal(first_evt_rec.snapshot_value, 150.0f, 0.01f);  // 快照电压 150V
    assert_true(first_evt_rec.is_active);                             // 事件处于发生状态
    assert_int_equal(first_evt_rec.start_time.mono_ns, base_ts + 5ULL * 1000000000ULL);

    /* 墙钟来自 CLOCK_REALTIME，不是 ev->ts（单调纳秒）；容差 5s 覆盖调度抖动。 */
    uint64_t wall_now = (uint64_t)time(NULL);
    assert_true(first_evt_rec.start_time.epoch_s >= wall_now - 5 &&
                first_evt_rec.start_time.epoch_s <= wall_now + 5);

    /* 线上 A6 的 date_time_s 往返解码回记录里的 epoch_s，而不是 1970 或单调纳秒算出的日期。 */
    uint8_t a6[256];
    size_t a6_len = 0;
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 6), a6, sizeof(a6), &a6_len), 0);
    // array 头(2) + structure 头(2) + 序号(5) 之后是事件发生时间。
    uint64_t wire_epoch = 0;
    assert_int_equal(dcu_698_date_time_s_to_epoch(a6 + 9, 8, &wire_epoch), 0);
    assert_true(wire_epoch >= wall_now - 5 && wire_epoch <= wall_now + 5);

    /* 落库单位同步为墙钟整秒；产品侧没有 event_history 读取路径，这里只核对写入值。 */
    uint64_t row_start = 0, row_end = 0;
    _event_row_times(&storage, OI_VOLTAGE_DROP, &row_start, &row_end);
    assert_true(row_start >= wall_now - 5 && row_start <= wall_now + 5);
    assert_int_equal(row_end, 0);

    // 💡 C. 注入电压恢复 220.5V 标量，失压事件应消除
    scalar_msg.ts = base_ts + 10ULL * 1000000000ULL;
    scalar_msg.payload.val.f = 220.5f;
    stream_engine_push(stream_engine, &scalar_msg);

    // 消除时记录序号不变，但 is_active 状态应改变，且时标填充
    first_evt_rec = evt_3000.records[0];
    assert_false(first_evt_rec.is_active);  // 状态成功变更为恢复 (false)
    assert_int_equal(first_evt_rec.end_time.mono_ns, base_ts + 10ULL * 1000000000ULL);
    wall_now = (uint64_t)time(NULL);
    assert_true(first_evt_rec.end_time.epoch_s >= wall_now - 5 &&
                first_evt_rec.end_time.epoch_s <= wall_now + 5);

    assert_int_equal(first_evt_rec.start_time.epoch_s, row_start);
    _event_row_times(&storage, OI_VOLTAGE_DROP, &row_start, &row_end);
    assert_int_equal(row_start, first_evt_rec.start_time.epoch_s);
    assert_int_equal(row_end, first_evt_rec.end_time.epoch_s);

    /* 旧行策略：旧代码在 event_history 里写的是 uv_hrtime 单调纳秒，不是公历时间。
     * 没有产品读取路径；未来读取必须按单位/版本识别旧行，公历区间判定只做最后拒绝。
     * 这里把旧行值喂给同一判定和 A6 编码，锁定旧行永远不会被渲染成日期。 */
    assert_int_equal(dcu_storage_write_event(&storage, "LegacyMeter", OI_VOLTAGE_DROP,
                                             1716500000000000000ULL, 0, 1.0f),
                     0);
    uint64_t legacy_start = 0, legacy_end = 0;
    _event_row_times_for(&storage, "LegacyMeter", OI_VOLTAGE_DROP, &legacy_start, &legacy_end);
    assert_int_equal(legacy_start, 1716500000000000000ULL);
    assert_false(dcu_698_epoch_is_civil(legacy_start));

    dcu_event_stamp_t saved_start = evt_3000.records[0].start_time;
    evt_3000.records[0].start_time = (dcu_event_stamp_t){.mono_ns = 1, .epoch_s = legacy_start};
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 6), a6, sizeof(a6), &a6_len),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);
    evt_3000.records[0].start_time = saved_start;

    // ==============================================================================
    // 6. 验证日冻结管线与 SQLite RSD 持久化 (Class 9 Freezing & Query)
    // ==============================================================================
    pipeline_t* freeze_pipeline =
        stream_engine_add_pipeline(stream_engine, "TEST_FREEZE_PIPELINE", 0);
    pipeline_stage_t* freeze_stage = stage_periodic_freeze_create_ex(&test_inv, &hub, event_sink);
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

    // 5004 A2 是冻结数据数组，不再用条数冒充冻结表。
    uint16_t freeze_count = 0;
    ret = dcu_router_get_attr(OAD_DAILY_FREEZE, &freeze_count);
    assert_int_not_equal(ret, 0);

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
    dcu_inventory_destroy(&test_inv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_advanced_features_dispatch),
    };
    return cmocka_run_group_tests(tests, setup_timezone, teardown_timezone);
}
