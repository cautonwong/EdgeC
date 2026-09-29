#ifndef PERSISTENCE_MGR_H
#define PERSISTENCE_MGR_H

#include <stddef.h>
#include <stdalign.h>
#include <stdint.h>
#include <uv.h>

#include "shmring.h"
#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct persistence_mgr persistence_mgr_t;

/**
 * @brief 持久化管理器调用方预分配存储容器 (Caller-Allocated Storage - AEP-752)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[512];
} persistence_mgr_storage_t;

/**
 * @brief 在调用方提供的内存就地构造持久化管理器聚合根 (Zero-Alloc)
 * @param store 调用方分配的存储块
 * @param loop libuv 事件循环
 * @param ring 共享内存环形缓冲区
 * @param interval_ms 定时刷盘间隔
 * @return 构造成功的聚合根指针, 失败返回 NULL
 */
persistence_mgr_t *persistence_mgr_create_in(persistence_mgr_storage_t *store,
                                             uv_loop_t *loop,
                                             shm_ring_t *ring,
                                             uint64_t interval_ms);

// 初始化持久化管理器
int persistence_mgr_init(persistence_mgr_t *mgr, uv_loop_t *loop, shm_ring_t *ring,
                         uint64_t interval_ms);

// 设置 SQLite 写入器
void persistence_mgr_set_writer(persistence_mgr_t *mgr, sqlite_svc_t *writer);

// 关闭持久化管理器
void persistence_mgr_close(persistence_mgr_t *mgr);

#ifdef __cplusplus
}
#endif

#endif
