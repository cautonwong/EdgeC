#ifndef SYS_SUPERVISOR_H
#define SYS_SUPERVISOR_H

#include <uv.h>
#include "shmring.h"

typedef struct {
    uv_loop_t *loop;
    shm_ring_t *ring;
    uv_poll_t tfd_poll;
    int tfd;
} sys_supervisor_t;

// 初始化系统监护人：监听时间跳变 (Scenario 8) 和 电源状态 (Scenario 10)
int sys_supervisor_init(sys_supervisor_t *sv, uv_loop_t *loop, shm_ring_t *ring);

void sys_supervisor_close(sys_supervisor_t *sv);

#endif
