/**
 * @file edge_slave_plane.h
 * @brief 从站面的取数/收数端口（ADR-002 S3-0）
 *
 * ## 为什么需要这个文件
 *
 * 7 个驱动的 `handle_rx`（从站应答）原来做三件事：
 *   1. `lookup_by_key(addr)`            —— 查点位表
 *   2. `edge_driver_read_shadow(...)`   —— 读当前值
 *   3. `edge_driver_publish(...)`       —— 写回
 *
 * 这三样都是"驱动反向读消费侧"，是 ADR-002 要删除的对象。删掉之后，
 * 驱动在组应答时需要的"当前值"只能由**调用方**给。
 *
 * ## 边界纪律（三条，缺一不可）
 *
 * 1. **接口由消费方定义**：需要"给我这个点位的当前值"的是 edge_io 的从站编排，
 *    所以端口声明在 edge_io；实现在 cmd（只有产品认识自己的点位模型）。
 * 2. **端口不进 `edge_driver_ctx_t`**：进了 ctx，驱动就又变成"自己取数"，
 *    等于把 `read_shadow` 换个名字请回来。端口**每次调用现传**。
 * 3. **组帧留在协议层**：CS、+0x33、控制码是协议知识，cmd 不该知道。
 */

#ifndef EDGE_SLAVE_PLANE_H
#define EDGE_SLAVE_PLANE_H

#include <stdint.h>

#include "shmring.h" /* pipe_msg_t */

#ifdef __cplusplus
extern "C" {
#endif

/** 从站应答需要的一个点位取值 */
typedef struct {
    double value;    /**< 工程值（已标定） */
    int32_t raw;     /**< 未标定原始值；驱动编码字节用这个 */
    uint8_t width;   /**< 数据宽度：645 是字节数(2/4)，Modbus 是寄存器个数 */
    uint8_t quality; /**< 质量码：645 不用；IEC104 需要 */
    uint64_t ts_ms;  /**< 取值时标 */
} edge_point_value_t;

/**
 * @brief 从站取数/收数端口。由 cmd 实现，组合根注入。
 */
/**
 * @brief 北向生产者端口：读本节点某个 tag 的当前状态，用于组北向报文。
 *
 * 与 `edge_slave_source_ops_t` **并列但不同用途**：
 *   - 从站端口：南向从站应答，"这个点位现在是多少" → 标量就够
 *   - 北向端口：北向发帧，"这个 tag 现在是什么状态" → 可能是 BATCH/EVENT/标量
 * 所以这里用 pipe_msg_t 而不是标量——形状由消费方（北向编排）决定。
 *
 * 纪律与从站端口完全一致：声明在 edge_io、实现在 cmd、组合根注入，
 * **不进 `edge_driver_ctx_t`**，每次调用现传。
 */
struct edge_north_source_ops_s {
    int (*peek)(void *ctx, uint32_t tag_id, pipe_msg_t *out);
};
typedef struct edge_north_source_ops_s edge_north_source_ops_t;

typedef struct {
    /**
     * @brief 取点位当前值。
     * @return 0 = 取到；非 0 = 无此点位/不可读 → 从站回异常应答
     */
    int (*resolve)(void *ctx, uint32_t point_id, edge_point_value_t *out);

    /**
     * @brief 收下主站写入的值。
     * @param value 工程值
     * @return 0 = 接受
     */
    int (*accept)(void *ctx, uint32_t point_id, double value);

    /**
     * @brief 遍历本从站的全部点位（总召唤 / 批量读要用）。
     *
     * 只靠 `resolve(point_id)` 不够：IEC101 响应总召唤（C_IC_NA_1）时，
     * 驱动必须知道"这个从站有哪些点位"才能逐个组 ASDU。
     * 回调式，避免为核心路径引入动态内存。
     *
     * @param fn 对每个点位回调一次（point_id 是协议原生标识）
     * @return 0 = 成功
     */
    int (*enumerate)(void *ctx, void (*fn)(void *ud, uint32_t point_id), void *ud);
} edge_slave_source_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGE_SLAVE_PLANE_H */
