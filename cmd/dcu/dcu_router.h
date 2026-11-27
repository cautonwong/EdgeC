#ifndef DCU_ROUTER_H
#define DCU_ROUTER_H

#include <stdbool.h>
#include <stdint.h>

#include "dcu_discovery.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_security.h"

#define DCU_SLOTS_PER_DEVICE 32

/* 应用点位区：tag_id 落在 [DCU_APP_BASE, DCU_APP_BASE + DCU_APP_SLOT_TOTAL)
 * 时不走 device_id * SLOTS + slot 公式，map_fn 返回 -1，DataHub 以 idx = tag_id
 * 承载。产品 tag 可以大于 DCU_APP_BASE（OAD 形态），但必须避开这 4096 个值。
 * 槽位总数必须与 app_points_sqlite.h 的 APP_POINT_SLOT_TOTAL 一致。 */
#define DCU_APP_BASE (MAX_METERS * DCU_SLOTS_PER_DEVICE)
#define DCU_APP_SLOT_TOTAL 4096
#define DCU_SHADOW_CAPACITY (DCU_APP_BASE + DCU_APP_SLOT_TOTAL)

#define IC_ENERGY 1         // 电能量接口类 (0010, 0020)
#define IC_MAX_DEMAND 2     // 最大需量接口类 (1010)
#define IC_VARIABLE 3       // 分相变量接口类 (如电压、电流、相角)
#define IC_POWER 4          // 功率接口类 (如 2004 有功功率)
#define IC_HARMONIC 5       // 谐波接口类 (200D, 200E)
#define IC_DATA_VARIABLE 6  // 数据变量接口类 (如 200F 频率)
#define IC_EVENT 7          // 事件对象接口类
#define IC_PARAM 8          // 参数变量接口类（如 6001 采集档案配置单元）
#define IC_RECORD 9         // 记录/冻结数据接口类 (如日冻结 5004)
#define IC_ACQUISITION 10   // 采集监控接口类 (如 6000 档案, 6012 任务)
#define IC_COLLECTION 11    // 集合接口类 (如 6014 普通采集方案集, 6002 搜表)
#define IC_PULSE 12         // 脉冲计量接口类 (如 2401 脉冲计量)
#define IC_CONTROL 13       // 控制/继电器动作类 (如 8000 远程拉合闸)
#define IC_INTERVAL_STAT 14 // 区间统计接口类
#define IC_AVERAGE 15       // 累加平均接口类
#define IC_EXTREMUM 16      // 极值工具接口类
#define IC_DISPLAY 17       // 显示接口类 (F300 自动轮显, F301 按键轮显)
#define IC_FILE_TRANSFER 18 // 文件传输接口类 (F000 分帧, F001 分块, F002 扩展)
#define IC_DEVICE_MGMT 19   // 设备管理接口类 (4300 电气设备, 4307..4309 水/气/热表)
#define IC_ESAM 21          // ESAM 接口类 (F100)
#define IC_PERIPHERAL 22    // 输入输出设备接口类 (F200 串口)
#define IC_GROUP 23         // 总加组接口类 (2301..2308)
#define IC_SUB_EVENT 24     // 分项事件对象接口类 (3000 失压, 3005 过流)
#define IC_WIRELESS_PUBLIC 25 // 无线公网通信接口类 (4500/4501)
#define IC_ETHERNET 26        // 以太网通信接口类 (4510..4517)

#define OAD_ACTIVE_ENERGY 0x00100200         // 0010 属性 2（总及费率电能数组）
#define OAD_REACTIVE_ENERGY 0x00200200       // 0020 属性 2（总及费率电能数组）
#define OAD_VOLTAGE 0x20000200               // 2000 属性 2（A/B/C 电压数组）
#define TAG_VOLTAGE_B 0x20000300             // B 相影子 tag（数组元素，非 698 OAD）
#define TAG_VOLTAGE_C 0x20000400             // C 相影子 tag（数组元素，非 698 OAD）
#define OAD_CURRENT 0x20010200               // 2001 属性 2（A/B/C 电流数组）
#define TAG_CURRENT_B 0x20010300             // B 相影子 tag（数组元素，非 698 OAD）
#define TAG_CURRENT_C 0x20010400             // C 相影子 tag（数组元素，非 698 OAD）
#define OAD_ACTIVE_POWER_TOTAL 0x20040200    // 2004 属性 2（总/A/B/C 功率数组）
#define TAG_ACTIVE_POWER_A 0x20040300        // A 相影子 tag（数组元素，非 698 OAD）
#define TAG_ACTIVE_POWER_B 0x20040400        // B 相影子 tag（数组元素，非 698 OAD）
#define TAG_ACTIVE_POWER_C 0x20040500        // C 相影子 tag（数组元素，非 698 OAD）
#define OAD_REACTIVE_POWER_TOTAL 0x20050200  // 总无功功率 (OI 2005 属性 2)
#define OAD_POWER_FACTOR_TOTAL 0x200A0200    // 200A 属性 2（功率因数数组）
#define OAD_GRID_FREQUENCY 0x200F0200        // 200F 属性 2（Class 6 单值）

