/**
 * @file iems_node.h
 * @brief IEMS Commercial Energy Storage Aggregate Root (No-Contract Architecture)
 */

#ifndef IEMS_NODE_H
#define IEMS_NODE_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "datahub.h"
#include "edge_composition_root.h"
#include "iems_bms_cluster_engine.h"
#include "iems_fire_safety_interlock.h"
#include "iems_microgrid_coordinator.h"
#include "iems_multi_pcs_dispatcher.h"
#include "iems_pcs_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iems_node_s iems_node_t;

typedef struct {
    const char *product_name;          /**< 产品标识，默认 "IEMS" */
    const char *db_path;               /**< 数据库文件路径 */
    const char *shm_name;              /**< 共享内存段标识 */
    uint32_t datahub_capacity;         /**< 影子寄存器容量 */
    double max_pcs_power_kw;           /**< PCS 额定功率 (kW) */
    double transformer_capacity_kva;   /**< 变压器额定容量 (kVA) */
    double demand_limit_kw;            /**< 需量控制上限 (kW) */
} iems_node_config_t;

/**
 * @brief 创建并组装 IEMS 聚合根
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[16384];
} iems_node_storage_t;

size_t iems_node_storage_size(const iems_node_config_t *cfg);

iems_node_t *iems_node_create_in(iems_node_storage_t *storage, uv_loop_t *loop,
                                 const iems_node_config_t *cfg);

/**
 * @brief 销毁 IEMS 聚合根并释放所有资源
 */
void iems_node_destroy(iems_node_t *node);

/**
 * @brief 启动 IEMS 节点与自律业务引擎
 */
int iems_node_start(iems_node_t *node);

/**
 * @brief 停止 IEMS 节点
 */
int iems_node_stop(iems_node_t *node);

/**
 * @brief 单步推进微网协调与控制决策周期
 */
int iems_node_step(iems_node_t *node);

/* --- 子聚合获取接口 (Domain Getters) --- */
iems_bms_cluster_engine_t *iems_node_get_bms(iems_node_t *node);
iems_pcs_controller_t *iems_node_get_pcs(iems_node_t *node);
iems_microgrid_coordinator_t *iems_node_get_coordinator(iems_node_t *node);
iems_fire_safety_interlock_t *iems_node_get_fire_safety(iems_node_t *node);
iems_multi_pcs_dispatcher_t *iems_node_get_pcs_dispatcher(iems_node_t *node);
datahub_t *iems_node_get_datahub(iems_node_t *node);
edge_composition_container_t *iems_node_get_container(iems_node_t *node);

/* --- 核心业务领域方法 (Domain Operations) --- */

/**
 * @brief 调度充放电功率 (带消防联动闭锁、BMS热失控限功率与变流器多机分摊保护)
 *
 * @param node 聚合根句柄
 * @param target_kw 期望调度功率 (正放负充)
 * @param out_actual_kw 输出实际被约束裁决后的执行功率
 * @return 0 成功执行, <0 故障/闭锁拒绝
 */
int iems_node_dispatch_power(iems_node_t *node, double target_kw, double *out_actual_kw);

/**
 * @brief 紧急停机断开保护
 */
int iems_node_emergency_trip(iems_node_t *node, const char *reason);

/**
 * @brief 更新单簇 BMS 遥测数据并触发系统聚合与不变量校验
 */
int iems_node_update_bms_rack(iems_node_t *node, uint8_t rack_id, double voltage_v,
                              double current_a, double soc_pct, double max_temp_c);

