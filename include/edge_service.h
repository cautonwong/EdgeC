#ifndef EDGE_SERVICE_H
#define EDGE_SERVICE_H

#include <uv.h>
#include "core/net_buf.h"
#include "arbiter.h"
#include "datahub.h"
#include "persistence_mgr.h"
#include "northbound_mgr.h"
#include "sys_supervisor.h"
#include "profile_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

// Command Bridge types
typedef enum {
    SVC_CMD_WRITE = 0,
    SVC_CMD_READ_ON_DEMAND,
    SVC_CMD_OTA_START,
    SVC_CMD_OTA_END
} service_cmd_type_t;

typedef struct {
    service_cmd_type_t type;
    uint32_t tag_id;
    double   value;
    void     *data;
} service_cmd_t;

// 命令行选项解析结果
typedef struct {
    char config_path[256];
    char mode[32];
    int web_port;
    int rest_port;
} service_options_t;

// 大一统 Service 容器 (Runtime Container)
typedef struct service_s {
    uv_loop_t *loop;
    service_options_t opts;

    // 以下组件由具体的 App (二开者) 初始化后注入
    net_buf_pool_t    *pool;
    arbiter_t         *arbiter;
    datahub_t         *data_hub;
    persistence_mgr_t *storage;
    northbound_mgr_t  *cloud;
    sys_supervisor_t  *supervisor;
    
    device_config_t   *config;

    // Command Bridge
    uv_async_t        cmd_async;
    uv_mutex_t        cmd_lock;
    service_cmd_t     cmd_queue[16];
    int               cmd_head, cmd_tail;

    // 二开私有上下文
    void *product_ctx;

    // 状态标志
    int shutting_down;
} service_t;

/**
 * @brief 启动服务容器
 */
int service_start(service_t *svc);

/**
 * @brief 停止服务容器
 */
void service_stop(service_t *svc);

/**
 * @brief 从其他线程发送指令到服务主循环
 */
int service_post_cmd(service_t *svc, service_cmd_t *cmd);

#ifdef __cplusplus
}
#endif

#endif // EDGE_SERVICE_H