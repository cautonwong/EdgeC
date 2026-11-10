/**
 * @file arbiter_types.h
 * @brief Pure command/arbiter value types shared without edge_core linkage.
 */
#ifndef ARBITER_TYPES_H
#define ARBITER_TYPES_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct edge_req_s edge_req_t;

#define ARBITER_MAX_BUSES 64
#define ARBITER_DEFAULT_TIMEOUT_MS 1500
#define ARBITER_MAX_QUEUE_PER_BUS  128

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
    CMD_STATUS_QUEUE_FULL = -4,
    CMD_STATUS_RATE_LIMITED = -5
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
    uint64_t deadline_ns;

    void (*on_start)(struct cmd_msg *msg);
    void (*on_complete)(struct cmd_msg *msg, int status);
    void (*on_cleanup)(struct cmd_msg *msg); // 恢复清理回调
    struct cmd_msg *next;
} cmd_msg_t;

typedef struct {
    int bus_id;
    bool is_busy;
    int queue_len;
    uint64_t dropped_count;
} arbiter_bus_status_t;

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

#ifdef __cplusplus
}
#endif

#endif /* ARBITER_TYPES_H */
