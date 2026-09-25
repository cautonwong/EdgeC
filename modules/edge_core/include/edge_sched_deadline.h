/**
 * @file edge_sched_deadline.h
 * @brief 确定性实时任务调度死线超限监控器与熔断器 (AEP-720 / Issue #251)
 * @details 负责工业边缘控制平面（Control Plane）关键控制任务的执行死线（Deadline）、
 *          周期抖动（Jitter）、WCET 统计监控以及连续超限熔断/降级后备保护。
 *          遵循三原则：
 *          1. 消费者定义接口 (Consumer-Defined Ports)
 *          2. 不透明聚合根与调用方分配存储 (Caller-Allocated Storage, 0 malloc)
 *          3. 组合根就地组装
 */

#ifndef EDGE_SCHED_DEADLINE_H
#define EDGE_SCHED_DEADLINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_SCHED_MAX_TASKS        32
#define EDGE_SCHED_TASK_NAME_LEN    32
#define EDGE_SCHED_DEFAULT_MAX_JITTER_US 500
#define EDGE_SCHED_DEFAULT_TRIP_OVERRUNS 3

/**
 * @brief 任务熔断器与健康状态
 */
typedef enum {
    EDGE_SCHED_STATE_NORMAL = 0,    /**< 正常运行态 (执行耗时与抖动在死线内) */
    EDGE_SCHED_STATE_WARNING,       /**< 预警态 (单次超限或抖动逼近死线) */
    EDGE_SCHED_STATE_TRIPPED,       /**< 熔断态 (连续超限达到门限，触发熔断) */
    EDGE_SCHED_STATE_DEGRADED       /**< 降级后备态 (切换至低频或简化控制律) */
} edge_sched_circuit_state_t;

/**
 * @brief 死线违规处置动作
 */
typedef enum {
    EDGE_SCHED_ACTION_LOG_ONLY = 0, /**< 仅记录审计日志 */
    EDGE_SCHED_ACTION_WARN,         /**< 上报告警并保持监控 */
    EDGE_SCHED_ACTION_THROTTLE,     /**< 动态降低后续执行频率 */
    EDGE_SCHED_ACTION_SAFE_FALLBACK,/**< 切换至安全后备控制律 */
    EDGE_SCHED_ACTION_CIRCUIT_TRIP  /**< 立即切断/停止该任务 */
} edge_sched_action_t;

/**
 * @brief 单个任务调度与死线监控配置
 */
typedef struct {
    uint32_t task_id;                         /**< 任务全局唯一 ID */
    char name[EDGE_SCHED_TASK_NAME_LEN];      /**< 任务标识名称 */
    uint64_t period_us;                       /**< 预期运行周期 (微秒, e.g. 10000 = 10ms) */
    uint64_t deadline_us;                     /**< 硬/软实时执行死线预算 (微秒, e.g. 2000 = 2ms) */
    uint64_t max_allowed_jitter_us;           /**< 最大允许周期抖动 (微秒) */
    uint32_t max_consecutive_overruns;        /**< 触发熔断的连续超限次数门限 (e.g. 3) */
    edge_sched_action_t fallback_action;      /**< 超限熔断触发后的安全动作 */
} edge_sched_task_config_t;

/**
 * @brief 任务运行时统计与 WCET 分析指标
 */
typedef struct {
    uint32_t task_id;
    char name[EDGE_SCHED_TASK_NAME_LEN];
    edge_sched_circuit_state_t state;         /**< 当前熔断器状态 */
    uint64_t total_runs;                      /**< 累计执行总轮数 */
    uint64_t deadline_misses;                 /**< 累计死线违规总次数 */
    uint32_t consecutive_misses;              /**< 当前连续超限违规次数 */
    uint64_t min_duration_us;                 /**< 历史最小执行耗时 (BCET) */
    uint64_t max_duration_us;                 /**< 历史最大执行耗时 (WCET) */
    uint64_t last_duration_us;                /**< 上次单次执行耗时 */
    double   avg_duration_us;                 /**< 滑动平均执行耗时 (EMA) */
    uint64_t max_jitter_us;                   /**< 历史最大周期启动抖动 (Jitter) */
    uint64_t last_jitter_us;                  /**< 上次周期启动抖动 */
    uint64_t last_start_time_us;              /**< 上次开始时间戳 */
    uint64_t last_end_time_us;                /**< 上次结束时间戳 */
} edge_sched_task_stats_t;

