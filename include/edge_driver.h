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

/**
 * @brief 轮询分组配置 (Legacy V2)
 */
typedef struct {
    uint16_t start_addr;
    uint16_t count;
    uint8_t func_code;
    uint32_t slave_id;
} edge_poll_group_t;

/**
 * @brief 协议运行角色
 */
typedef enum {
    PROTOCOL_ROLE_MASTER, /**< 主站/客户端：主动发起请求 */
    PROTOCOL_ROLE_SLAVE,  /**< 从站/服务端：被动响应请求 */
    PROTOCOL_ROLE_PUB_SUB /**< 发布/订阅：如 GOOSE/MQTT */
} protocol_role_t;

typedef struct edge_driver_s edge_driver_t;

/**
 * @brief 协议栈私有上下文 (由管理器自动分配)
 */
typedef struct {
    edge_driver_t *driver; /**< [新增] 指向驱动模板的指针 */
    void *priv;            /**< 协议私有状态内存 */
    protocol_role_t role;  /**< 当前运行角色 */
    uint32_t device_id;    /**< 关联的物理设备 ID */
    void *user_data;       /**< 用户自定义挂载 */
} edge_driver_ctx_t;

/**
 * @brief 协议驱动元模型
 */
struct edge_driver_s {
    const char *name; /**< 协议名称，如 "IEC104" */
    size_t ctx_size;  /**< 协议栈需要的私有上下文大小 */

    // --- 1. 生命周期 ---
    /** @brief 初始化协议栈内部状态 (如初始化序列号) */
    int (*on_init)(edge_driver_ctx_t *ctx);

    /** @brief 协议心跳处理 (用于维持长连接或处理超时) */
    int (*on_tick)(edge_driver_ctx_t *ctx, net_buf_t **out_tx);

    // --- 2. 主站/客户端逻辑 (Master Plane) ---
    /**
     * @brief 构建物理请求包
     * @param tags 需要读取的点位列表
     * @param out 填充目标缓冲区
     */
    int (*encode_request)(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int tag_count,
                          net_buf_t *out);

    /**
     * @brief 解析主站收到的响应
     * @param in 接收到的原始数据 (net_buf 支持链式)
     * @param out_msgs 解析出的逻辑点位数据，用于注入 DataHub
     */
    int (*decode_response)(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs,
                           int *msg_count);

    // --- 3. 响应与突发处理 (Reactive Plane) ---
    /**
     * @brief 统一接收处理入口
     * @param in 收到的原始字节
     * @param resp_out 如果是作为 Slave，在此填充响应报文
     * @param async_msgs 如果是收到突发数据 (Spontaneous)，在此填充点位数据
     */
    int (*handle_rx)(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                     pipe_msg_t *async_msgs, int *async_count);

    /** @brief 资源回收 */
    void (*on_cleanup)(edge_driver_ctx_t *ctx);
};

typedef struct edge_driver_s edge_driver_t;

#ifdef __cplusplus
}
#endif

#endif