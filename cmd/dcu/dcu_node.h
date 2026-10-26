/**
 * @file dcu_node.h
 * @brief DCU Edge Collector Domain Aggregate Root (Issue #375 / Clean Architecture)
 *
 * Encapsulates the Smart Meter Inventory, Poller Engine, Event Monitoring,
 * Storage Sink, and DataHub Shadow Bus into a single Aggregate Root.
 */

#ifndef DCU_NODE_H
#define DCU_NODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "datahub.h"
#include "dcu_event_monitor.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dcu_storage.h"
#include "edge_composition_root.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dcu_node_s dcu_node_t;

typedef struct {
    const char *product_name;       /**< Product identifier, default "DCU" */
    const char *db_path;            /**< Path to SQLite digital twin database */
    size_t datahub_capacity;        /**< DataHub tag capacity (e.g. 32768) */
    uint32_t poll_interval_ms;      /**< Poller timer period in ms (default 1000) */
    poller_source_ops_t source_ops; /**< Injected physical/twin meter data source adapter */
} dcu_node_config_t;

/**
 * @brief Create and assemble a DCU Node Aggregate Root
 */
dcu_node_t *dcu_node_create(uv_loop_t *loop, const dcu_node_config_t *cfg);

/**
 * @brief Start the DCU Node polling loop and event engines
 */
int dcu_node_start(dcu_node_t *node);

/**
 * @brief Stop the DCU Node polling loop
 */
int dcu_node_stop(dcu_node_t *node);

/**
 * @brief 设置表计数据源（组合根在拿到 hub/inv 之后一次性交给聚合根）
 * @note 必须在 dcu_node_start() 之前调用：poller 每轮直接读该字段，运行期替换会
 *       丢掉旧源的 ctx（及其在途请求），故不做运行期热替换。
 */
void dcu_node_set_source_ops(dcu_node_t *node, poller_source_ops_t source_ops);

/**
 * @brief Perform a single polling pump cycle across active inventory
 */
int dcu_node_pump(dcu_node_t *node);

/**
 * @brief Access encapsulated Meter Inventory
 */
/**
 * @brief 聚合根唯一进件入口：驱动 decode 完直接调这里，不经 shadow 反查。
 *
 * @param device_id 表在档案中的下标（= 驱动会话的 device_id）
 * @param proto_id  线上协议号(DLT645 的 DI / Modbus 寄存器号)；OAD 归属与 scale 由本层点表决定
 * @param raw       驱动解出的原始值字节（当前是 pipe_msg 的 double）
 * @param raw_len   raw 的字节数
 */
int dcu_node_record_reading(dcu_node_t *node, uint32_t device_id, uint32_t proto_id,
                            const uint8_t *raw, size_t raw_len);

/** @brief 取走自上轮以来的落库条数（供轮次驱动用，取后清零） */
uint32_t dcu_node_take_saved(dcu_node_t *node);

dcu_inventory_t *dcu_node_get_inventory(dcu_node_t *node);

/**
 * @brief Access encapsulated Poller Engine
 */
dcu_poller_t *dcu_node_get_poller(dcu_node_t *node);

/**
 * @brief Access encapsulated Storage Sink
 */
dcu_storage_t *dcu_node_get_storage(dcu_node_t *node);

/**
 * @brief Access encapsulated DataHub
 */
datahub_t *dcu_node_get_datahub(dcu_node_t *node);

/**
 * @brief Access Composition Root Container
 */
edge_composition_container_t *dcu_node_get_container(dcu_node_t *node);

/**
 * @brief Access Event Objects (Voltage Drop / Overcurrent)
 */
dcu_event_obj_t *dcu_node_get_event_3000(dcu_node_t *node);
dcu_event_obj_t *dcu_node_get_event_3005(dcu_node_t *node);

/**
 * @brief 领域事件类型定义 (微服务架构消息解耦)
 */
typedef enum {
    DCU_EVENT_TELEMETRY = 1,     /**< 实时遥测数据到达 */
    DCU_EVENT_VOLTAGE_DROP,      /**< 失压事件发生/消除 */
    DCU_EVENT_OVER_CURRENT,      /**< 过流事件发生/消除 */
    DCU_EVENT_REMOTE_TRIP,       /**< 远程拉闸/合闸动作事件 */
} dcu_event_type_t;

typedef struct {
    dcu_event_type_t type;
    char meter_addr[MAX_ADDR_LEN];
    uint32_t oad;
    double value;
    uint64_t timestamp_ns;
    bool is_alarm_active;
} dcu_domain_event_t;

typedef void (*dcu_event_handler_t)(const dcu_domain_event_t *event, void *user_data);

/**
 * @brief 注册微服务事件监听器 (对接微服务消息总线 / MQTT / IPC)
 */
int dcu_node_set_event_handler(dcu_node_t *node, dcu_event_handler_t handler, void *user_data);

/**
 * @brief 遥测输入领域方法 (In-Process Domain Method)
 * 适配器解析出 698/645 报文后直接调用此方法，更新聚合根状态并向事件总线广播
 */
int dcu_node_feed_meter_reading(dcu_node_t *node, const char *meter_addr, uint32_t oad,
                                double val, uint64_t ts_ns);

/**
 * @brief 下行控制领域方法 (Inbound Command Method)
 * 执行远程拉闸/合闸等安全控制，内置五防与权限前置校验，绝不走 DataHub 穿透
 */
int dcu_node_execute_control(dcu_node_t *node, const char *meter_addr, uint16_t control_oi);

/**
 * @brief Destroy the DCU Node and clean up all resources
 */
void dcu_node_destroy(dcu_node_t *node);

#ifdef __cplusplus
}
#endif

#endif /* DCU_NODE_H */
