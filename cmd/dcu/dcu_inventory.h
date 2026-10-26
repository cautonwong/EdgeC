#ifndef DCU_INVENTORY_H
#define DCU_INVENTORY_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#define MAX_METERS 4096
#define MAX_ADDR_LEN 32

#include "contract_data_types.h"

/**
 * @brief 工业生产级电能表档案实体 (对应 OI 6001 采集档案配置单元)
 */
typedef struct {
    char address[MAX_ADDR_LEN];  // 电能表通信地址 (TSA)
    char protocol[16];           // 通信协议类型 (dlt698, dlt645 等)
    uint32_t baud_rate;          // 串口波特率
    uint8_t channel_id;          // 关联的物理通道/串口 ID

    // [PRD Alignment] 6001 附加属性
    uint8_t wiring_mode;      // 接线方式
    char collector_addr[16];  // 采集器地址
    char asset_no[32];        // 资产号
    uint32_t pt_ratio;        // PT变比
    uint32_t ct_ratio;        // CT变比

    bool is_online;      // 状态：true 表示 ACTIVE, false 表示 SUSPENDED (熔断离线)
    uint32_t last_poll_time_s;                  // 上一次被调度抄读的挂钟时间戳 (秒)
    const driver_contract_injection_t* inject;  // 关联的契约注入字典
} production_meter_t;

/**
 * @brief 集中器电表档案容器
 *
 * 线程所有权：`meters` / `count` / `revision` 只由 uv 事件循环线程读写
 * （协议入口 `_oi_6000_execute`、`dcu_node` 装载、`dcu_poller_step` 全在循环线程上），
 * 因此 `add` / `remove` / `load_from_db` 不再各自加锁 —— 单线程下加锁只会造成伪安全。
 * 因此本结构不再带锁 —— 一把没有对应写者的锁只会造成伪安全。
 * 若将来要在循环线程之外改档案，必须另加锁并把修改和 `revision++` 放进同一临界区。
 */
typedef struct {
    production_meter_t meters[MAX_METERS];
    uint16_t count;
    uint32_t revision; // 档案变更版本号：删除后又新增同数目不会变 count，靠它察觉
} dcu_inventory_t;

/* --- 生产级 API 函数原型声明 --- */

/**
 * @brief 初始化电表档案容器
 */
int dcu_inventory_init(dcu_inventory_t* inv);

/**
 * @brief 增加一台电表注册档案 (C11 边界安全拷贝)
 */
int dcu_inventory_add(dcu_inventory_t* inv, const char* addr, const char* proto, uint32_t baud,
                      uint8_t channel, const driver_contract_injection_t* inject);

/**
 * @brief 删除指定 TSA 地址的表计
 */
int dcu_inventory_remove(dcu_inventory_t* inv, const char* addr);

/**
 * @brief 根据 TSA 地址查找对应的表计档案条目
 */
production_meter_t* dcu_inventory_get(dcu_inventory_t* inv, const char* addr);

/**
 * @brief 取通道上首个表计的协议名 (档案驱动, 非按通道号猜测)
 * @return 该通道无表计时返回 NULL; 同通道混协议时取首个并打 WARN
 */
const char* dcu_inventory_channel_protocol(const dcu_inventory_t* inv, uint8_t channel_id);

/**
 * @brief 从 SQLite 数据库加载表计档案
 */
int dcu_inventory_load_from_db(dcu_inventory_t* inv, const char* db_path);

/**
 * @brief 释放资源与销毁容器
 */
void dcu_inventory_destroy(dcu_inventory_t* inv);

#endif  // DCU_INVENTORY_H
