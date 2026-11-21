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

/** 单条属性的 A-XDR 上限（ADR-0013）；超过的记录型属性走 GetRecord 分页。 */
#define EDGE_SLAVE_AXDR_MAX 2048

/** 从站应答需要的一个点位取值 */
typedef struct {
    double value;  /**< 工程值（已标定） */
    int32_t raw;   /**< 未标定原始值；驱动编码字节用这个 */
    uint8_t width; /**< 数据宽度：645 是字节数(2/4)，Modbus 是寄存器个数 */
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
 * @brief 从站取数/收数回调的失败原因。
 *
 * 698 驱动据此回 DAR（DL/T 698.45 6.3.3.11）；其它驱动只把非 0 当失败。
 * 旧实现返回 -1 时按 NOT_FOUND 处理，保持兼容。
 */
typedef enum {
    EDGE_SLAVE_SRC_OK = 0,
    EDGE_SLAVE_SRC_NOT_FOUND = -20,      /**< 对象不在对象表 → DAR 6 */
    EDGE_SLAVE_SRC_CLASS_MISMATCH = -21, /**< OAD 类与对象不匹配 → DAR 5 */
    EDGE_SLAVE_SRC_TYPE_MISMATCH = -22,  /**< A-XDR 类型/宽度不匹配 → DAR 7 */
    EDGE_SLAVE_SRC_UNDEFINED = -23,      /**< OI 在标准/目录中未定义 → DAR 4 */
    EDGE_SLAVE_SRC_TEMPORARY = -24,      /**< 暂时失效 → DAR 2 */
    EDGE_SLAVE_SRC_DENIED = -25          /**< 读写被拒绝（只读/闭锁）→ DAR 3 */
} edge_slave_src_rc_t;

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
     * @brief 点位死区（工程值）。
     *
     * 自发上报用它区分「小变化静默」与「大变化上报」。返回 0 表示只要变化
     * 就报；返回 <0 表示该点没有死区配置（驱动按 0 处理）。实现可为 NULL。
     */
    double (*deadband)(void *ctx, uint32_t point_id);
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

    /**
     * @brief 主动断开当前主站链路（协议定时器到期时用）。
     *
     * IEC104 的 t1（I 帧 15s 未被确认）在协议上没有"重发"以外的恢复手段；
     * 本产品单客户端、无重发队列，正确反应是断开链路让主站重连。驱动不认识
     * socket，由产品在这里 shutdown 活动连接，让 io 层收到 EOF 并标记 stale。
     *
     * @return 0 = 已发出关闭；-1 = 没有活动连接（驱动忽略返回值）
     */
    int (*close_link)(void *ctx);

    /**
     * @brief 取一个 OAD 的完整 A-XDR 值（含类型标签）。
     *
     * 缓冲由调用方提供；产品只写 `cap` 以内，超限返回非 0，驱动不得截断。
     * 未实现时为 NULL，驱动回落标量 `resolve`；已实现但返回非 0 是真实失败，
     * 不得回落（ADR-0013）。
     */
    int (*resolve_axdr)(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len);

    /**
     * @brief 收一个 OAD 的原始 A-XDR 值（含类型标签）。
     *
     * 未实现时为 NULL，SET 回落标量 `accept`。
     */
    int (*accept_axdr)(void *ctx, uint32_t oad, const uint8_t *axdr, size_t len);
} edge_slave_source_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGE_SLAVE_PLANE_H */