/**
 * @brief 南向轮询采样的镜像写入口 (镜像聚合的唯一写路径)
 *
 * 不变量:sample_ts 是采样(读帧编码发出)时刻,单位与 pipe_msg_t.ts 相同(uv_hrtime 纳秒);
 * 比 (device_id, tag_id) 槽当前内容更旧的采样被丢弃,迟到的轮询结果不能覆盖更晚的
 * 控制写。device_id 是 inventory 的稠密身份(1..63),不是从站地址;0 表示 tag 级视图,
 * 只写 device 0 槽。
 *
 * 每笔采样先写 (device_id, tag_id) 槽(按设备守卫),再派生 device 0 的 tag 级视图
 * (按视图自己的 ts 守卫)。tag-only 读者(REST/app/UI/CLI)读视图,多机同 tag 互不覆盖。
 * 未采集过的槽读到 ts==0。
 * 调用方须在轮询回调里调用,热路径无堆分配。
 *
 * @param node 聚合根句柄
 * @param device_id 设备身份(inventory id),0 = tag 级视图
 * @param tag_id 采集点
 * @param value 采样值
 * @param sample_ts 采样时刻(请求编码发出时,不是结果到达时)
 * @return 0 成功(含旧采样被丢弃), <0 参数非法或镜像写入失败
 */
int iems_node_ingest_sample(iems_node_t *node, uint32_t device_id, uint32_t tag_id, double value,
                            uint64_t sample_ts);

/**
 * @brief IEMS 控制环路延迟预算与看门狗统计 (AEP-407)
 */
typedef struct {
    uint64_t last_cycle_us;       /**< 最近单步决策与下发总耗时 (微秒) */
    uint64_t max_cycle_us;        /**< 历史最大决策耗时 (微秒) */
    uint64_t watchdog_budget_us;  /**< 控制环路安全看门狗预算截止时间 (微秒，默认 10000us 即 10ms) */
    uint32_t watchdog_trips;      /**< 超出预算触发降级/看门狗保护的次数 */
    bool watchdog_degraded;       /**< 当前是否处于超时降级保护状态 */
} iems_loop_watchdog_stats_t;

/**
 * @brief 配置控制环路看门狗预算与安全降级阈值
 */
int iems_node_set_watchdog_budget(iems_node_t *node, uint64_t budget_us);

/**
 * @brief 获取控制环路延迟统计与看门狗状态
 */
int iems_node_get_watchdog_stats(iems_node_t *node, iems_loop_watchdog_stats_t *out_stats);

/**
 * @brief 重置看门狗降级状态，恢复正常调度
 */
int iems_node_reset_watchdog_degrade(iems_node_t *node);

/* 执行完成状态（消费者定接口）：北向回执只关心成功/失败/超时三态。
 * 总线 cmd_status_t 到这三态的映射由执行适配器负责，聚合根与端口调用方不依赖总线错误码。 */
typedef enum {
    IEMS_WRITE_DONE_OK = 0,
    IEMS_WRITE_DONE_FAILED = 1,
    IEMS_WRITE_DONE_TIMEOUT = 2
} iems_write_done_status_t;

/* 完成回调可能在总线/IO 线程触发，实现方必须自己保证线程安全（北向用 uv_async 回投）。
 * on_done 为 NULL 时行为与旧接口一致：只提交，不要结果。 */
typedef void (*iems_write_done_fn)(void *user, iems_write_done_status_t status);

typedef struct {
    void *ctx;
    int (*write_tag)(void *ctx, int bus_id, uint32_t slave_id, uint32_t tag_id, double value,
                     const char *proto, iems_write_done_fn on_done, void *on_done_user);
} iems_execution_port_t;

/**
 * @brief 注入执行类端口。端口只交给聚合根持有，组合根不直接调用原始写接口。
 */
int iems_node_set_execution_port(iems_node_t *node, iems_execution_port_t port);

/**
 * @brief 领域写入口：先过聚合根校验，再经执行端口下发。
 *
 * slave_id 是点位的物理设备地址，由调用方的轮询路由决定；聚合根不猜默认值。
 * device_id 是 inventory 身份（影子槽键），0 表示只写 tag 级视图。
 * on_done/on_done_user 可选；PCS_CONTROL 的完成回调只覆盖最终这一路写，
 * iems_node_dispatch_power 内部的多路分摊结果本轮不聚合（accepted 语义仍有效）。
 */
int iems_node_write_tag(iems_node_t *node, int bus_id, uint32_t slave_id, uint32_t device_id,
                        uint32_t tag_id, double value, const char *proto,
                        iems_write_done_fn on_done, void *on_done_user);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_NODE_H
