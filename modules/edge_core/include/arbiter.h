#ifndef ARBITER_H
#define ARBITER_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct uv_loop_s uv_loop_t;

#include "edge_common.h"
#include "edge_request.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * @brief [Issue #384] 调用方预分配仲裁器存储 (Caller-Allocated Storage)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[4096];
} arbiter_storage_t;

typedef struct arbiter arbiter_t;

/**
 * @brief 在调用方预分配内存中初始化仲裁器 (Zero-Alloc)
 */
arbiter_t *arbiter_create_in(arbiter_storage_t *storage, uv_loop_t *loop);

int arbiter_init(arbiter_t **arb, uv_loop_t *loop);
void arbiter_destroy(arbiter_t *arb);
void arbiter_pause(arbiter_t *arb);
void arbiter_resume(arbiter_t *arb);
void *arbiter_bus_register(arbiter_t *arb, const char *name);
int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd);
void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *msg, int status);
int arbiter_get_statuses(arbiter_t *arb, arbiter_bus_status_t *out_stats, int max_count);
void arbiter_set_exclusive(arbiter_t *arb, int bus_id, bool exclusive);
void arbiter_set_bus_queue_limit(arbiter_t *arb, int bus_id, uint32_t max_queue);
uint64_t arbiter_get_bus_dropped_count(arbiter_t *arb, int bus_id);
int arbiter_check_timeouts(arbiter_t *arb);

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


// 💡 [Consumer-Driven] 执行器抽象：Arbiter 不再直接依赖 io_executor
typedef struct {
    void *ctx;
    /** 执行指令。返回 0=成功, <0=错误 */
    int (*execute)(void *ctx, int bus_id, cmd_msg_t *cmd);
    /** IO 完成通知。返回 0=已处理继续调度, 1=未处理 */
    int (*on_io_done)(void *ctx, int bus_id, cmd_msg_t *msg, int status);
} arb_executor_ops_t;

/**
 * @brief 安装执行器钩子
 * @note 按字段打补丁:只覆盖非 NULL 的字段。执行器可能由别的层先装过,
 *       整份覆盖会把它装的 execute/ctx 一起抹掉。
 */
void arbiter_set_executor(arbiter_t *arb, arb_executor_ops_t ops);

/**
 * @brief 这个 cmd 现在还是该总线的 owner 吗(只比较指针,不解引用)
 *
 * 引擎持有 current_cmd 期间,arbiter_check_timeouts 可能已经把这条件结算并
 * free 掉;引擎的超时/错误回调必须先问这一句,再决定要不要碰 cmd。
 */
bool arbiter_owns_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd);

#ifdef __cplusplus
}
#endif

#endif