/* 南向读数组元素：索引字节是属性内元素，不是北向的表槽位（ADR-0016）。 */
#define OAD_VOLTAGE_B_ELEM 0x20000201
#define OAD_VOLTAGE_C_ELEM 0x20000202
#define OAD_CURRENT_B_ELEM 0x20010201
#define OAD_CURRENT_C_ELEM 0x20010202
#define OAD_ACTIVE_POWER_A_ELEM 0x20040201
#define OAD_ACTIVE_POWER_B_ELEM 0x20040202
#define OAD_ACTIVE_POWER_C_ELEM 0x20040203

#define OI_VOLTAGE_DROP 0x3000  // 失压事件 OI
#define OI_OVER_CURRENT 0x3005  // 过流事件 OI

#define OAD_DAILY_FREEZE \
    0x50040200  // 日冻结记录表属性 2 (日冻结 OAD 既作为定时流触发 Tag，也是存储召测 Target)

#define OI_METER_ARCHIVE 0x6000     // 采集档案配置表 OI
#define OI_ARCHIVE_UNIT 0x6001      // 采集档案配置单元 OI（标准 A.7，IC 8）
#define OI_TASK_SCHEDULER 0x6012    // 任务配置表 OI
#define OI_TASK_UNIT 0x6013         // 任务配置单元 OI（标准 A.7，IC 8）
#define OI_NORMAL_SCHEME 0x6014     // 普通采集方案集 OI（标准 A.7，IC 11）
#define OI_NORMAL_SCHEME_UNIT 0x6015 // 普通采集方案 OI（标准 A.7，IC 8）
#define OI_EVENT_SCHEME 0x6016      // 事件采集方案集 OI（标准 A.7，IC 11）
#define OI_EVENT_SCHEME_UNIT 0x6017 // 事件采集方案 OI（标准 A.7，IC 8）
#define OI_TRANSPARENT_SCHEME 0x6018 // 透明方案集 OI（标准 A.7，IC 11）
#define OI_TRANSPARENT_SCHEME_UNIT 0x6019 // 透明方案 OI（标准 A.7，IC 8）
#define OI_TRANSPARENT_RESULT 0x601A      // 透明方案结果集 OI（标准 A.7，IC 11）
#define OI_TRANSPARENT_RESULT_UNIT 0x601B // 一个透明方案结果 OI（标准 A.7，IC 8）
#define OI_REPORT_SCHEME 0x601C           // 上报方案集 OI（标准 A.7，IC 11）
#define OI_REPORT_SCHEME_UNIT 0x601D      // 上报方案 OI（标准 A.7，IC 8）
#define OI_RULE_LIBRARY 0x601E            // 采集规则库 OI（标准 A.7，IC 11）
#define OI_RULE 0x601F                    // 采集规则 OI（标准 A.7，IC 8）
#define OI_ACQ_STATUS_SET 0x6032          // 采集状态集（标准 A.7，IC 11）
#define OI_ACQ_STATUS 0x6033              // 一个采集状态（标准 A.7，IC 8）
#define OI_TASK_MONITOR_SET 0x6034        // 采集任务监控集（标准 A.7，IC 11）
#define OI_TASK_MONITOR 0x6035            // 采集任务监控单元（标准 A.7，IC 8）
#define OI_ACQ_START_TIME 0x6040          // 采集启动时标（标准 A.7，IC 8）
#define OI_ACQ_SUCCESS_TIME 0x6041        // 采集成功时标（标准 A.7，IC 8）
#define OI_ACQ_STORE_TIME 0x6042          // 采集存储时标（标准 A.7，IC 8）
#define OI_METER_DISCOVERY 0x6002   // 搜表（Class 11 集合对象）
#define OI_FILE_SET 0x7000          // 文件集合（标准 A.8，IC 11）
#define OI_SCRIPT_SET 0x7010        // 脚本集合（标准 A.8，IC 11）
#define OI_PULSE_METER 0x2401       // 脉冲计量（标准 7.3.12 表157，IC 12）
#define OI_GROUP_FIRST 0x2301       // 总加组 1（标准 7.3.23 表190，IC 23）
#define OI_GROUP_LAST 0x2308        // 总加组 8
#define OI_FILE_FRAME 0xF000        // 文件分帧传输管理（表A.11，IC 18）
#define OI_FILE_CHUNK 0xF001        // 文件分块传输管理（表A.11，IC 18）
#define OI_FILE_EXT 0xF002          // 文件扩展传输管理（表A.11，IC 18）
#define OI_DEVICE_ELECTRIC 0x4300   // 电气设备（标准 7.3.19 表178，IC 19）
#define OI_DEVICE_WATER 0x4307      // 水表（标准设备管理 OI 方法表，IC 19）
#define OI_DEVICE_GAS 0x4308        // 气表（标准设备管理 OI 方法表，IC 19）
#define OI_DEVICE_HEAT 0x4309       // 热表（标准设备管理 OI 方法表，IC 19）
#define OI_WIRELESS_COMM_FIRST 0x4500 // 公网通信模块 1（标准 7.3.25 表196，IC 25）
#define OI_WIRELESS_COMM_LAST 0x4501  // 公网通信模块 2
#define OI_ETHERNET_COMM_FIRST 0x4510 // 以太网通信模块 1（标准 7.3.26 表199，IC 26）
#define OI_ETHERNET_COMM_LAST 0x4517  // 以太网通信模块 8
#define OI_REMOTE_CONTROL 0x8000    // 远程拉闸动作控制 OI
#define OI_POWER_SAVE 0x8001        // 保电
#define OI_TERMINAL_SECURITY 0x8100 // 终端保安定值
#define OI_POWER_CTRL_PERIOD 0x8101 // 终端功控时段
#define OI_POWER_CTRL_ALARM 0x8102  // 功控告警时间
#define OI_DAILY_POWER_CTRL 0x8103  // 时段功控
#define OI_CONTROL_OBJECT 0x810E    // 控制对象
#define OI_TRIP_ROUND 0x810F        // 跳闸轮次
#define OI_ELEC_CTRL_VALUE 0x8110   // 电控定值
#define OI_ESAM_SECURITY 0xF100     // ESAM 北向鉴权加密芯片 OI
#define OI_CHANNEL_CONFIG 0xF200    // RS232 端口（Class 22）
#define OI_RS485_PORT 0xF201        // RS485 端口
#define OI_INFRARED_PORT 0xF202      // 红外端口
#define OI_DIGITAL_INPUT 0xF203      // 开关量输入
#define OI_DC_ANALOG 0xF204          // 直流模拟量
#define OI_RELAY_OUTPUT 0xF205       // 继电器输出
#define OI_ALARM_OUTPUT 0xF206       // 告警输出
#define OI_MULTI_TERMINAL 0xF207     // 多功能端子
#define OI_AC_SAMPLING 0xF208        // 交采接口
#define OI_LOCAL_MODULE 0xF209       // 载波/微功率无线接口
#define OI_PULSE_INPUT 0xF20A        // 脉冲输入设备
#define OI_BLUETOOTH 0xF20B          // 蓝牙
#define OI_WIRELESS_PRIVATE 0xF20C   // 230M 无线专网接口

