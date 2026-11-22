#ifndef DCU_INVENTORY_H
#define DCU_INVENTORY_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#define MAX_METERS 4096
#define MAX_ADDR_LEN 32

#include "contract_data_types.h"

typedef struct sqlite3 sqlite3;

/**
 * @brief 6001 采集档案配置单元（标准 A.7 的 Basic + Extended）。
 *
 * 6000 的 Add/AddBatch/Update 参数与 6001 A2 都按这个结构搬运，
 * RAM 与 SQLite 落同一份字段。
 */
typedef struct {
    uint16_t config_seq;          // 配置序号（0 表示新增时由档案分配）
    char address[MAX_ADDR_LEN];   // 通信地址 TSA
    uint8_t baud_code;            // 波特率 enum（0–10，255 自适应）
    uint8_t protocol_code;        // 规约类型 enum（1/2 DL/T645，3 DL/T698.45，4 CJ/T188）
    uint8_t port;                 // 端口 OAD 的末字节 = 产品通道号
    char password[16];            // 通信密码（octet-string，按文本保存）
    uint8_t tariff_count;         // 费率个数
    uint8_t user_type;            // 用户类型
    uint8_t wiring_mode;          // 接线方式 enum（0–3）
    uint16_t rated_voltage;       // 额定电压（换算 -1）
    uint16_t rated_current;       // 额定电流（换算 -1）
    uint16_t pt_ratio;            // PT
    uint16_t ct_ratio;            // CT
    char collector_addr[16];      // 采集器地址 TSA
    char asset_no[32];            // 资产号（octet-string，按文本保存）
} dcu_archive_unit_t;

/**
 * @brief 工业生产级电能表档案实体 (对应 OI 6001 采集档案配置单元)
 */
typedef struct {
    char address[MAX_ADDR_LEN];  // 电能表通信地址 (TSA)
    char protocol[16];           // 通信协议类型 (dlt698, dlt645 等)
    uint32_t baud_rate;          // 串口波特率
    uint8_t channel_id;          // 关联的物理通道/串口 ID
    uint8_t baud_code;           // 6001 Basic 的波特率 enum（回读用）
    uint8_t protocol_code;       // 6001 Basic 的规约类型 enum（回读用）
    uint16_t config_seq;         // 6001 配置序号（设备分配、持久化）

    // [PRD Alignment] 6001 附加属性
    uint8_t wiring_mode;      // 接线方式
    char collector_addr[16];  // 采集器地址
    char asset_no[32];        // 资产号
    uint32_t pt_ratio;        // PT变比
    uint32_t ct_ratio;        // CT变比
    char password[16];        // 通信密码（文本）
    uint8_t tariff_count;     // 费率个数
    uint8_t user_type;        // 用户类型
    uint16_t rated_voltage;   // 额定电压（换算 -1）
    uint16_t rated_current;   // 额定电流（换算 -1）

    bool is_online;      // 状态：true 表示 ACTIVE, false 表示 SUSPENDED (熔断离线)
    uint32_t last_poll_time_s;                  // 上一次被调度抄读的挂钟时间戳 (秒)

    /* 影子表的设备身份。
     *
     * `dcu_inventory_remove` 用"末元素覆盖删除位"保持紧凑布局，所以**数组下标会变**。
     * 而影子表按 `device_id * 32 + tag_slot` 索引，device_id 一旦跟着下标走，
     * 删一只表就会把最后一只表的数据挪到别人的槽位上。
     *
     * slot_id 在 add 时分配、跟着表走（swap 是整体 memcpy，它自然一起搬），
     * 是设备在影子表里的稳定身份。 */
    uint32_t slot_id;
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
    uint32_t revision;  // 档案变更版本号：删除后又新增同数目不会变 count，靠它察觉
    uint32_t next_slot; // 下一个待分配的 slot_id（单调递增，耗尽后才回收）
    uint16_t next_config_seq;  // 下一个待分配的 6001 配置序号
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
                      uint8_t channel);

/**
 * @brief 按 6001 配置单元新增一台表（6000 方法 127/128 用）。
 * @return 0 成功；-2 地址重复；-3 容器已满
 */
int dcu_inventory_add_unit(dcu_inventory_t* inv, const dcu_archive_unit_t* unit);

/**
 * @brief 按配置序号改基本信息（6000 方法 129）；地址变化时重设主键。
 * @return 0 成功；-1 未找到；-2 新地址与其它表冲突
 */
int dcu_inventory_update_basic(dcu_inventory_t* inv, uint16_t config_seq,
                               const dcu_archive_unit_t* unit);

/**
 * @brief 按配置序号改扩展/附属信息（6000 方法 130）。
 * @return 0 成功；-1 未找到；-2 附属信息非空（未实现存储）
 */
int dcu_inventory_update_extended(dcu_inventory_t* inv, uint16_t config_seq,
                                  const dcu_archive_unit_t* unit, bool has_annex);

/**
 * @brief 按配置序号删除（6000 方法 131）。
 */
int dcu_inventory_remove_seq(dcu_inventory_t* inv, uint16_t config_seq);

/**
 * @brief 清空整张档案表（6000 方法 134）。
 */
int dcu_inventory_clear(dcu_inventory_t* inv);

const production_meter_t* dcu_inventory_by_seq(const dcu_inventory_t* inv, uint16_t config_seq);

/**
 * @brief 把一只表的完整档案写入 SQLite（新增或更新）。
 */
int dcu_inventory_save_meter(sqlite3* db, const production_meter_t* m);

int dcu_inventory_delete_meter(sqlite3* db, const char* address);

int dcu_inventory_clear_db(sqlite3* db);

/**
 * @brief 按 slot_id 反查档案（影子表用的是 slot_id，不是数组下标）。
 * @return 找到返回实体指针，否则 NULL
 */
const production_meter_t* dcu_inventory_by_slot(const dcu_inventory_t* inv, uint32_t slot_id);

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
