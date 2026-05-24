#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/un.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contracts.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "edge_params.h"
#include "edge_service.h"
#include "northbound_mgr.h"
#include "sqlite_service.h"
#include "stream_engine.h"

/* --- 引入生产级 DLT 698.45 核心自治组件 --- */
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_storage.h"
#include "dcu_router.h"
#include "dcu_security.h"
#include "dcu_discovery.h"
#include "dcu_event_monitor.h"

// 全局运行时句柄，便于停电保护及 SIGTERM 销毁
static service_t *g_running_svc = NULL;
static dcu_storage_t g_dcu_storage;
static dcu_poller_t g_dcu_poller;
static dcu_inventory_t g_dcu_inventory;

// 集中器高级模块业务单例实体
static dcu_esam_t g_dcu_esam;
static dcu_discovery_t g_dcu_discovery;
static dcu_event_obj_t g_dcu_evt_3000; // 失压监控对象
static dcu_event_obj_t g_dcu_evt_3005; // 过流监控对象

static uv_timer_t g_sync_timer;        // 遥测同步与流式管线注入定时器
static uv_timer_t g_discovery_timer;   // 后台搜表扫描定时器
static uv_timer_t g_freeze_trigger_timer; // 定期日冻结模拟触发定时器

typedef struct {
    datahub_t *hub;
    stream_engine_t *engine;
} sync_ctx_t;

static sync_ctx_t g_sync_ctx;

/**
 * @brief 【掉电保护核心回调】接收到 SIGTERM 物理停电中断信号时，强制将内存缓冲原子刷写存盘
 */
static void on_sigterm(int sig) {
    (void)sig;
    printf("\n⚠️  [WARN] SIGTERM/停电中断触发！启动紧急掉电原子保护，强制执行缓存落盘...\n");
    
    // 强行刷盘，通过批量 TRANSACTION 原子存盘，确保计量数据和最后遥测断面 100% 不损坏
    dcu_storage_flush(&g_dcu_storage);
    
    // 停止轮询生产器
    dcu_poller_stop(&g_dcu_poller);
    
    if (g_running_svc) {
        service_stop(g_running_svc);
    }
}

/**
 * @brief 周期性从 DataHub 影子总线提取电压电流，注入到流引擎管线中执行流式越限判定
 */
static void _sync_datahub_to_stream_cb(uv_timer_t *timer) {
    sync_ctx_t *ctx = (sync_ctx_t *)timer->data;
    if (!ctx || !ctx->hub || !ctx->engine) return;
    
    uint64_t now_s = (uint64_t)time(NULL);
    
    // 1. 读取 A 相电压影子值并注入事件监控管线
    pipe_msg_t shadow_msg;
    if (datahub_read_shadow(ctx->hub, OAD_VOLTAGE, &shadow_msg) == 0) {
        pipe_msg_t inject_msg;
        memset(&inject_msg, 0, sizeof(pipe_msg_t));
        inject_msg.tag_id = OAD_VOLTAGE;
        inject_msg.type = PIPEMSG_TYPE_SCALAR;
        inject_msg.ts = now_s;
        inject_msg.payload.val.f = shadow_msg.payload.val.f;
        
        stream_engine_push(ctx->engine, &inject_msg);
    }
    
    // 2. 读取 A 相电流影子值并注入
    if (datahub_read_shadow(ctx->hub, OAD_CURRENT, &shadow_msg) == 0) {
        pipe_msg_t inject_msg;
        memset(&inject_msg, 0, sizeof(pipe_msg_t));
        inject_msg.tag_id = OAD_CURRENT;
        inject_msg.type = PIPEMSG_TYPE_SCALAR;
        inject_msg.ts = now_s;
        inject_msg.payload.val.f = shadow_msg.payload.val.f;
        
        stream_engine_push(ctx->engine, &inject_msg);
    }
}

/**
 * @brief 后台模拟物理扫表定时心跳，驱动搜表状态机流转
 */
static void _discovery_tick_cb(uv_timer_t *timer) {
    (void)timer;
    dcu_discovery_tick_simulate(&g_dcu_discovery);
}

/**
 * @brief 周期性模拟触发“日定期冻结管线”
 */
