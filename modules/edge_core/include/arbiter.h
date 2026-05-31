#ifndef ARBITER_H
#define ARBITER_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "edge_common.h"
#include "edge_request.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARBITER_MAX_BUSES 64
#define ARBITER_DEFAULT_TIMEOUT_MS 1500

typedef enum {
    ARBITER_PRIO_LOW = 0,
    ARBITER_PRIO_NORMAL = 50,
    ARBITER_PRIO_HIGH = 100,
    ARBITER_PRIO_EMERGENCY = 200
} cmd_priority_t;

typedef enum {
    CMD_STATUS_SUCCESS = 0,
    CMD_STATUS_IO_ERROR = -1,
    CMD_STATUS_PROTO_ERROR = -2,
    CMD_STATUS_TIMEOUT = -3,
} cmd_status_t;

typedef enum {
    CMD_ACTION_READ,
    CMD_ACTION_WRITE,
    CMD_ACTION_RAW,
    CMD_ACTION_OTA,
    CMD_ACTION_POLL
} cmd_action_t;

typedef struct cmd_msg {
    char req_id[64];
    char protocol[16];
    uint8_t priority;
    cmd_action_t action;

    uint32_t tag_id;
    uint32_t device_id; // [Nexus-Pro] 实例 ID 支持
    double value;
    
    edge_req_t *req; // 【修正】对齐 edge_request.h 的类型名
    void *params;       
    void *user_data;    

    uint32_t signature;
    uint32_t timeout_ms;
    uint64_t submit_ts;

    void (*on_start)(void *user);
    void (*on_complete)(struct cmd_msg *msg, int status);
    void (*on_cleanup)(struct cmd_msg *msg); // 恢复清理回调
    struct cmd_msg *next;
} cmd_msg_t;

typedef struct {
    int bus_id;
    bool is_busy;
    int queue_len;
} arbiter_bus_status_t;

typedef struct arbiter arbiter_t;

int arbiter_init(arbiter_t **arb, uv_loop_t *loop);
void arbiter_destroy(arbiter_t *arb);
void arbiter_pause(arbiter_t *arb);
void arbiter_resume(arbiter_t *arb);
void *arbiter_bus_register(arbiter_t *arb, const char *name);
int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd);
void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *msg, int status);
int arbiter_get_statuses(arbiter_t *arb, arbiter_bus_status_t *out_stats, int max_count);
void arbiter_set_exclusive(arbiter_t *arb, int bus_id, bool exclusive);

// --- 辅助初始化函数 (Helper Init Functions) ---
static inline void cmd_msg_init_read(cmd_msg_t *msg, uint32_t tag_id, uint8_t priority) {
    memset(msg, 0, sizeof(*msg));
    msg->action = CMD_ACTION_READ;
    msg->tag_id = tag_id;
    msg->priority = priority;
    msg->timeout_ms = ARBITER_DEFAULT_TIMEOUT_MS;
}

static inline void cmd_msg_init_write(cmd_msg_t *msg, uint32_t tag_id, double val, uint8_t priority) {
    memset(msg, 0, sizeof(*msg));
    msg->action = CMD_ACTION_WRITE;
    msg->tag_id = tag_id;
    msg->value = val;
    msg->priority = priority;
    msg->timeout_ms = ARBITER_DEFAULT_TIMEOUT_MS;
}

// 架构补强点
void arbiter_set_orchestrator(arbiter_t *arb, void *orch);
void *arbiter_get_orchestrator(arbiter_t *arb);

#ifdef __cplusplus
}
#endif

#endif
