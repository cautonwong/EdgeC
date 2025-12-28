#ifndef PERSISTENCE_MGR_H
#define PERSISTENCE_MGR_H

#include <uv.h>

#include "shmring.h"
#include "sqlite_service.h"

typedef struct {
    shm_ring_t *ring;

    uv_timer_t timer;

    uint64_t last_seq;

    uv_loop_t *loop;

    sqlite_svc_t *writer;

} persistence_mgr_t;

// 初始化持久化管理器
int persistence_mgr_init(persistence_mgr_t *mgr, uv_loop_t *loop, shm_ring_t *ring,
                         uint64_t interval_ms);

// 关闭持久化管理器
void persistence_mgr_close(persistence_mgr_t *mgr);

#endif