static void _freeze_trigger_cb(uv_timer_t *timer) {
    sync_ctx_t *ctx = (sync_ctx_t *)timer->data;
    if (!ctx || !ctx->engine) return;
    
    // 构造日冻结时钟触发信号 (用 OAD_DAILY_FREEZE 代表定时冻结事件触发，完全消灭 0x9999 魔数)
    pipe_msg_t trigger_msg;
    memset(&trigger_msg, 0, sizeof(pipe_msg_t));
    trigger_msg.tag_id = OAD_DAILY_FREEZE;
    trigger_msg.type = PIPEMSG_TYPE_SCALAR;
    trigger_msg.ts = (uint64_t)time(NULL);
    trigger_msg.payload.val.f = 1.0f;
    
    stream_engine_push(ctx->engine, &trigger_msg);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("🚀 Starting Production-Grade DLT 698.45 DCU Service Engine...\n");
    
    uv_loop_t *loop = uv_default_loop();
    edge_log_t *logger = edge_log_create(NULL, true);

    // 1. 初始化 SQLite 存储服务与双缓冲批量事务模块
    int ret = dcu_storage_init(&g_dcu_storage, loop, "dcu_production.db");
    if (ret != 0) {
        printf("[FATAL] SQLite storage initialization failed!\n");
        return -1;
    }

    // 2. 初始化电能表注册档案 (在此处移除，稍后在契约加载后执行)

    // 3. 初始化定时任务与采集方案调度引擎
    dcu_poller_init(&g_dcu_poller);
    
    // 注册普通采集方案 1：抄读电压 (OAD_VOLTAGE) 与电流 (OAD_CURRENT)
    uint32_t oads[2] = {OAD_VOLTAGE, OAD_CURRENT};
    dcu_poller_add_scheme(&g_dcu_poller, 1, 0, oads, 2);
    
    // 注册定时任务 1：执行频率 TI = 5 秒，绑定方案 1
    dcu_poller_add_task(&g_dcu_poller, 1, 5, 1);

    // 4. 初始化集中器特有高级模块实体与 Class 7 事件契约参数
    dcu_security_init(&g_dcu_esam);
    dcu_discovery_init(&g_dcu_discovery);
    
    // 初始化失压事件对象 (0x3000)：判定门限 176.0V，触发延时 5 秒，开启使能和主动上报
    g_dcu_evt_3000.oi = OI_VOLTAGE_DROP;
    g_dcu_evt_3000.record_count = 0;
    memset(g_dcu_evt_3000.records, 0, sizeof(g_dcu_evt_3000.records));
    g_dcu_evt_3000.snapshot_oad = OAD_VOLTAGE;
    g_dcu_evt_3000.threshold = 176.0f;
    g_dcu_evt_3000.delay_s = 5;
    g_dcu_evt_3000.report_flag = 1; // 开启主动上报
    g_dcu_evt_3000.is_enabled = true;
    g_dcu_evt_3000.last_start_time = 0;
    g_dcu_evt_3000.last_end_time = 0;
    
    // 初始化过流事件对象 (0x3005)：判定门限 5.0A，触发延时 3 秒，开启使能与主动上报
    g_dcu_evt_3005.oi = OI_OVER_CURRENT;
    g_dcu_evt_3005.record_count = 0;
    memset(g_dcu_evt_3005.records, 0, sizeof(g_dcu_evt_3005.records));
    g_dcu_evt_3005.snapshot_oad = OAD_CURRENT;
    g_dcu_evt_3005.threshold = 5.0f;
    g_dcu_evt_3005.delay_s = 3;
    g_dcu_evt_3005.report_flag = 1;
    g_dcu_evt_3005.is_enabled = true;
    g_dcu_evt_3005.last_start_time = 0;
    g_dcu_evt_3005.last_end_time = 0;

    // 5. 对外契约寻址窗口初始化与高优动作路由绑定
    dcu_router_init_ex(&g_dcu_inventory, &g_dcu_poller, &g_dcu_storage, 
                      &g_dcu_esam, &g_dcu_discovery, &g_dcu_evt_3000, &g_dcu_evt_3005);

    // 6. 初始化系统底层 DataHub 总线底座
    datahub_t hub;
    datahub_init(&hub, loop, "dcu_production_bus", 1024, g_tag_metadata_len, g_tag_metadata, g_tag_metadata_len,
                 NULL, edge_tag_get_compact_index, edge_router_needs_wakeup);

    // 【重要】先加载服务合约以保证 edge_model_find 可用
    extern const edge_contract_t g_contract_manifest;
    contract_register(&g_contract_manifest);

    // 7. 挂接南向总线编排器与驱动管理器
    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);
    driver_manager_t *drv_mgr = driver_manager_create();
    
    // [MATRIX] 注册生产级协议栈驱动
    extern const edge_driver_t modbus_rtu_driver;
    extern const edge_driver_t dlt645_driver;
    extern const edge_driver_t dlt698_driver;
    driver_manager_register(drv_mgr, &modbus_rtu_driver);
    driver_manager_register(drv_mgr, &dlt645_driver);
    driver_manager_register(drv_mgr, &dlt698_driver);

    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter, drv_mgr, 8, 32, 2048);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);
    
    // [MATRIX] 开启仲裁调度
    arbiter_resume(arbiter);

    // [MATRIX] 绑定物理串口设备 (使用 UDS 模拟物理设备进行跨进程测试)
    const char *uds_path = "/tmp/rs485_1.sock";
    int uds_fd = -1;
    struct sockaddr_un uds_addr;
    if ((uds_fd = socket(AF_UNIX, SOCK_STREAM, 0)) >= 0) {
        memset(&uds_addr, 0, sizeof(uds_addr));
        uds_addr.sun_family = AF_UNIX;
        strncpy(uds_addr.sun_path, uds_path, sizeof(uds_addr.sun_path) - 1);
        if (connect(uds_fd, (struct sockaddr *)&uds_addr, sizeof(uds_addr)) < 0) {
            printf("⚠️  [MATRIX] Failed to connect to simulator at %s, using STDOUT placeholder.\n", uds_path);
            close(uds_fd);
            uds_fd = 1; // Fallback to stdout
        } else {
            printf("🔗 [MATRIX] Connected to physical simulator at %s\n", uds_path);
        }
    }
    
    io_handle_t *rs485_1 = io_executor_create_from_fd(loop, uds_fd, hub.io_pool);
    bus_orchestrator_attach_io(io_orch, 1, rs485_1);
    
    // 从生成的契约获取注入字典
    const driver_contract_injection_t *inj_modbus = edge_model_find("MODBUS");
    const driver_contract_injection_t *inj_dlt645 = edge_model_find("DLT645");
    const driver_contract_injection_t *inj_dlt698 = edge_model_find("Advanced_DCU_Interface");
    if (!inj_dlt698) inj_dlt698 = edge_model_find("DLT698");
    if (!inj_dlt698) {
        printf("⚠️  [MATRIX] DLT698 model injection not found, falling back to DLT645 map for OAD lookup.\n");
        inj_dlt698 = inj_dlt645;
    }

    if (inj_modbus) bus_orchestrator_setup_master(io_orch, 1, "ModbusRTU", 1, inj_modbus);
    if (inj_dlt645) bus_orchestrator_setup_master(io_orch, 1, "DLT645-2007", 1, inj_dlt645);
    if (inj_dlt698) bus_orchestrator_setup_master(io_orch, 1, "DLT698", 1, inj_dlt698);

    // 2. 初始化电能表注册档案
    dcu_inventory_init(&g_dcu_inventory);
    
    // 自动载入首批工业子表注册信息
    dcu_inventory_add(&g_dcu_inventory, "10000001", "DLT645-2007", 9600, 1, inj_dlt645);
    dcu_inventory_add(&g_dcu_inventory, "10000002", "ModbusRTU", 9600, 1, inj_modbus);
    dcu_inventory_add(&g_dcu_inventory, "10000003", "DLT698", 9600, 1, inj_dlt698);

    // 8. 挂接北向前置通信与参数配置管理层
    sqlite_config_t db_conf = {.db_path = "dcu_runtime.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *stream_engine =
        stream_engine_create(&hub, NULL, NULL, edge_stage_factory_get_registry());
    edge_params_t *param_mgr = edge_params_create(db_service, &hub);

    // 9. [核心微内核重构]：用 stream_engine 动态编排“失压与过流事件监控管线”和“历史定期冻结管线”
    // 🚀 管线 A: 事件越限监控与快照追加管线
    pipeline_t *evt_pipeline = stream_engine_add_pipeline(stream_engine, "EVENT_PIPELINE", 0);
    
    pipeline_stage_t *mon_stage = stage_event_monitor_create(&g_dcu_evt_3000, &g_dcu_evt_3005);
    pipeline_add_stage(evt_pipeline, mon_stage->name, mon_stage->process, mon_stage->priv_state, mon_stage->cleanup_state, NULL, NULL);
    free(mon_stage);
    
    pipeline_stage_t *log_stage = stage_event_logger_create(&g_dcu_evt_3000, &g_dcu_evt_3005, &hub);
    pipeline_add_stage(evt_pipeline, log_stage->name, log_stage->process, log_stage->priv_state, log_stage->cleanup_state, NULL, NULL);
    free(log_stage);
    
    // 💾 管线 B: 定期日冻结落盘管线
    pipeline_t *freeze_pipeline = stream_engine_add_pipeline(stream_engine, "FREEZE_PIPELINE", 0);
    pipeline_stage_t *freeze_stage = stage_periodic_freeze_create_ex(&g_dcu_storage, &hub);
    pipeline_add_stage(freeze_pipeline, freeze_stage->name, freeze_stage->process, freeze_stage->priv_state, freeze_stage->cleanup_state, NULL, NULL);
    free(freeze_stage);

    // 10. 北向接口参数初始化
    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub.ring, "dcu", db_service);

    service_t *svc =
        service_new(loop, &hub, arbiter, db_service, stream_engine, param_mgr, north_mgr);
    g_running_svc = svc;
    north_mgr->user_data = svc;

    // 11. 【安全第一】注册 SIGTERM 掉电告警中断信号
    signal(SIGTERM, on_sigterm);
    signal(SIGINT, on_sigterm); // 支持 Ctrl+C 干净退出测试

    // 12. 启动物理数采并时间对齐 (MATRIX 真实调度引擎)
    dcu_poller_start(&g_dcu_poller, &g_dcu_inventory, io_orch, drv_mgr, loop);
    printf("📊 [DCU] Production poller started (698.45 OI-driven, Matrix Engine)\n");

    // 13. [核心注入桥接] 启动对齐遥测同步与冻结触发模拟定时器 (1秒执行一次同步，5秒触发一次日冻结快照)
    g_sync_ctx.hub = &hub;
    g_sync_ctx.engine = stream_engine;
    
    uv_timer_init(loop, &g_sync_timer);
    g_sync_timer.data = &g_sync_ctx;
    uv_timer_start(&g_sync_timer, _sync_datahub_to_stream_cb, 1000, 1000);
    
    uv_timer_init(loop, &g_discovery_timer);
    uv_timer_start(&g_discovery_timer, _discovery_tick_cb, 2000, 2000);
    
    uv_timer_init(loop, &g_freeze_trigger_timer);
    g_freeze_trigger_timer.data = &g_sync_ctx;
    uv_timer_start(&g_freeze_trigger_timer, _freeze_trigger_cb, 5000, 5000);

    // 14. 开启总线并发编排并进入 libuv 主事件循环
    bus_orchestrator_run(io_orch);
    int exit_code = service_run(svc);

    // 15. 退出资源干净清理与回收
    printf("🧹 [DCU] Main loop exited, releasing all allocated resources...\n");
    uv_timer_stop(&g_sync_timer);
    uv_timer_stop(&g_discovery_timer);
    uv_timer_stop(&g_freeze_trigger_timer);
    
    dcu_poller_stop(&g_dcu_poller);
    dcu_poller_destroy(&g_dcu_poller);
    dcu_inventory_destroy(&g_dcu_inventory);
    dcu_storage_destroy(&g_dcu_storage);
    dcu_discovery_destroy(&g_dcu_discovery);
    dcu_router_destroy();

    service_delete(svc);
    northbound_mgr_stop(north_mgr);
    free(north_mgr);
    bus_orchestrator_destroy(io_orch);
    stream_engine_destroy(stream_engine);
    driver_manager_destroy(drv_mgr);
    arbiter_destroy(arbiter);
    edge_params_destroy(param_mgr);
    sqlite_svc_destroy(db_service);
    datahub_destroy(&hub);
    edge_log_destroy(logger);
    
    printf("👋 [DCU] DCU engine shutdown gracefully.\n");
    return exit_code;
}