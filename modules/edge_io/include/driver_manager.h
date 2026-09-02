#ifndef DRIVER_MANAGER_H
#define DRIVER_MANAGER_H

#include <uv.h>
#include "contract_data_types.h"
#include "edge_driver.h"
#include "io_executor.h"

// 前向声明 DataHub 与内存池
struct datahub_s;
typedef struct datahub_s datahub_t;

typedef struct driver_manager_s driver_manager_t;

/**
 * @brief 轮询调度策略
 */
typedef struct {
    uint64_t interval_ms;       /**< 轮询周期 (ms) */
    uint32_t timeout_ms;        /**< 响应超时时间 (ms) */
    uint32_t max_retries;       /**< 超时重试次数 */
    bool mtt_wall_clock_align;  /**< 是否对齐挂钟整点时间 */
    uint32_t priority;          /**< 调度优先级 (0 = 默认, 1 = 高频遥测) */
} edge_poll_policy_t;

/**
 * @brief 自动轮询调度器全局配置
 */
typedef struct {
    uv_loop_t *loop;
    datahub_t *hub;
    net_buf_pool_t *io_pool;
    uint32_t max_channels;
} edge_orchestrator_cfg_t;

/**
 * @brief 轮询通道统计指标
 */
typedef struct {
    uint64_t poll_count;        /**< 发起轮询总次数 */
    uint64_t success_count;     /**< 成功响应并解析入库次数 */
    uint64_t timeout_count;     /**< 超时次数 */
    uint64_t error_count;       /**< 编码/解码/底层链路错误次数 */
    uint64_t consecutive_fails; /**< 当前连续失败次数 */
    uint64_t last_poll_ts;      /**< 上次轮询发起时间 (ms) */
    uint64_t last_success_ts;   /**< 上次成功时间 (ms) */
} edge_poll_stats_t;

/**
 * @brief 创建驱动管理器实例 (组合模式)
 */
driver_manager_t *driver_manager_create(void);

/**
 * @brief 销毁管理器
 */
void driver_manager_destroy(driver_manager_t *mgr);

/**
 * @brief 注册协议模板到指定管理器
 */
int driver_manager_register(driver_manager_t *mgr, const edge_driver_t *driver);

/**
 * @brief [AEP-301] 批量注册由编译器生成的驱动注册表
 * @param mgr 驱动管理器
 * @param registry 驱动注册表数组
 * @param count 驱动注册表项数量
 * @return 0 成功，非0 失败
 */
int driver_manager_register_table(driver_manager_t *mgr, const edge_driver_reg_entry_t *registry, size_t count);

/**
 * @brief 查找协议模板
 */
const edge_driver_t *driver_manager_find(driver_manager_t *mgr, const char *name);

/**
 * @brief 查找模型对应的契约注入包
 */
const driver_contract_injection_t *edge_model_find(const char *name);

/**
 * @brief 实例化驱动上下文 (Stateful V3)
 */
edge_driver_ctx_t *driver_instance_create(driver_manager_t *mgr, const char *protocol,
                                          uint32_t device_id);

/**
 * @brief 销毁驱动实例
 */
void driver_instance_destroy(edge_driver_ctx_t *ctx, const char *protocol);

/**
 * @brief 初始化并启动南向自动轮询编排引擎
 */
int driver_manager_start_auto_polling(driver_manager_t *mgr, const edge_orchestrator_cfg_t *cfg);

/**
 * @brief 声明式注册物理设备轮询通道
 * @param mgr 驱动管理器句柄
 * @param device_id 目标设备 ID
 * @param protocol 协议名称 (如 "DLT645", "ModbusRTU")
 * @param io_handle 南向传输层句柄 (串口/TCP/Socket)
 * @param tag_ids 需要轮询的测点 ID 列表
 * @param tag_count 测点数量
 * @param policy 调度策略
 */
int driver_manager_add_poll_target(driver_manager_t *mgr, uint32_t device_id,
                                   const char *protocol, io_handle_t *io_handle,
                                   const uint32_t *tag_ids, size_t tag_count,
                                   const edge_poll_policy_t *policy);

/**
 * @brief 暂停/恢复指定通道轮询
 */
int driver_manager_set_target_active(driver_manager_t *mgr, uint32_t device_id, bool active);

/**
 * @brief 获取通道运行统计指标
 */
int driver_manager_get_target_stats(driver_manager_t *mgr, uint32_t device_id, edge_poll_stats_t *stats);

/**
 * @brief 手动立即单次触发指定设备轮询
 */
int driver_manager_trigger_poll_now(driver_manager_t *mgr, uint32_t device_id);

/**
 * @brief 喂入收到的物理帧响应并触发自动解码入库
 */
int driver_manager_feed_response(driver_manager_t *mgr, uint32_t device_id, net_buf_t *rx_buf);

/**
 * @brief 停止并销毁自动轮询编排引擎
 */
int driver_manager_stop_auto_polling(driver_manager_t *mgr);

#endif