/**
 * @brief 任务单次周期执行打桩令牌
 */
typedef struct {
    uint32_t task_id;
    uint64_t start_time_us;
    bool active;
} edge_sched_token_t;

/**
 * @brief 消费者定义接口 (Consumer-Defined Ports - AEP-751)
 */
typedef struct {
    /**
     * @brief 死线违规或熔断事件通知回调
     * @param ctx 用户上下文
     * @param task_id 任务 ID
     * @param stats 发生违规时的统计镜像
     * @param action 系统执行的处置动作
     */
    void (*on_violation)(void *ctx, uint32_t task_id, const edge_sched_task_stats_t *stats, edge_sched_action_t action);

    /**
     * @brief 挂钟/单调时钟读取回调 (微秒)
     */
    uint64_t (*get_now_us)(void *ctx);

    void *ctx;
} edge_sched_deadline_ports_t;

/**
 * @brief 确定性死线监控器不透明聚合根类型
 */
typedef struct edge_sched_deadline_monitor edge_sched_deadline_monitor_t;

/**
 * @brief 调用方分配存储块 (Caller-Allocated Storage - AEP-752)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[8192];
} edge_sched_deadline_storage_t;

/**
 * @brief 就地构造确定性死线监控器聚合根 (0 malloc)
 * @param store 调用方分配的对齐存储
 * @param ports 消费者定义接口表 (可选, 可为 NULL)
 * @return 构造成功的聚合根指针, 失败返回 NULL
 */
edge_sched_deadline_monitor_t *edge_sched_deadline_create_in(edge_sched_deadline_storage_t *store,
                                                             const edge_sched_deadline_ports_t *ports);

/**
 * @brief 初始化死线监控器 (向下兼容适配)
 * @param mon 监控器句柄
 * @param ports 消费者接口表
 * @return 0 成功, 负数 错误码
 */
int edge_sched_deadline_init(edge_sched_deadline_monitor_t *mon,
                             const edge_sched_deadline_ports_t *ports);

/**
 * @brief 注册受控任务至死线监控器
 * @param mon 监控器句柄
 * @param cfg 任务配置
 * @return 0 成功, 负数 错误码 (如超出容量或配置非法)
 */
int edge_sched_deadline_register_task(edge_sched_deadline_monitor_t *mon,
                                      const edge_sched_task_config_t *cfg);

/**
 * @brief 任务执行周期开始打桩 (Task Begin Hook)
 * @param mon 监控器句柄
 * @param task_id 任务 ID
 * @param token 输出打桩令牌
 * @return 0 允许执行, 1 处于熔断/抑制状态建议跳过, 负数 错误码
 */
int edge_sched_deadline_task_begin(edge_sched_deadline_monitor_t *mon,
                                   uint32_t task_id,
                                   edge_sched_token_t *token);

/**
 * @brief 任务执行周期结束打桩 (Task End Hook)
 * @param mon 监控器句柄
 * @param token 打桩令牌
 * @param out_action 若发生违规返回触发的动作, 正常返回 EDGE_SCHED_ACTION_LOG_ONLY (可为 NULL)
 * @return 0 正常完成且未超时, 1 发生死线超限违规, 2 触发连续超限熔断切断, 负数 错误码
 */
int edge_sched_deadline_task_end(edge_sched_deadline_monitor_t *mon,
                                 edge_sched_token_t *token,
                                 edge_sched_action_t *out_action);

/**
 * @brief 查询单任务的当前运行时统计指标与 WCET
 * @param mon 监控器句柄
 * @param task_id 任务 ID
 * @param out_stats 输出统计结构
 * @return 0 成功, 负数 错误码
 */
int edge_sched_deadline_get_task_stats(const edge_sched_deadline_monitor_t *mon,
                                       uint32_t task_id,
                                       edge_sched_task_stats_t *out_stats);

/**
 * @brief 重置单任务的熔断器状态与连续超限计数器
 * @param mon 监控器句柄
 * @param task_id 任务 ID
 * @return 0 成功, 负数 错误码
 */
int edge_sched_deadline_reset_circuit(edge_sched_deadline_monitor_t *mon,
                                      uint32_t task_id);

/**
 * @brief 获取当前注册的任务总数
 * @param mon 监控器句柄
 * @return 注册任务数
 */
uint32_t edge_sched_deadline_get_task_count(const edge_sched_deadline_monitor_t *mon);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_SCHED_DEADLINE_H */
