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
#include "dcu_698_var.h"
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

    /* 698 北向不暴露软件 ESAM 会话：方法 1 按类定义回 DAR 5，不重置、不伪造。 */
    ret = dcu_router_action(((uint32_t)OI_ESAM_SECURITY << 16) | 0x0100, NULL, NULL);
    assert_int_equal(ret, EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_true(esam.is_authenticated);
    uint8_t mac_still[4] = {0};
    for (int i = 0; i < 4; i++) mac_still[i % 4] ^= (uint8_t)(test_payload[i] ^ esam.session_key[i % 16]);
    assert_true(dcu_security_verify_mac(&esam, test_payload, 4, mac_still));

    // 3. F200 的 A2/A3/A4 类已定义但未广告（DAR 5）；没有真实串口重配路径，全部不对外
    uint32_t baudrate = 0;
    ret = dcu_router_get_attr(((uint32_t)OI_CHANNEL_CONFIG << 16) | 0x0200, &baudrate);
    assert_int_equal(ret, -1);
    uint32_t new_baud = 2400;
    ret = dcu_router_set_attr(((uint32_t)OI_CHANNEL_CONFIG << 16) | 0x0200, &new_baud);
    assert_int_equal(ret, -1);

    // 4. 没有下行搜表驱动：方法 127 回 DAR 2，状态保持空闲（表 A.7 A10）
    uint8_t disc_status = 99;
    ret = dcu_router_get_attr(((uint32_t)OI_METER_DISCOVERY << 16) | 0x0200, &disc_status);
    assert_int_equal(ret, -2);
    uint32_t duration = 10;
    int action_res = -1;
    ret = dcu_router_action(((uint32_t)OI_METER_DISCOVERY << 16) | 0x7F00, &duration, &action_res);
    assert_int_equal(ret, EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(discovery.status, DISCOVERY_STATUS_IDLE);

    uint16_t found_count = 0;
    ret = dcu_router_get_attr(((uint32_t)OI_METER_DISCOVERY << 16) | 0x0300,
                              &found_count);  // Class 11 A3 当前元素个数
    assert_int_equal(ret, 0);
    assert_int_equal(found_count, 0);
    ret = dcu_router_get_attr(((uint32_t)OI_METER_DISCOVERY << 16) | 0x0600,
                              &found_count);  // 表 A.7 A6 所有搜表结果记录数
    assert_int_equal(ret, 0);
    assert_int_equal(found_count, 0);

    char discovered_addr[32] = {0};
    ret = dcu_router_get_attr(((uint32_t)0x6003 << 16) | 0x0200,
                              discovered_addr);  // 6003 是结构，不是独立对象
    assert_int_equal(ret, -1);

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
    free(mon_stage);  // 管线复制 stage 字段，工厂对象只是临时描述符。

    event_sink_ops_t event_sink = {.ctx = &storage,
                                   .archive_event = _test_archive_event,
                                   .archive_freeze = _test_archive_freeze};
    pipeline_stage_t* log_stage =
        stage_event_logger_create(&test_inv, &evt_3000, &evt_3005, &hub, event_sink);
    pipeline_add_stage(evt_pipeline, log_stage->name, log_stage->process, log_stage->priv_state,
                       log_stage->cleanup_state, NULL, NULL);
    free(log_stage);

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

    /* 复杂属性不走标量入口。 */
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

    uint8_t a6[256];
    size_t a6_len = 0;
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 6), a6, sizeof(a6), &a6_len), 0);
    enum { A6_ARRAY_HDR = 2, A6_STRUCT_HDR = 2, A6_INDEX_BYTES = 5 };
    uint64_t wire_epoch = 0;
    assert_int_equal(dcu_698_date_time_s_to_epoch(
                         a6 + A6_ARRAY_HDR + A6_STRUCT_HDR + A6_INDEX_BYTES, 8, &wire_epoch),
                     0);
    assert_true(wire_epoch >= wall_now - 5 && wire_epoch <= wall_now + 5);

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

    /* 旧行单位是单调纳秒：没有产品读取路径，未来读取必须按单位/版本识别，
     * 公历区间判定只做最后拒绝，旧行不得被渲染成日期。 */
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
    free(freeze_stage);

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

