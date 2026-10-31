/**
 * @file edge_driver.h
 * @brief 工业级协议栈接口 (V3.0 有状态事务版)
 */

#ifndef EDGE_DRIVER_H
#define EDGE_DRIVER_H

#include <stddef.h>
#include <stdint.h>

#include "edge_slave_plane.h"
#include "net_buf.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 驱动测点物理与逻辑配置 (Legacy V2)
 */
typedef struct {
    uint32_t tag_id;
    uint32_t reg_addr;
    uint16_t reg_count;
    uint8_t func_code;
    double scaler;
    double offset;
} edge_point_config_t;

typedef struct edge_driver_s edge_driver_t;

/**
 * @brief 协议运行角色
 */
typedef enum {
    PROTOCOL_ROLE_MASTER, /**< 主站/客户端：主动发起请求 */
    PROTOCOL_ROLE_SLAVE,  /**< 从站/服务端：被动响应请求 */
    PROTOCOL_ROLE_PUB_SUB /**< 发布/订阅：如 GOOSE/MQTT */
} protocol_role_t;

/**
 * @brief 协议栈私有上下文 (由管理器自动分配，支持持久化 Session)
 */
typedef struct edge_driver_ctx_s {
    const edge_driver_t *driver; /**< 指向驱动模板的指针 */
    void *priv;                  /**< 协议私有状态内存 */
    protocol_role_t role;        /**< 当前运行角色 */
    uint32_t device_id;          /**< 关联的物理设备 ID */
    void *user_data;             /**< 用户自定义挂载 (通常是 DataHub) */
    net_buf_pool_t *pool; /**< [新增] 关联的内存池 (用于 on_tick 等自主分配) */

    // --- 状态管理 (Session Lifecycle) ---
    uint64_t last_activity_ts; /**< 上次收到或发送数据的时间戳 (ms) */
    int session_state;         /**< 协议特定的会话状态码 */
    uint32_t flags;            /**< 协议运行标志位 */

    // [ADR-002] 这里已经没有任何"驱动反查消费侧"的指针了。
    // 需要外部状态就走端口（edge_slave_source_ops_t / edge_north_source_ops_t），
    // 端口每次调用现传，不进这个结构体。**不要加回来。**
} edge_driver_ctx_t;

/**
 * @brief 协议驱动元模型 (V4.0 状态机版)
 */
/* 请求原生驱动（ADR-002）的私有数据上限。
 * 定长内嵌在请求上下文里，避免为核心路径引入动态内存。
 * 128 是量出来的：Modbus 要装 32 个原始寄存器（64B）+ 头字段，
 * 645 的 dlt645_req_t 只有 24B，CAN 的 can_req_t 只有 24B。 */
#define REQ_DRV_PRIV_MAX 128

struct edge_driver_s {
    const char *name; /**< 协议名称，如 "IEC104" */
    size_t ctx_size;  /**< 协议栈需要的私有上下文大小 */

    // --- 1. 生命周期 ---
    /** @brief 初始化协议栈内部状态 */
    int (*on_init)(edge_driver_ctx_t *ctx);

    /** @brief 销毁协议栈私有资源 */
    void (*on_cleanup)(edge_driver_ctx_t *ctx);

    /** @brief 检查协议是否已就绪 (如 DLMS 是否已完成 AARQ 关联) */
    bool (*is_ready)(edge_driver_ctx_t *ctx);

    /** @brief 协议心跳处理 (用于维持长连接、处理重连或超时) */
    int (*on_tick)(edge_driver_ctx_t *ctx, net_buf_t **out_tx);

    // --- 2. 主站/客户端逻辑 (Master Plane) ---
    /**
     * @brief 构建物理请求包
     */
    int (*encode_request)(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int tag_count,
                          net_buf_t *out);

    /**
     * @brief 构建写操作请求包
     */
    int (*encode_write)(edge_driver_ctx_t *ctx, uint32_t tag_id, double value, net_buf_t *out);

    /**
     * @brief 解析主站收到的响应
     */
    int (*decode_response)(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                           pipe_msg_t *out_msgs, int *msg_count, size_t *consumed);

    // --- 3. 从站/服务端逻辑 (Slave Plane) ---
    /**
     * @brief 处理被动请求并生成响应
     */
    int (*handle_rx)(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                     pipe_msg_t *async_msgs, int *async_count, size_t *consumed);

    // --- 4. 纯编解码 (Request-Native / ADR-002) ---
    //
    // 与上面 1-3 的 legacy 四件套**互斥**：一个驱动要么实现 legacy，要么实现这组，
    // 不得混用（driver_manager_register 会拒绝混用）。
    //
    // 这组接口里没有 edge_driver_ctx_t、没有 tag_ids、没有 pipe_msg_t：
    // 驱动只认"点位"（di/寄存器地址）与字节，不查表、不读影子表、不决定 tag_id。
    // tag_id → 点位 的翻译由请求层完成，解码出的值也由请求层装进 pipe_msg_t。

    /** 驱动私有请求数据的字节数（≤ REQ_DRV_PRIV_MAX）。0 表示走 legacy。 */
    size_t req_priv_size;

    // 两个 priv 的分工（ADR-002 S3-d）：
    //   priv    —— 请求级 scratch，req_priv_size 字节，每次请求清零，只放"这一帧"的东西；
    //   session —— 设备级持久上下文，ctx_size 字节，随会话存活，放"跨帧"的东西
    //              （APCI 收发序号、HDLC/COSEM 关联状态、总召唤是否已发）。
    // session 由请求层从 SessionManager 取来传入，可为 NULL（无状态驱动不看它）。
    // 驱动私有不透明，请求层不解释其内容——这与"驱动不查表"是同一条边界。

