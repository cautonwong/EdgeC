/**
 * @file snf_manager.h
 * @brief Store-and-Forward (SnF) 管理器接口
 */

#ifndef SNF_MANAGER_H
#define SNF_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "shmring.h"
#include "sqlite_service.h"

typedef struct snf_manager_s snf_manager_t;

/**
 * @brief SnF 事件回调 (用于上层同步数据)
 */
typedef int (*snf_publish_cb)(void *user_data, const PipeMsg *msg);

typedef struct {
    sqlite_svc_t *db;
    uv_loop_t *loop;
    snf_publish_cb publish_fn;
    void *user_data;
    uint32_t sync_batch_size;
    uint32_t sync_interval_ms;
} snf_config_t;

/**
 * @brief 创建 SnF 管理器
 */
snf_manager_t *snf_manager_create(const snf_config_t *cfg);

/**
 * @brief 更新连接状态 (触发或暂停转发)
 */
void snf_manager_set_online(snf_manager_t *mgr, bool online);

/**
 * @brief 处理离线缓存 (当 MQTT 断开时，直接写入 DB)
 */
int snf_manager_buffer_data(snf_manager_t *mgr, const PipeMsg *msg);

/**
 * @brief 销毁管理器
 */
void snf_manager_destroy(snf_manager_t *mgr);

#endif // SNF_MANAGER_H