/* 档案删除用末元素覆盖删除位：删 B 后 D 落到下标 1，再新增的 E 占下标 3（slot 4）。
 * 此时 slot 3 的 D 在下标 1，而下标 3 是 slot 4 的 E；事件管线的 device_id 是
 * slot_id，记录与冻结都必须按 slot 归 D，不能按下标挂到 E。 */
static void on_walk_close_slot_test(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static void test_event_and_freeze_keep_slot_identity_after_archive_shift(void** state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    assert_int_equal(dcu_inventory_add(&inv, "10000001", "DLT698", 9600, 1), 0);
    assert_int_equal(dcu_inventory_add(&inv, "10000002", "DLT698", 9600, 1), 0);
    assert_int_equal(dcu_inventory_add(&inv, "10000003", "DLT698", 9600, 1), 0);
    assert_int_equal(dcu_inventory_add(&inv, "10000004", "DLT698", 9600, 1), 0);
    assert_int_equal(dcu_inventory_remove(&inv, "10000002"), 0);
    assert_int_equal(dcu_inventory_add(&inv, "10000005", "DLT698", 9600, 1), 0);
    assert_int_equal(inv.count, 4);
    assert_string_equal(inv.meters[1].address, "10000004");
    assert_int_equal(inv.meters[1].slot_id, 3);
    assert_string_equal(inv.meters[3].address, "10000005");
    assert_int_equal(inv.meters[3].slot_id, 4);

    dcu_storage_t storage;
    assert_int_equal(dcu_storage_init(&storage, &loop, ":memory:"), 0);

    datahub_t hub;
    datahub_init(&hub, &loop, "dcu_test_slot_identity", 1024, 1024, NULL, 0, NULL, NULL, NULL);
    hub.map_fn_v2 = dcu_tag_map_fn;

    dcu_poller_t poller;
    dcu_poller_init(&poller);
    dcu_discovery_t discovery;
    dcu_discovery_init(&discovery);

    dcu_event_obj_t evt_3000;
    memset(&evt_3000, 0, sizeof(evt_3000));
    evt_3000.oi = OI_VOLTAGE_DROP;
    evt_3000.threshold = 176.0f;
    evt_3000.delay_s = 0;
    evt_3000.is_enabled = true;
    /* F5：A6 关联数据快照必须按事件的 slot_id 取表。 */
    evt_3000.snapshot_count = 1;
    evt_3000.snapshot_oads[0] = OAD_VOLTAGE;
    dcu_event_obj_t evt_3005;
    memset(&evt_3005, 0, sizeof(evt_3005));
    evt_3005.oi = OI_OVER_CURRENT;
    evt_3005.threshold = 5.0f;
    evt_3005.delay_s = 0;
    evt_3005.is_enabled = true;

    router_sink_ops_t router_sink = {.ctx = &storage,
                                     .record_telemetry = (void*)dcu_storage_write_telemetry,
                                     .count_freezes = _mock_count_freezes};
    router_bus_ops_t router_bus = {.ctx = &hub,
                                   .msg_size = sizeof(pipe_msg_t),
                                   .publish_realtime = _mock_publish_realtime,
                                   .read_shadow = _mock_read_shadow};
    assert_int_equal(dcu_router_init_ex(&inv, &poller, router_sink, NULL, &discovery, &evt_3000,
                                        &evt_3005, router_bus),
                     0);

    event_sink_ops_t event_sink = {.ctx = &storage,
                                   .archive_event = _test_archive_event,
                                   .archive_freeze = _test_archive_freeze};

    stream_engine_t* se = stream_engine_create_datahub(&hub, &loop, NULL, NULL);
    pipeline_t* evt_pipeline = stream_engine_add_pipeline(se, "TEST_SLOT_EVT", 0);
    pipeline_stage_t* mon_stage = stage_event_monitor_create(&inv, &evt_3000, &evt_3005);
    pipeline_add_stage(evt_pipeline, mon_stage->name, mon_stage->process, mon_stage->priv_state,
                       mon_stage->cleanup_state, NULL, NULL);
    free(mon_stage);  // 管线复制 stage 字段，工厂对象只是临时描述符。
    pipeline_stage_t* log_stage =
        stage_event_logger_create(&inv, &evt_3000, &evt_3005, &hub, event_sink);
    pipeline_add_stage(evt_pipeline, log_stage->name, log_stage->process, log_stage->priv_state,
                       log_stage->cleanup_state, NULL, NULL);
    free(log_stage);

    uint64_t t0 = 1750000000000000000ULL;
    /* 两张表的电压影子：slot 0 是 A，slot 3 是 D。旧码把 OAD 低字节 0 当
     * device_id，会把 A 的值写进 D 的事件快照。 */
    static const uint32_t vtags[3] = {OAD_VOLTAGE, TAG_VOLTAGE_B, TAG_VOLTAGE_C};
    static const float volts_a[3] = {100.0f, 101.0f, 102.0f};
    static const float volts_d[3] = {150.0f, 151.0f, 152.0f};
    for (int i = 0; i < 3; i++) {
        pipe_msg_t v;
        memset(&v, 0, sizeof(v));
        v.tag_id = vtags[i];
        v.type = PIPEMSG_TYPE_SCALAR;
        v.device_id = 0;
        v.payload.val.f = volts_a[i];
        datahub_publish(&hub, &v);
        v.device_id = 3;
        v.payload.val.f = volts_d[i];
        datahub_publish(&hub, &v);
    }
    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.tag_id = OAD_VOLTAGE;
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.device_id = 3; /* D 的 slot；D 在下标 1，下标 3 是 E */
    msg.ts = t0;
    msg.payload.val.f = 150.0f;
    stream_engine_push(se, &msg);
    msg.ts = t0 + 1;
    stream_engine_push(se, &msg);
    for (int i = 0; i < 200 && evt_3000.record_count == 0; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(evt_3000.record_count, 1);

    /* D 的 A/B/C 相电压 150.0/151.0/152.0 → raw 1500/1510/1520；旧码从 slot 0
     * 的 A 表取到 1000/1010/1020。 */
    assert_int_equal(evt_3000.records[0].snapshot_len[0], 11);
    const uint8_t *snap = evt_3000.records[0].snapshot[0];
    assert_int_equal(snap[0], DCU_698_TAG_ARRAY);
    assert_int_equal(snap[1], 3);
    assert_int_equal(snap[2], DCU_698_TAG_LONG_UNSIGNED);
    assert_int_equal((snap[3] << 8) | snap[4], 1500);
    assert_int_equal(snap[5], DCU_698_TAG_LONG_UNSIGNED);
    assert_int_equal((snap[6] << 8) | snap[7], 1510);
    assert_int_equal(snap[8], DCU_698_TAG_LONG_UNSIGNED);
    assert_int_equal((snap[9] << 8) | snap[10], 1520);

    msg.ts = t0 + 2;
    msg.payload.val.f = 220.0f;
    stream_engine_push(se, &msg);
    for (int i = 0; i < 200 && evt_3000.records[0].is_active; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_false(evt_3000.records[0].is_active);

    uint64_t d_start = 0, d_end = 0, e_start = 0, e_end = 0;
    _event_row_times_for(&storage, "10000004", OI_VOLTAGE_DROP, &d_start, &d_end);
    _event_row_times_for(&storage, "10000005", OI_VOLTAGE_DROP, &e_start, &e_end);
    assert_int_not_equal(d_start, 0);
    assert_int_not_equal(d_end, 0);
    assert_int_equal(e_start, 0);
    assert_int_equal(e_end, 0);

    pipeline_t* frz_pipeline = stream_engine_add_pipeline(se, "TEST_SLOT_FRZ", 0);
    pipeline_stage_t* f_stage = stage_periodic_freeze_create_ex(&inv, &hub, event_sink);
    pipeline_add_stage(frz_pipeline, f_stage->name, f_stage->process, f_stage->priv_state,
                       f_stage->cleanup_state, NULL, NULL);
    free(f_stage);

    pipe_msg_t energy;
    memset(&energy, 0, sizeof(energy));
    energy.tag_id = OAD_ACTIVE_ENERGY;
    energy.type = PIPEMSG_TYPE_SCALAR;
    energy.device_id = 3;
    energy.payload.val.f = 12.5f;
    datahub_publish(&hub, &energy);

    pipe_msg_t trig;
    memset(&trig, 0, sizeof(trig));
    trig.tag_id = OAD_DAILY_FREEZE;
    trig.type = PIPEMSG_TYPE_SCALAR;
    trig.device_id = 3;
    trig.ts = t0 + 3;
    stream_engine_push(se, &trig);

    dcu_freeze_record_t rows[5];
    uint16_t row_count = 0;
    for (int i = 0; i < 200; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        row_count = 0;
        dcu_storage_query_freeze(&storage, "10000004", OAD_DAILY_FREEZE, 0, 0x7FFFFFFFFFFFFFFFULL,
                                 rows, 5, &row_count);
        if (row_count > 0) break;
        usleep(1000);
    }
    assert_int_equal(row_count, 3);
    bool found_energy = false;
    for (uint16_t i = 0; i < row_count; i++) {
        if (rows[i].source_oad == OAD_ACTIVE_ENERGY) {
            assert_float_equal(rows[i].frozen_value, 12.5f, 0.01f);
            found_energy = true;
        }
    }
    assert_true(found_energy);

    /* 下标 3 是 E：D 的冻结断面不得挂到 E 名下。 */
    row_count = 0;
    dcu_storage_query_freeze(&storage, "10000005", OAD_DAILY_FREEZE, 0, 0x7FFFFFFFFFFFFFFFULL, rows,
                             5, &row_count);
    assert_int_equal(row_count, 0);

    stream_engine_destroy(se);
    datahub_destroy(&hub);
    dcu_discovery_destroy(&discovery);
    dcu_router_destroy();
    dcu_storage_destroy(&storage);
    dcu_inventory_destroy(&inv);
    uv_walk(&loop, on_walk_close_slot_test, NULL);
    while (uv_loop_close(&loop) == UV_EBUSY) uv_run(&loop, UV_RUN_ONCE);
}

static void test_event_recovery_start_time_is_per_slot(void** state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    assert_int_equal(dcu_inventory_add(&inv, "10000001", "DLT698", 9600, 1), 0);
    assert_int_equal(dcu_inventory_add(&inv, "10000002", "DLT698", 9600, 1), 0);

    dcu_storage_t storage;
    assert_int_equal(dcu_storage_init(&storage, &loop, ":memory:"), 0);

    datahub_t hub;
    datahub_init(&hub, &loop, "dcu_test_event_slot_start", 1024, 1024, NULL, 0, NULL, NULL, NULL);
    hub.map_fn_v2 = dcu_tag_map_fn;

    dcu_poller_t poller;
    dcu_poller_init(&poller);
    dcu_discovery_t discovery;
    dcu_discovery_init(&discovery);

    dcu_event_obj_t evt_3000;
    memset(&evt_3000, 0, sizeof(evt_3000));
    evt_3000.oi = OI_VOLTAGE_DROP;
    evt_3000.threshold = 176.0f;
    evt_3000.delay_s = 0;
    evt_3000.is_enabled = true;
    dcu_event_obj_t evt_3005;
    memset(&evt_3005, 0, sizeof(evt_3005));
    evt_3005.oi = OI_OVER_CURRENT;
    evt_3005.threshold = 5.0f;
    evt_3005.delay_s = 0;
    evt_3005.is_enabled = true;

    router_sink_ops_t router_sink = {.ctx = &storage,
                                     .record_telemetry = (void*)dcu_storage_write_telemetry,
                                     .count_freezes = _mock_count_freezes};
    router_bus_ops_t router_bus = {.ctx = &hub,
                                   .msg_size = sizeof(pipe_msg_t),
                                   .publish_realtime = _mock_publish_realtime,
                                   .read_shadow = _mock_read_shadow};
    assert_int_equal(dcu_router_init_ex(&inv, &poller, router_sink, NULL, &discovery, &evt_3000,
                                        &evt_3005, router_bus),
                     0);

    event_sink_ops_t event_sink = {.ctx = &storage,
                                   .archive_event = _test_archive_event,
                                   .archive_freeze = _test_archive_freeze};

    stream_engine_t* se = stream_engine_create_datahub(&hub, &loop, NULL, NULL);
    pipeline_t* evt_pipeline = stream_engine_add_pipeline(se, "TEST_EVT_START_SLOT", 0);
    pipeline_stage_t* mon_stage = stage_event_monitor_create(&inv, &evt_3000, &evt_3005);
    pipeline_add_stage(evt_pipeline, mon_stage->name, mon_stage->process, mon_stage->priv_state,
                       mon_stage->cleanup_state, NULL, NULL);
    free(mon_stage);
    pipeline_stage_t* log_stage =
        stage_event_logger_create(&inv, &evt_3000, &evt_3005, &hub, event_sink);
    pipeline_add_stage(evt_pipeline, log_stage->name, log_stage->process, log_stage->priv_state,
                       log_stage->cleanup_state, NULL, NULL);
    free(log_stage);

    const uint64_t sec = 1000000000ULL;
    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.tag_id = OAD_VOLTAGE;
    msg.type = PIPEMSG_TYPE_SCALAR;

    msg.device_id = 0;
    msg.ts = 1000 * sec;
    msg.payload.val.f = 150.0f;
    stream_engine_push(se, &msg);
    msg.ts = 1001 * sec;
    stream_engine_push(se, &msg);
    for (int i = 0; i < 500 && evt_3000.record_count < 1; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(evt_3000.record_count, 1);
    assert_int_equal(evt_3000.records[0].start_time.mono_ns, 1001 * sec);

    msg.device_id = 1;
    msg.ts = 2000 * sec;
    msg.payload.val.f = 150.0f;
    stream_engine_push(se, &msg);
    msg.ts = 2001 * sec;
    stream_engine_push(se, &msg);
    for (int i = 0; i < 500 && evt_3000.record_count < 2; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(evt_3000.record_count, 2);
    assert_int_equal(evt_3000.last_start_time.mono_ns, 2001 * sec);

    msg.device_id = 0;
    msg.ts = 3000 * sec;
    msg.payload.val.f = 220.0f;
    stream_engine_push(se, &msg);
    for (int i = 0; i < 500 && evt_3000.record_count < 3; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(evt_3000.record_count, 3);
    assert_false(evt_3000.records[0].is_active);
    assert_int_equal(evt_3000.records[0].end_time.mono_ns, 3000 * sec);
    assert_int_equal(evt_3000.records[0].start_time.mono_ns, 1001 * sec);
    assert_int_equal(evt_3000.last_start_time.mono_ns, 2001 * sec);

    msg.device_id = 0;
    msg.ts = 5000 * sec;
    msg.payload.val.f = 150.0f;
    stream_engine_push(se, &msg);
    msg.ts = 5001 * sec;
    stream_engine_push(se, &msg);
    for (int i = 0; i < 500 && evt_3000.record_count < 4; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(evt_3000.record_count, 4);

    msg.device_id = 1;
    msg.ts = 6000 * sec;
    msg.payload.val.f = 220.0f;
    stream_engine_push(se, &msg);
    for (int i = 0; i < 500 && evt_3000.record_count < 5; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(evt_3000.record_count, 5);
    assert_false(evt_3000.records[0].is_active);
    assert_int_equal(evt_3000.records[0].end_time.mono_ns, 6000 * sec);
    assert_int_equal(evt_3000.records[0].start_time.mono_ns, 2001 * sec);

    msg.device_id = 0;
    msg.ts = 7000 * sec;
    msg.payload.val.f = 220.0f;
    stream_engine_push(se, &msg);
    for (int i = 0; i < 500 && evt_3000.record_count < 6; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(evt_3000.record_count, 6);
    assert_false(evt_3000.records[0].is_active);
    assert_int_equal(evt_3000.records[0].end_time.mono_ns, 7000 * sec);
    assert_int_equal(evt_3000.records[0].start_time.mono_ns, 5001 * sec);
    assert_int_equal(evt_3000.last_start_time.mono_ns, 5001 * sec);
    assert_int_equal(evt_3000.last_end_time.mono_ns, 7000 * sec);

    msg.device_id = 0;
    msg.ts = 8000 * sec;
    msg.payload.val.f = 150.0f;
    stream_engine_push(se, &msg);
    msg.ts = 8001 * sec;
    stream_engine_push(se, &msg);
    for (int i = 0; i < 500 && evt_3000.record_count < 7; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(evt_3000.record_count, 7);

    uint64_t t = 9000;
    for (int c = 0; c < 5; c++) {
        msg.device_id = 1;
        msg.ts = t * sec;
        msg.payload.val.f = 150.0f;
        stream_engine_push(se, &msg);
        msg.ts = (t + 1) * sec;
        stream_engine_push(se, &msg);
        msg.ts = (t + 2) * sec;
        msg.payload.val.f = 220.0f;
        stream_engine_push(se, &msg);
        t += 10;
    }
    for (int i = 0; i < 1000 && evt_3000.records[0].end_time.mono_ns != 9042 * sec; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(evt_3000.records[0].end_time.mono_ns, 9042 * sec);

    msg.device_id = 0;
    msg.ts = 20000 * sec;
    msg.payload.val.f = 220.0f;
    stream_engine_push(se, &msg);
    for (int i = 0; i < 500 && evt_3000.last_end_time.mono_ns != 20000 * sec; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    assert_int_equal(evt_3000.record_count, 10);
    assert_int_equal(evt_3000.records[0].end_time.mono_ns, 9042 * sec);
    assert_int_equal(evt_3000.last_start_time.mono_ns, 9041 * sec);
    assert_int_equal(evt_3000.last_end_time.mono_ns, 20000 * sec);

    stream_engine_destroy(se);
    datahub_destroy(&hub);
    dcu_discovery_destroy(&discovery);
    dcu_router_destroy();
    dcu_storage_destroy(&storage);
    dcu_inventory_destroy(&inv);
    uv_walk(&loop, on_walk_close_slot_test, NULL);
    while (uv_loop_close(&loop) == UV_EBUSY) uv_run(&loop, UV_RUN_ONCE);
}

typedef struct {
    int calls;
    char addr[32];
    uint16_t oi;
    uint64_t start_epoch;
    uint64_t end_epoch;
    float value;
} captured_archive_t;

static int _capture_archive_event(void* ctx, const char* addr, uint16_t event_code,
                                  uint64_t start_time, uint64_t end_time, float snapshot_val) {
    captured_archive_t* cap = (captured_archive_t*)ctx;
    cap->calls++;
    snprintf(cap->addr, sizeof(cap->addr), "%s", addr);
    cap->oi = event_code;
    cap->start_epoch = start_time;
    cap->end_epoch = end_time;
    cap->value = snapshot_val;
    return 0;
}

/* 落盘起点必须来自本 slot 的发生记录：slot0 恢复不得写成 slot1 更晚的发生时间。 */
static void test_event_archive_start_is_per_slot(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    assert_int_equal(dcu_inventory_add(&inv, "10000001", "DLT698", 9600, 1), 0);
    assert_int_equal(dcu_inventory_add(&inv, "10000002", "DLT698", 9600, 1), 0);

    dcu_event_obj_t evt_3000;
    memset(&evt_3000, 0, sizeof(evt_3000));
    evt_3000.oi = OI_VOLTAGE_DROP;
    evt_3000.is_enabled = true;
    dcu_event_obj_t evt_3005;
    memset(&evt_3005, 0, sizeof(evt_3005));

    /* 两只表同 OI 的发生记录：slot0 更早（1700000000），slot1 更晚（1700000001）。 */
    evt_3000.records[1].index = 1;
    evt_3000.records[1].slot_id = 0;
    evt_3000.records[1].is_active = true;
    evt_3000.records[1].start_time.mono_ns = 1001ULL * 1000000000ULL;
    evt_3000.records[1].start_time.epoch_s = 1700000000ULL;
    evt_3000.records[0].index = 2;
    evt_3000.records[0].slot_id = 1;
    evt_3000.records[0].is_active = true;
    evt_3000.records[0].start_time.mono_ns = 2001ULL * 1000000000ULL;
    evt_3000.records[0].start_time.epoch_s = 1700000001ULL;
    evt_3000.record_count = 2;
    evt_3000.last_start_time = evt_3000.records[0].start_time; /* A14 最近发生 = slot1 */

    captured_archive_t cap;
    memset(&cap, 0, sizeof(cap));
    event_sink_ops_t sink = {.ctx = &cap, .archive_event = _capture_archive_event};
    pipeline_stage_t* log_stage =
        stage_event_logger_create(&inv, &evt_3000, &evt_3005, NULL, sink);
    assert_non_null(log_stage);

    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = PIPEMSG_TYPE_EVENT;
    msg.tag_id = OI_VOLTAGE_DROP;
    msg.device_id = 0;
    msg.ts = 3000ULL * 1000000000ULL;
    msg.payload.val.f = -220.0f;
    assert_int_equal(log_stage->process(log_stage, &msg), STAGE_DROP);

    assert_int_equal(cap.calls, 1);
    assert_int_equal(cap.start_epoch, 1700000000ULL);
    assert_int_not_equal(cap.start_epoch, 1700000001ULL);
    assert_int_equal(cap.oi, OI_VOLTAGE_DROP);
    assert_string_equal(cap.addr, "10000001");
    assert_int_equal(evt_3000.records[0].start_time.epoch_s, 1700000000ULL);
    assert_int_equal(evt_3000.last_start_time.epoch_s, 1700000001ULL);

    log_stage->cleanup_state(log_stage->priv_state);
    free(log_stage);
    dcu_inventory_destroy(&inv);
}

/* 发生记录已被挤出窗口时，恢复不落盘、不追加记录行。 */
static void test_event_recovery_evicted_occurrence_does_not_archive(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    assert_int_equal(dcu_inventory_add(&inv, "10000001", "DLT698", 9600, 1), 0);

    dcu_event_obj_t evt_3000;
    memset(&evt_3000, 0, sizeof(evt_3000));
    evt_3000.oi = OI_VOLTAGE_DROP;
    evt_3000.is_enabled = true;
    dcu_event_obj_t evt_3005;
    memset(&evt_3005, 0, sizeof(evt_3005));

    for (int i = 0; i < MAX_EVENT_RECORDS; i++) {
        evt_3000.records[i].index = (uint16_t)(i + 1);
        evt_3000.records[i].slot_id = 1;
        evt_3000.records[i].is_active = true;
        evt_3000.records[i].start_time.mono_ns = (uint64_t)(2001 + i) * 1000000000;
        evt_3000.records[i].start_time.epoch_s = 1700000001ULL;
    }
    evt_3000.record_count = MAX_EVENT_RECORDS;
    evt_3000.last_start_time = evt_3000.records[0].start_time;

    captured_archive_t cap;
    memset(&cap, 0, sizeof(cap));
    event_sink_ops_t sink = {.ctx = &cap, .archive_event = _capture_archive_event};
    pipeline_stage_t* log_stage =
        stage_event_logger_create(&inv, &evt_3000, &evt_3005, NULL, sink);
    assert_non_null(log_stage);

    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = PIPEMSG_TYPE_EVENT;
    msg.tag_id = OI_VOLTAGE_DROP;
    msg.device_id = 0;
    msg.ts = 3000ULL * 1000000000ULL;
    msg.payload.val.f = -220.0f;
    assert_int_equal(log_stage->process(log_stage, &msg), STAGE_DROP);

    assert_int_equal(cap.calls, 0);
    assert_int_equal(evt_3000.record_count, MAX_EVENT_RECORDS);
    assert_int_equal(evt_3000.records[0].slot_id, 1);
    assert_int_equal(evt_3000.records[0].start_time.epoch_s, 1700000001ULL);
    assert_true(evt_3000.records[0].is_active);
    assert_true(evt_3000.last_end_time.mono_ns == msg.ts);

    log_stage->cleanup_state(log_stage->priv_state);
    free(log_stage);
    dcu_inventory_destroy(&inv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_advanced_features_dispatch),
        cmocka_unit_test(test_event_and_freeze_keep_slot_identity_after_archive_shift),
        cmocka_unit_test(test_event_recovery_start_time_is_per_slot),
        cmocka_unit_test(test_event_archive_start_is_per_slot),
        cmocka_unit_test(test_event_recovery_evicted_occurrence_does_not_archive),
    };
    return cmocka_run_group_tests(tests, setup_timezone, teardown_timezone);
}