#define MAX_EVENT_RECORDS 10
#define MAX_EVENT_SNAPSHOTS 8
#define DCU_EVENT_SNAPSHOT_BYTES 64

/**
 * @brief 事件时标的两个口径：mono_ns 是产生记录时的单调纳秒副本，epoch_s 是墙钟整秒
 *        （date_time_s 与 event_history 落库）。
 *
 * uv_hrtime 是开机以来单调纳秒，不能当公历时间；CLOCK_REALTIME 才是 epoch 来源。
 * 两者必须一起存，任何一侧缺失都如实回 NULL 或类型不匹配，不互相换算冒充。
 */
typedef struct {
    uint64_t mono_ns;
    uint64_t epoch_s;
} dcu_event_stamp_t;

/**
 * @brief Class 7 单个事件记录结构体 (与电力契约数据严格对齐)
 */
typedef struct {
    uint16_t index;
    uint32_t slot_id;
    dcu_event_stamp_t start_time;
    dcu_event_stamp_t end_time;
    float snapshot_value;
    bool is_active;
    /* 生成记录时抓到的关联对象属性 A-XDR 值（A6 记录单元的 Data 字段）。len 0 = 未取到，编 NULL。
     */
    uint8_t snapshot_len[MAX_EVENT_SNAPSHOTS];
    uint8_t snapshot[MAX_EVENT_SNAPSHOTS][DCU_EVENT_SNAPSHOT_BYTES];
} dcu_event_record_t;

/**
 * @brief 分项事件对象存储（Class 24，3000/3005）
 */
