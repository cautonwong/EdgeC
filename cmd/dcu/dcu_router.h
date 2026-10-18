#ifndef DCU_ROUTER_H
#define DCU_ROUTER_H

#include <stdbool.h>
#include <stdint.h>

#include "dcu_discovery.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_security.h"

// ==============================================================================
// 698 协议接口类 (Interface Class ID) 与标准 OI 定义
// ==============================================================================
#define IC_VARIABLE 3      // 变量接口类 (如电压、电流、电能量)
#define IC_EVENT 7         // 事件对象接口类 (如失压 3000, 过流 3005)
#define IC_RECORD 9        // 记录/冻结数据接口类 (如日冻结 5004)
#define IC_ACQUISITION 10  // 采集监控配置类 (如 6000 档案, 6012 任务, F100 ESAM)
#define IC_CONTROL 13      // 控制/继电器动作类 (如 8000 远程拉合闸)

// ==============================================================================
// DLT 698.45 标准寻址 OI 与 OAD 契约宏 (消灭所有 Hardcoded 魔数)
// ==============================================================================
// 💡 1. 实时测点标量变量类标准 OAD (OI 2字节 + 属性 1字节 + 索引 1字节)
#define OAD_ACTIVE_ENERGY       0x00100200  // 累计正向有功电能量 (OI 0010 属性 2)
#define OAD_REACTIVE_ENERGY     0x00200200  // 累计反向有功电能量 (OI 0020 属性 2)
#define OAD_VOLTAGE             0x20000200  // A相实时电压 (OI 2000 属性 2)
#define OAD_VOLTAGE_B           0x20000300  // B相实时电压 (OI 2000 属性 3)
#define OAD_VOLTAGE_C           0x20000400  // C相实时电压 (OI 2000 属性 4)
#define OAD_CURRENT             0x20010200  // A相实时电流 (OI 2001 属性 2)
#define OAD_CURRENT_B           0x20010300  // B相实时电流 (OI 2001 属性 3)
#define OAD_CURRENT_C           0x20010400  // C相实时电流 (OI 2001 属性 4)
#define OAD_ZERO_SEQ_CURRENT    0x20020200  // 零序电流 (OI 2002 属性 2)
#define OAD_ACTIVE_POWER_TOTAL  0x20040200  // 总有功功率 (OI 2004 属性 2)
#define OAD_ACTIVE_POWER_A      0x20040300  // A相有功功率 (OI 2004 属性 3)
#define OAD_ACTIVE_POWER_B      0x20040400  // B相有功功率 (OI 2004 属性 4)
#define OAD_ACTIVE_POWER_C      0x20040500  // C相有功功率 (OI 2004 属性 5)
#define OAD_REACTIVE_POWER_TOTAL 0x20050200 // 总无功功率 (OI 2005 属性 2)
#define OAD_POWER_FACTOR_TOTAL  0x20090200  // 总功率因数 (OI 2009 属性 2)
#define OAD_GRID_FREQUENCY      0x200A0200  // 电网频率 (OI 200A 属性 2)

// 💡 2. 异常越限告警监控事件类标准 OI (16-bit OI)
#define OI_VOLTAGE_DROP 0x3000  // 失压事件 OI
#define OI_OVER_CURRENT 0x3005  // 过流事件 OI

// 💡 3. 定期归档冻结数据类标准 OAD
#define OAD_DAILY_FREEZE \
    0x50040200  // 日冻结记录表属性 2 (日冻结 OAD 既作为定时流触发 Tag，也是存储召测 Target)

// 💡 4. 集中器控制、搜表与安全接口类标准 OI
#define OI_METER_ARCHIVE 0x6000     // 采集档案配置表 OI
#define OI_TASK_SCHEDULER 0x6012    // 任务配置表 OI
#define OI_METER_DISCOVERY 0x6002   // 搜表发现控制器 OI
#define OI_DISCOVERY_RESULT 0x6003  // 搜表结果表 OI
#define OI_REMOTE_CONTROL 0x8000    // 远程拉闸动作控制 OI
#define OI_ESAM_SECURITY 0xF100     // ESAM 北向鉴权加密芯片 OI
#define OI_CHANNEL_CONFIG 0xF200    // 外设串口信道物理配置 OI

#define MAX_EVENT_RECORDS 10

/**
 * @brief Class 7 单个事件记录结构体 (与电力契约数据严格对齐)
 */
typedef struct {
    uint16_t index;        // 事件记录序号，单调递增
    uint64_t start_time;   // 事件发生挂钟时间戳 (秒)
    uint64_t end_time;     // 事件恢复挂钟时间戳 (秒)
    float snapshot_value;  // 发生/恢复时的物理量实时快照值 (如电压/电流)
    bool is_active;        // 事件当前是否处于激活/发生状态
} dcu_event_record_t;

/**
 * @brief Class 7 异常事件监控对象结构体
 */
typedef struct {
    uint16_t oi;                                    // 事件对象 OID (如 OI_VOLTAGE_DROP)
    dcu_event_record_t records[MAX_EVENT_RECORDS];  // 倒序排列的事件记录表 (属性 2)
    uint16_t record_count;                          // 当前记录条数 (属性 4)
    uint32_t snapshot_oads[8];  // [Align Attr 3] 关联抓取的快照 OAD 列表
    uint8_t snapshot_count;     // 快照列表长度
    float threshold;            // 判定门限值 (属性 6 配置参数)
    uint32_t delay_s;           // 判定触发延时时间 (秒)
    uint8_t report_flag;        // 上报标识 (属性 8)
    bool is_enabled;            // 是否使能有效 (属性 9)
    uint64_t last_start_time;   // 最近发生时间 (属性 10 的部分元素)
    uint64_t last_end_time;     // 最近恢复时间
} dcu_event_obj_t;

// === [Consumer-Defined Interfaces] ===
typedef struct {
    void* ctx;
    int (*record_telemetry)(void* ctx, const char* meter_addr, uint32_t meter_idx, uint32_t tag_id,
                            double val);
    int (*count_freezes)(void* ctx, uint16_t* out_count);
} router_sink_ops_t;

typedef struct {
    void* ctx;
    size_t msg_size;
    int (*publish_realtime)(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                            uint64_t timestamp_ns);
    int (*read_shadow)(void* ctx, uint32_t tag_id, uint32_t device_id, void* out_msg);
} router_bus_ops_t;

// 通用 OI 实例虚拟方法表结构体
typedef struct dcu_object_s dcu_object_t;

struct dcu_object_s {
    uint16_t oi;         // 16位对象标识 (例如 0x6000, 0x2000, 0x8000)
    uint16_t class_id;   // 接口类 ID (如 IC_VARIABLE)
    void* instance_ptr;  // 指向模块实体实例数据指针

    // 面向对象 C 接口虚函数映射
    int (*read_attr)(dcu_object_t* obj, uint8_t attr_id, uint8_t elem_idx, void* out_val);
    int (*write_attr)(dcu_object_t* obj, uint8_t attr_id, uint8_t elem_idx, const void* in_val);
    int (*execute_method)(dcu_object_t* obj, uint8_t method_id, const void* param, void* result);
};

/* --- 生产级 API 函数原型声明 --- */

/**
 * @brief 影子映射：物理矩阵映射 (Index = MeterIdx * 10 + Offset)
 * 用于将 698 OAD 或物理测点映射到 DataHub 影子寄存器槽位
 */
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

#endif  // DCU_ROUTER_H
