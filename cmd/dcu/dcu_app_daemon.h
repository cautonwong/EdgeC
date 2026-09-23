/**
 * @file dcu_app_daemon.h
 * @brief Production SGCC/CSG Microservices Container Lifecycle & Resource Quota Daemon Engine
 */

#ifndef DCU_APP_DAEMON_H
#define DCU_APP_DAEMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DCU_MAX_CONTAINER_APPS 16

typedef enum {
    APP_STATE_UNINSTALLED = 0,
    APP_STATE_INSTALLED = 1,
    APP_STATE_RUNNING = 2,
    APP_STATE_PAUSED = 3,
    APP_STATE_STOPPED = 4,
    APP_STATE_FAULT = 5
} dcu_app_state_t;

typedef struct {
    uint8_t max_cpu_pct;       // 最大允许 CPU 占用 (e.g. 20%)
    uint32_t max_mem_bytes;    // 最大允许内存占用 (e.g. 64 * 1024 * 1024)
    uint32_t max_flash_bytes;  // 最大允许磁盘占用 (e.g. 32 * 1024 * 1024)
} dcu_app_quota_t;

typedef struct {
    char app_name[32];
    char app_version[16];
    uint32_t app_id;
    dcu_app_state_t state;
    dcu_app_quota_t quota;

    // 实时资源占用
    double current_cpu_pct;
    uint32_t current_mem_bytes;
    uint32_t current_flash_bytes;

    uint64_t last_heartbeat_ms;
    uint32_t restart_count;
    bool is_quota_exceeded;
} dcu_app_container_t;

typedef struct {
    dcu_app_container_t apps[DCU_MAX_CONTAINER_APPS];
    uint32_t app_count;
    uint64_t daemon_start_ms;
} dcu_app_daemon_t;

/**
 * @brief 初始化国网/南网微服务容器守护引擎
 */
void dcu_app_daemon_init(dcu_app_daemon_t* daemon, uint64_t now_ms);

/**
 * @brief 安装并注册一个新微服务 APP
 */
int dcu_app_daemon_install(dcu_app_daemon_t* daemon, const char* name, const char* ver,
                           uint32_t app_id, const dcu_app_quota_t* quota);

/**
 * @brief 启动 APP 容器
 */
int dcu_app_daemon_start(dcu_app_daemon_t* daemon, uint32_t app_id, uint64_t now_ms);

/**
 * @brief 停止 APP 容器
 */
int dcu_app_daemon_stop(dcu_app_daemon_t* daemon, uint32_t app_id);

/**
 * @brief APP 心跳保活与资源占用采样上报
 */
int dcu_app_daemon_heartbeat(dcu_app_daemon_t* daemon, uint32_t app_id, double cpu_pct,
                             uint32_t mem_bytes, uint64_t now_ms);

/**
 * @brief 守护巡检: 检查心跳超时与资源超额强杀/自愈重启 (Watchdog Routine)
 */
int dcu_app_daemon_watchdog_poll(dcu_app_daemon_t* daemon, uint64_t timeout_threshold_ms,
                                 uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif  // DCU_APP_DAEMON_H