    /**
     * @brief 把请求参数填进私有数据。
     * @param priv      由请求层提供的 req_priv_size 字节缓冲（已清零）
     * @param slave_id  从站地址/设备号
     * @param point_id  行业原生点位标识（645 是 DI，Modbus 是寄存器地址）
     * @param meta_list 设备附加信息（645 是 6 字节 BCD 表地址数组，按 slave_id 索引）
     * @param point_desc 本次请求的点位描述（点位标识超过 32 位的协议用，如 DLMS 的
     *                   OBIS+class_id+attr）。**不是表**——调用方给的就是这一个点。
     *                   驱动用 point_desc_len 校验尺寸，不匹配就拒绝。
     * @param is_write   本次是写还是读。**必须传**：native 驱动的 encode 只有一个入口，
     *                   拿不到这个标志就只能一律发读帧——写请求会静默变成读，
     *                   控制命令失效且不报错（ADR-002 S5 修的就是这个）。
     * @param value      写入值（`is_write` 为 true 时有效）。是 **raw**：调用方负责把
     *                   工程量换成寄存器/BCD 原始值，驱动不碰标定。
     */
    int (*req_prepare)(void *priv, uint32_t slave_id, uint32_t point_id, const void *meta_list,
                       uint32_t meta_list_len, const void *point_desc, size_t point_desc_len,
                       bool is_write, double value);

    /**
     * @brief 私有数据 → 请求字节
     * @param priv    本次请求的 scratch
     * @param session 设备级持久上下文（ctx_size 字节，可为 NULL）；有会话状态的协议
     *                （IEC104 的 N(S)、DLMS 的关联状态）从这里读写
     */
    int (*req_encode)(void *priv, void *session, net_buf_t *out);

    /**
     * @brief 应答字节 → 私有数据里的值
     * @param session 设备级持久上下文，同上
     * @param out_value 解出的工程量/原始值，由请求层负责标定与落库
     * @return 0=解出完整帧；EDGE_ERR_INCOMPLETE=未收全；<0=校验/格式错误
     */
    int (*req_decode)(void *priv, void *session, const uint8_t *buf, size_t len, size_t *consumed,
                      double *out_value);

    /**
     * @brief 从站面：解请求 → 向端口取数/收数 → 组应答（ADR-002 S3-0）
     *
     * 与 `handle_rx` 的区别：这里**没有** ctx 查表、没有 read_shadow、没有 publish。
     * "当前值"由 `src->resolve` 给，"写入值"交给 `src->accept`，
     * 驱动只负责字节 ↔ 结构体的转换。端口每次现传，不进 ctx。
     *
     * @param priv      req_priv_size 字节的私有缓冲（scratch，跨帧不必保留）
     * @param session   设备级持久上下文（ctx_size 字节，可为 NULL）。HDLC 帧序号、
     *                  APCI 收发序号这类**跨帧**状态放这里；放 scratch 里每帧归零，
     *                  序号会永远停在 0，对端直接判帧错。
     * @param consumed  本帧消耗的字节数；返回 1 表示跳过了前导垃圾
     */
    int (*req_slave_exchange)(void *priv, void *session, const uint8_t *in, size_t in_len,
                              const edge_slave_source_ops_t *src, void *src_ctx, net_buf_t *out,
                              size_t *consumed);

    /**
     * @brief 周期发帧：北向报文 / 从站自发报文（ADR-002）
     *
     * 与 `on_tick` 的区别：这里没有 `read_shadow`、没有 datahub 句柄。
     * 状态由端口给，驱动不知道背后是什么。
     * `priv` 是**持久**上下文（ctx_size 字节），因为生产者要记序号等状态。
     *
     * 两个端口都要给：北向生产者读 `north`（发北向报文），
     * 从站自发上报读 `slave`（测点变化 → 主动上送）。两者用途不同，不能互替。
     * 任一端口可为 NULL（该方向不需要发帧时）。
     */
    int (*req_tick)(void *priv, const edge_north_source_ops_t *north, void *north_ctx,
                    const edge_slave_source_ops_t *slave, void *slave_ctx, net_buf_pool_t *pool,
                    net_buf_t **out_tx);
};

/**
 * @brief 类型安全的私有上下文转换宏 (解决 C5 UB 风险)
 * 检查 ctx->driver->name 是否与期望的 protocol_name 匹配，匹配才返回 ctx->priv，否则返回 NULL
 */
static inline void *edge_driver_get_priv_safe(edge_driver_ctx_t *ctx, const char *protocol_name) {
    if (!ctx || !ctx->driver || !ctx->driver->name || !protocol_name) {
        return NULL;
    }
    // 允许忽略大小写或精确匹配，这里用精确匹配
    const char *dname = ctx->driver->name;
    // 假设 name 比较短，简单对比
    int i = 0;
    while (dname[i] && protocol_name[i] && dname[i] == protocol_name[i]) {
        i++;
    }
    if (dname[i] == '\0' && protocol_name[i] == '\0') {
        return ctx->priv;
    }
    return NULL;
}

#define EDGE_DRIVER_GET_PRIV(type, ctx, name_str) \
    ((type *)edge_driver_get_priv_safe((ctx), (name_str)))

/**
 * @brief 契约生成式驱动注册表项 (AEP-301)
 */
typedef struct {
    const char *protocol_name;
    const edge_driver_t *driver;
} edge_driver_reg_entry_t;

#define EDGE_DRIVER_ENTRY(proto, drv) { .protocol_name = (proto), .driver = &(drv) }

#ifdef __cplusplus
}
#endif

#endif