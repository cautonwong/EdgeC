/**
 * @file edge_driver.h
 * @brief 工业级协议栈接口 (V3.0 有状态事务版)
 */

#ifndef EDGE_DRIVER_H
#define EDGE_DRIVER_H

#include <stddef.h>
#include <stdint.h>

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

    // --- 契约注入 (Phase 4: 依赖注入模式) ---
    /** @brief 根据协议地址查找元数据 (注入的哈希查找函数) */
    const void *(*lookup_by_key)(uint64_t key);
    /** @brief 根据 TagID 查找协议地址映射 (注入的哈希查找函数) */
    const void *(*lookup_by_tag)(uint32_t tag_id);

    /** @brief 协议特有的紧凑列表 (如 g_iec104_gi_list) */
    const void *meta_list;
    uint32_t meta_list_len;

    // --- 消费端端口抽象 (Consumer Ports: 隔离外界 DataHub / 消息总线) ---
    int (*read_shadow)(struct edge_driver_ctx_s *ctx, uint32_t tag_id, pipe_msg_t *out_msg);
    int (*publish)(struct edge_driver_ctx_s *ctx, const pipe_msg_t *msg);
    uint32_t (*get_tag_count)(struct edge_driver_ctx_s *ctx);
    const char *(*get_tag_name)(struct edge_driver_ctx_s *ctx, uint32_t tag_id);

} edge_driver_ctx_t;

static inline int edge_driver_read_shadow(edge_driver_ctx_t *ctx, uint32_t tag_id, pipe_msg_t *out_msg) {
    if (ctx && ctx->read_shadow) {
        return ctx->read_shadow(ctx, tag_id, out_msg);
    }
    return -1;
}

static inline int edge_driver_publish(edge_driver_ctx_t *ctx, const pipe_msg_t *msg) {
    if (ctx && ctx->publish) {
        return ctx->publish(ctx, msg);
    }
    return -1;
}

static inline uint32_t edge_driver_get_tag_count(edge_driver_ctx_t *ctx) {
    if (ctx && ctx->get_tag_count) {
        return ctx->get_tag_count(ctx);
    }
    return 0;
}

static inline const char *edge_driver_get_tag_name(edge_driver_ctx_t *ctx, uint32_t tag_id) {
    if (ctx && ctx->get_tag_name) {
        return ctx->get_tag_name(ctx, tag_id);
    }
    return NULL;
}

/**
 * @brief 协议驱动元模型 (V4.0 状态机版)
 */
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