#ifndef EDGE_EVENT_H
#define EDGE_EVENT_H

#include <stdint.h>

#include "shmring.h"

// 工业系统核心事件 ID
typedef enum {
    SYS_EVENT_NONE = 0,
    SYS_EVENT_TIME_CHANGED,  // Scenario 8
    SYS_EVENT_BUS_FAULT,
    SYS_EVENT_APP_READY,
    SYS_EVENT_ALARM_CRITICAL,
    SYS_EVENT_USER_CUSTOM = 100
} edge_event_id_t;

// 发布事件到 DataHub
static inline void edge_event_publish(shm_ring_t *ring, edge_event_id_t eid, uint32_t param) {
    PipeMsg msg = {0};
    msg.type = PIPEMSG_EVENT;
    msg.payload.event.event_id = (uint32_t)eid;
    msg.payload.event.param = param;
    msg.ts = uv_now(uv_default_loop());
    shm_ring_write(ring, &msg);
}

#endif
