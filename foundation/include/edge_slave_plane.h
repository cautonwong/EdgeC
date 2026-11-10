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

#include "pipe_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 从站应答需要的一个点位取值 */
typedef struct {
    double value;    /**< 工程值（已标定） */
    int32_t raw;     /**< 未标定原始值；驱动编码字节用这个 */
    uint8_t width;   /**< 数据宽度：645 是字节数(2/4)，Modbus 是寄存器个数 */
    /**
     * @brief A-XDR 类型标签（698 用）。0 = 让驱动按 width 推断。
     *
     * 为什么必须由产品给：`width=2` 可能是 long 也可能是 long-unsigned，
     * 编码标签写错，主站解出来的值就是错的（而且不会报错）。类型是点位模型的知识，
     * 驱动不该猜（ADR-002）。
     */
    uint8_t type;
    uint8_t quality; /**< 质量码：645 不用；IEC104 需要 */
    uint64_t ts_ms;  /**< 取值时标 */
} edge_point_value_t;

/**
 * @brief 从站取数/收数端口。由 cmd 实现，组合根注入。
 *
 * 北向面（族 A）的取数端口**不在这里**——见 `edge_north_plane.h`。
 * 两者形状不同是刻意的：从站应答要标量，北向发帧可能要多条消息。
 */
typedef struct {
    /**
     * @brief 取点位当前值。
     *
     * @param point_desc 点位描述（标识超过 32 位的协议用，如 DLMS 的 OBIS）。
     *                   与主站侧同一个约定：**不是表**，就是这一个点。
     * @return 0 = 取到；非 0 = 无此点位/不可读 → 从站回异常应答
     */
    int (*resolve)(void *ctx, uint32_t point_id, const void *point_desc, size_t point_desc_len,
                   edge_point_value_t *out);

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
    /**
     * @brief 收一次"对象方法操作"，参数是原始 A-XDR 字节。
     *
     * 为什么不能复用 `accept` 的 double：698 的方法参数经常是结构，例如
     * OI 8000 方法 129（跳闸）的参数是 `array structure{继电器 OAD, 告警延时,
     * 限电时间, 自动合闸}`（标准表 A.9）。把结构压成一个 double 只能靠猜。
     *
     * 驱动只负责"解出是哪一次操作 + 把参数字节原样交给产品"，语义归产品。
     * 未实现时返回非 0 —— 驱动据此回 DAR=2（暂时失效），**不许回 0**。
     */
    int (*accept_action)(void *ctx, uint32_t omd, const uint8_t *axdr, size_t axdr_len);
} edge_slave_source_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGE_SLAVE_PLANE_H */
