#ifndef ARBITER_H
#define ARBITER_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "edge_common.h"
#include "io_executor.h"

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

// --- S18: 统一优先级定义 (与 PipeMsgAttr 对齐) ---
// 值越大优先级越高
#define ARBITER_PRIO_LOW 10
#define ARBITER_PRIO_NORMAL 50
#define ARBITER_PRIO_HIGH 150
#define ARBITER_PRIO_EMERGENCY 255

typedef uint8_t cmd_priority_t;

/* 向后兼容别名 */
enum {
    CMD_PRIO_EMERGENCY = ARBITER_PRIO_EMERGENCY,
    CMD_PRIO_HIGH = ARBITER_PRIO_HIGH,
    CMD_PRIO_NORMAL = ARBITER_PRIO_NORMAL
};

typedef enum {
    CMD_ACTION_READ,
    CMD_ACTION_WRITE,
    CMD_ACTION_RAW,
    CMD_ACTION_OTA,
    CMD_ACTION_POLL
} cmd_action_t;

#define ARBITER_DEFAULT_TIMEOUT_MS 1000

typedef struct cmd_msg {
    char req_id[32];
    char protocol[16]; /**< [Phase 4] 目标协议名 (如 "ModbusRTU") */
    cmd_priority_t priority;
    cmd_action_t action;

    uint32_t tag_id;
    double value;
    void *params;
    edge_phy_params_t *phy_params;

    uint32_t signature;  /**< S20: Command security signature/fingerprint */
    uint32_t timeout_ms; /**< [补强] 任务执行超时时间 (0 表示使用默认值) */

    // Callbacks
    void (*on_start)(void *user);
    void (*on_preempt)(void *user);
    void (*on_complete)(struct cmd_msg *msg, int status);
    void (*on_cleanup)(struct cmd_msg *msg); /**< [新增] 用于物理释放 params 等私有数据 */
    void *user_data;

    /* 内部链表 */
    struct cmd_msg *next;
} cmd_msg_t;

// --- Builder API (New) ---

static inline void cmd_msg_init_write(cmd_msg_t *cmd, uint32_t tag, double val,
                                      cmd_priority_t prio) {
    *cmd = (cmd_msg_t){0};
    cmd->action = CMD_ACTION_WRITE;
    cmd->tag_id = tag;
    cmd->value = val;
    cmd->priority = prio;
}

static inline void cmd_msg_init_read(cmd_msg_t *cmd, uint32_t tag, cmd_priority_t prio) {
    *cmd = (cmd_msg_t){0};
    cmd->action = CMD_ACTION_READ;
    cmd->tag_id = tag;
    cmd->priority = prio;
}

typedef struct arbiter arbiter_t;

/**
 * @brief 初始化仲裁器
 */
int arbiter_init(arbiter_t **arb, uv_loop_t *loop);

/**
 * @brief 关闭仲裁器
 */
void arbiter_shutdown(arbiter_t *arb);
void arbiter_destroy(arbiter_t *arb);

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

/**
 * @brief 销毁仲裁器并回收内存
 */
void arbiter_destroy(arbiter_t *arb);

#ifdef __cplusplus
}
#endif

#endif  // ARBITER_H
