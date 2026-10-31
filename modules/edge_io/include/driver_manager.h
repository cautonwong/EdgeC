#ifndef DRIVER_MANAGER_H
#define DRIVER_MANAGER_H

typedef struct uv_loop_s uv_loop_t;
#include "contract_data_types.h"
#include "edge_driver.h"
#include "io_executor.h"

// 前向声明 DataHub 与内存池
struct datahub_s;
typedef struct datahub_s datahub_t;

#include <stddef.h>
#include <stdalign.h>

typedef struct driver_manager_s driver_manager_t;

typedef struct {
    alignas(max_align_t) uint8_t raw[65536];
} driver_manager_storage_t;

/**
 * @brief 轮询调度策略
 */
typedef struct {
    uint64_t interval_ms;       /**< 轮询周期 (ms) */
    uint32_t timeout_ms;        /**< 响应超时时间 (ms) */
    uint32_t max_retries;       /**< 超时重试次数 */
    bool mtt_wall_clock_align;  /**< 是否对齐挂钟整点时间 */
    uint32_t priority;          /**< 调度优先级 (0 = 默认, 1 = 高频遥测) */
    uint32_t max_in_flight;     /**< 最大并发在途请求数 (通常为 1) */
    uint32_t circuit_break_threshold; /**< 连续失败熔断阈值 (0 = 禁用) */
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
    uint32_t in_flight;         /**< 当前 in-flight 正在等待响应的请求数 */
    uint32_t queue_depth;       /**< 当前待调度任务排队深度 */
    bool is_circuit_broken;     /**< 是否已触发熔断保护 */
} edge_poll_stats_t;

/**
 * @brief 在调用方提供的内存就地创建驱动管理器实例 (Zero-Alloc)
 */
driver_manager_t *driver_manager_create_in(driver_manager_storage_t *storage);

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
/* [ADR-002] 已删除：驱动不再查点位表。点位翻译在产品侧的 *_driver_adapter.c。 */

/**
 * @brief 实例化驱动上下文 (Stateful V3)
 */
edge_driver_ctx_t *driver_instance_create(driver_manager_t *mgr, const char *protocol,
                                          uint32_t device_id);

/**
 * @brief 销毁驱动实例
 */
void driver_instance_destroy(edge_driver_ctx_t *ctx, const char *protocol);

/* [ADR-002] 内置轮询器（start/add_poll_target/set_target_active/get_target_stats/
 * trigger_poll_now/feed_response/stop_auto_polling/reset_circuit_breaker）已删除。
 *
 * 它是第三套轮询实现：直接调驱动的 legacy 槽位 encode_request/decode_response，
 * 绕过请求层，与 dcu_poller（DCU）和 edge_api_read_tag（IEMS/SmartBox）完全重复，
 * 且没有任何产品在用。保留它意味着每个驱动迁移都要额外照顾一条死路径。
 *
 * 需要轮询请用：DCU 走 dcu_poller，其余产品走 edge_api_read_tag / _ex。 */

/**
 * @brief 重置通道熔断状态并恢复轮询调度
 */
int driver_manager_reset_circuit_breaker(driver_manager_t *mgr, uint32_t device_id);

#endif