/**
 * @file edge_north_plane.h
 * @brief 北向取数端口（ADR-0011 族 A）
 *
 * ## 为什么单独一个头
 *
 * 这个端口原来寄居在 `edge_slave_plane.h` 里，注释还得解释"与从站端口并列但用途不同"。
 * 北向和从站是两个面：
 *   - 从站面（族 B）：外部主站来读，取的是**标量当前值**
 *   - 北向面（族 A）：本节点往外发，取的是**一条待编码的消息**（可能是 BATCH/EVENT）
 *
 * 形状不同是因为消费方不同，不是历史遗留。混在一个头里会让人以为可以互换。
 *
 * ## 边界纪律（与从站端口一致，三条）
 *
 * 1. **接口由消费方定义**：需要"这条消息现在是什么"的是北向编排（edge_io），
 *    所以端口声明在 edge_io；实现在 cmd（只有产品认识自己的点位模型）。
 * 2. **端口不进 `edge_driver_ctx_t`**：进了 ctx，驱动就又变成"自己取数"，
 *    等于把 `read_shadow` 换个名字请回来。端口**每次调用现传**。
 * 3. **组帧留在协议层**：A-XDR / protobuf 是协议知识，cmd 不该知道。
 */

#ifndef EDGE_NORTH_PLANE_H
#define EDGE_NORTH_PLANE_H

#include <stdint.h>

#include "shmring.h" /* pipe_msg_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 北向生产者端口：读本节点某个 tag 的当前状态，用于组北向报文。
 *
 * 用 `pipe_msg_t` 而不是标量——北向要发的东西可能是 BATCH/EVENT，
 * 形状由消费方（北向编排）决定，不是标量能覆盖的。
 */
struct edge_north_source_ops_s {
    /**
     * @brief 取一条待发送消息。
     * @return 0 = 取到；非 0 = 无此 tag / 不可读
     */
    int (*peek)(void *ctx, uint32_t tag_id, pipe_msg_t *out);

    /**
     * @brief 点位名（北向报文里的 metric 名）。
     *
     * 名字是"点位模型"的一部分，驱动不认识它——所以由端口给，驱动只负责
     * 把它编进报文。返回 NULL 时驱动回落成数字标识。
     *
     * 这一条取代了 `edge_driver_ctx_t.get_tag_name`（驱动反查 datahub 元数据）。
     */
    const char *(*name)(void *ctx, uint32_t tag_id);

    /**
     * @brief 名字 → 点位标识（**下行**要用：云端只给名字，不给数字）。
     *
     * 与 `name()` 互为反函数。返回 0 = 找到并写入 `*tag_id`；非 0 = 无此点位。
     *
     * 为什么必须由产品实现：名字→标识的映射是点位模型，驱动不认识。
     */
    int (*resolve_name)(void *ctx, const char *name, uint32_t *tag_id);

    /**
     * @brief 收下一条北向下行（控制 / 写点）。
     *
     * **实现方必须走聚合根**（`dcu_node_execute_control` / `iems_node_dispatch_power`
     * 之类），不能直接写影子表 —— 那会绕过五防闭锁与 SOC 包络（架构原则 3）。
     *
     * @return 0 = 已接受；非 0 = 拒绝（会回失败回执）
     */
    int (*accept)(void *ctx, uint32_t tag_id, double value);

    /**
     * @brief 收到一条北向下行**报文**，由产品自行解释（ADR-0011 N6-2 拍板）。
     *
     * 编排层只做"把字节送到"，**不解析载荷**。理由是下行报文的字段语义
     * （这条命令控哪个继电器、值域怎么校验）是产品知识，而且两个协议的
     * 点位位置根本不同：
     *   - 裸 JSON：点位可以在主题里，也可以在 JSON 字段里
     *   - Sparkplug NCMD：点位在 protobuf 的 metric name 里
     * 强行在编排层统一，只会得到一个"两边都不像"的解析器。
     *
     * **分层纪律**：产品可以调用协议层已有的解析函数
     * （如 `dlt698_decode_frame` / `dlt698_decode_data`），
     * 但**不要**把字节解析逻辑复制进 `cmd/` —— 那是协议知识，属于 `foundation/edge_codec`。
     * 产品负责的是"解析出来的字段 → 哪个聚合根方法"。
     *
     * @param address 传输层给的地址（MQTT topic / OPC UA NodeId）
     * @param payload 原始字节，**不做任何假设**
     * @return 0 = 已处理；非 0 = 未识别/拒绝（编排层记日志，不重复投递）
     */
    int (*on_downlink)(void *ctx, const char *address, const uint8_t *payload, size_t len);
};

typedef struct edge_north_source_ops_s edge_north_source_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGE_NORTH_PLANE_H */
