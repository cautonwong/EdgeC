#ifndef EDGE_SERVICE_H
#define EDGE_SERVICE_H

#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "net_buf.h"
#include "northbound_mgr.h"
#include "persistence_mgr.h"
#include "profile_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 不透明的服务容器句柄
 */
typedef struct service_s service_t;

/**
 * @brief 命令行选项
 */
typedef struct {
    char config_path[256];
    char mode[32];
    int web_port;
    int rest_port;
} service_options_t;

/**
 * @brief 创建服务实例 (黑盒)
 */
service_t *service_new(service_options_t *opts);

/**
 * @brief 注入事件循环 (用于在 run 之前初始化依赖 loop 的组件)
 */
void service_set_loop(service_t *svc, uv_loop_t *loop);

/**
 * @brief 获取服务容器的事件循环
 */
uv_loop_t *service_get_loop(service_t *svc);

/**
 * @brief 获取数据中枢的环形缓冲区指针
 */
shm_ring_t *service_get_ring(service_t *svc);
shm_ring_t *service_get_shmring(service_t *svc);  // 别名，对齐 rest_server.c

/**
 * @brief 获取仲裁器实例指针
 */
arbiter_t *service_get_arbiter(service_t *svc);

/**
 * @brief 获取数据中枢实例指针
 */
datahub_t *service_get_datahub(service_t *svc);

/**
 * @brief 获取持久化存储服务句柄
 */
sqlite_svc_t *service_get_storage(service_t *svc);

/**
 * @brief 启动服务容器并注入依赖 (黑盒模式)
 */
int service_run(service_t *svc, uv_loop_t *loop, net_buf_pool_t *pool, arbiter_t *arbiter,
                datahub_t *hub, persistence_mgr_t *storage, northbound_mgr_t *cloud);

/**
 * @brief 从外部（如 REST 线程）投递指令
 */
typedef enum {
    SVC_CMD_WRITE = 0,
    SVC_CMD_READ_ON_DEMAND,
    SVC_CMD_OTA_START,
    SVC_CMD_OTA_END
} service_cmd_type_t;

typedef struct {
    service_cmd_type_t type;
    uint32_t tag_id;
    double value;
    void *data;
} service_cmd_t;

int service_post_cmd(service_t *svc, service_cmd_t *cmd);

/**
 * @brief 销毁服务实例
 */
void service_delete(service_t *svc);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_SERVICE_H