typedef struct {
    uint16_t oi;
    dcu_event_record_t records[MAX_EVENT_RECORDS];  // 事件记录表 1 (A6)
    uint16_t record_count;                          // 记录表 1 当前条数 (A3 四表结构)
    uint32_t snapshot_oads[MAX_EVENT_SNAPSHOTS];    // 关联对象属性表 OAD (A2)
    uint8_t snapshot_count;
    float threshold;  // 触发门限 (A5 配置参数结构未暴露)
    uint32_t delay_s;
    uint8_t report_flag;                // 上报标识 (A11)
    bool is_enabled;                    // 有效标识 (A12)
    dcu_event_stamp_t last_start_time;  // 最近发生时间 (A14)
    dcu_event_stamp_t last_end_time;    // 最近结束时间 (A14)
} dcu_event_obj_t;

typedef struct {
    void* ctx;
    int (*record_telemetry)(void* ctx, const char* meter_addr, uint32_t meter_idx, uint32_t tag_id,
                            double val);
    int (*count_freezes)(void* ctx, uint16_t* out_count);
    /* 档案持久化端口（6000 方法用）：成功 0，失败非 0。ctx 为存储实例。 */
    int (*archive_save)(void* ctx, const production_meter_t* m);
    int (*archive_delete)(void* ctx, const char* address);
    int (*archive_clear)(void* ctx);
    /* 6012 任务持久化端口：op 已解码，ctx 是存储实例；返回 EDGE_SLAVE_SRC_*。 */
    int (*task_apply)(void* ctx, dcu_poller_t* poller, const dcu_task_op_t* op);
    /* 6014 方案持久化端口：op 已解码，ctx 是存储实例；返回 EDGE_SLAVE_SRC_*。 */
    int (*scheme_apply)(void* ctx, dcu_poller_t* poller, const dcu_scheme_op_t* op);
} router_sink_ops_t;

typedef struct {
    void* ctx;
    size_t msg_size;
    int (*publish_realtime)(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                            uint64_t timestamp_ns);
    int (*read_shadow)(void* ctx, uint32_t tag_id, uint32_t device_id, void* out_msg);
} router_bus_ops_t;

int dcu_tag_map_fn(uint32_t tag_id, uint32_t device_id);

/**
 * @brief 初始化 DCU 核心对象字典与路由调度中心，注册所有标准 OI 对象
 */
int dcu_router_init_ex(dcu_inventory_t* inv, dcu_poller_t* poller, router_sink_ops_t sink_ops,
                       dcu_esam_t* esam, dcu_discovery_t* discovery, dcu_event_obj_t* evt_3000,
                       dcu_event_obj_t* evt_3005, router_bus_ops_t bus_ops);

/**
 * @brief 传统初始化桥接接口 (兼容旧测试)
 */
int dcu_router_init(dcu_inventory_t* inv, dcu_poller_t* poller, void* storage);

/**
 * @brief GET 属性路由调度：根据 OAD 指针调用特定对象的成员变量读取
 */
int dcu_router_get_attr(uint32_t oad, void* out_val);

/**
 * @brief SET 属性路由调度：根据 OAD 指针写入对应对象的变量配置
 */
int dcu_router_set_attr(uint32_t oad, const void* in_val);

/**
 * @brief ACTION 方法调用分发：根据 OMD 物理指针执行特定的动作函数 (如注册、拉闸、鉴权)
 */
/**
 * @brief 遥控动作的下沉端口（组合根注入，实现是聚合根 `dcu_node_execute_control`）。
 *
 * 为什么不让路由自己发帧：路由只认识对象与属性，发 645 控制帧是**领域动作**，
 * 必须过聚合根的前置检查（在线、白名单、闭锁）。路由直连总线就是把
 * "南向控制严禁穿透"这条破掉了（架构原则 3）。
 */
typedef int (*dcu_control_sink_t)(void* ctx, const char* meter_addr, uint16_t control_oi,
                                  bool trip);

/** 注入下沉端口；未注入时 OI 8000 返回 -4（未装配），不会假装成功。 */
void dcu_router_set_control_sink(dcu_control_sink_t fn, void* ctx);

int dcu_router_action(uint32_t omd, const void* param, void* result);

/**
 * @brief [Generic Snapshot Engine] 根据 OI 的 Attr 3 (关联对象属性表) 自动从 DataHub 抓取断面数据
 *
 * @param oi 对象标识
 * @param device_id 设备实例 ID (用于 2D 矩阵寻址)
 * @param out_msgs 存储抓取结果的数组
 * @param max_count 数组容量
 * @return 实际抓取的条数
 */
int dcu_obj_snapshot(uint16_t oi, uint32_t device_id, void* out_msgs, int max_count);

/**
 * @brief 释放路由器与对象字典所有分配的资源
 */
void dcu_router_destroy(void);

#endif
