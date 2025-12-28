#ifndef ARBITER_H
#define ARBITER_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Bus Types ===== */
typedef enum {
    BUS_RS485_1 = 0,
    BUS_RS485_2,
    BUS_RS485_3,
    BUS_HPLC,
    BUS_CAN,
    BUS_MAX
} arbiter_bus_t;

// --- 4.2.2 控制指令包 (CmdMsg) ---
typedef enum { CMD_PRIO_EMERGENCY = 0, CMD_PRIO_HIGH = 1, CMD_PRIO_NORMAL = 2 } cmd_priority_t;

typedef enum { CMD_ACTION_READ, CMD_ACTION_WRITE, CMD_ACTION_RAW, CMD_ACTION_OTA } cmd_action_t;

typedef struct cmd_msg {
    char req_id[32];
    cmd_priority_t priority;
    cmd_action_t action;

    uint32_t tag_id;
    double value;
    void *params;

    // Callbacks
    void (*on_start)(void *user);
    void (*on_preempt)(void *user);
    void (*on_complete)(struct cmd_msg *msg, int status);
    void *user_data;

    /* 内部链表 */
    struct cmd_msg *next;
} cmd_msg_t;

typedef struct arbiter arbiter_t;

/**
 * @brief 初始化仲裁器
 */
int arbiter_init(arbiter_t **arb, uv_loop_t *loop);

/**
 * @brief 关闭仲裁器
 */
void arbiter_shutdown(arbiter_t *arb);

/**
 * @brief 注册总线句柄 (Opaque)
 */
typedef void *arbiter_bus_handle_t;
arbiter_bus_handle_t arbiter_bus_register(arbiter_t *arb, const char *name);

typedef struct {
    int bus_id;
    bool is_busy;
    bool is_exclusive;
    int queue_len;
    char current_job_id[32];
} arbiter_bus_status_t;

int arbiter_get_statuses(arbiter_t *arb, arbiter_bus_status_t *out_stats, int max_count);

/**
 * @brief 提交指令 (S5/S6/S7 场景)
 */
int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd);

/**
 * @brief 设置独占锁 (S7 OTA 场景)
 */
void arbiter_set_exclusive(arbiter_t *arb, int bus_id, bool exclusive);

/**
 * @brief 驱动层回调：任务完成
 */
void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *cmd);

#ifdef __cplusplus
}
#endif

#endif  // ARBITER_H
