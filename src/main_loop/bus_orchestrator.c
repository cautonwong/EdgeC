#include "bus_orchestrator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver_manager.h"
#include "edge_log.h"
#include "shmring.h"

// 轮询任务封装 (V3 事务版)
typedef struct poll_task_s {
    uv_timer_t timer;
    struct bus_orchestrator_s *parent;
    uint32_t tag_id;            /**< 目标点位 */
    uint32_t device_id;         /**< 目标设备物理 ID */
    edge_driver_ctx_t *drv_ctx; /**< 有状态驱动实例 */
    struct poll_task_s *next;
} poll_task_t;

struct bus_orchestrator_s {
    datahub_t *hub;
    arbiter_t *arb;
    uv_loop_t *loop;
    net_buf_pool_t pool; /**< 专用的报文内存池 */
    poll_task_t *tasks;
    int bus_fds[8]; /**< 物理总线 FD 映射表 */
    int is_running;
};

// 事务包装器 (用于在 Arbiter 回调中保持上下文)
typedef struct {
    cmd_msg_t base;
    poll_task_t *task;
} v3_poll_cmd_t;

// --- 回调逻辑 ---

static void _on_v3_cmd_cleanup(cmd_msg_t *cmd) {
    free(cmd);
}

static void _on_v3_cmd_start(void *user_data) {
    v3_poll_cmd_t *pcmd = (v3_poll_cmd_t *)user_data;
    poll_task_t *task = pcmd->task;
    bus_orchestrator_t *ctx = task->parent;
    edge_driver_ctx_t *drv = task->drv_ctx;

    // 1. 从池中分配缓冲区
    net_buf_t *tx_buf = net_buf_alloc(&ctx->pool, 0);
    if (!tx_buf) goto done;

    // 2. 协议组包
    if (drv->driver->encode_request(drv, &task->tag_id, 1, tx_buf) == 0) {
        LOG_INFO(LOG_CAT_BUS, "[V3 Arbiter] Dispatching %s to Device %u", drv->driver->name,
                 drv->device_id);

        // [核心改进] 执行真实物理 I/O (同步模拟版，后续可升级为 uv_poll 非阻塞)
        // 这里的 FD 是在添加任务时关联的，对应真实的串口或仿真 Socket
        int fd = ctx->bus_fds[0];  // 简化处理：假设所有任务走 Bus 0
        if (fd >= 0) {
            // 物理写
            write(fd, tx_buf->b.data, tx_buf->b.len);

            // 给模拟器一点时间响应
            usleep(100000);  // 100ms 宽限期

            net_buf_t *rx_buf = net_buf_alloc(&ctx->pool, 0);
            if (rx_buf) {
                ssize_t n = read(fd, rx_buf->b.data, rx_buf->b.size);
                if (n > 0) {
                    rx_buf->b.len = (uint16_t)n;
                    LOG_INFO(LOG_CAT_BUS, "Received %ld bytes from device", (long)n);

                    pipe_msg_t msgs[16];
                    int count = 0;
                    if (drv->driver->decode_response(drv, rx_buf, msgs, &count) == 0) {
                        for (int i = 0; i < count; i++) {
                            datahub_publish(ctx->hub, &msgs[i]);
                        }
                    } else {
                        LOG_WARN(LOG_CAT_BUS, "Protocol decode FAILED for %s", drv->driver->name);
                    }
                }
                net_buf_unref(rx_buf);
            }
        }
    }
    net_buf_unref(tx_buf);

done:
    arbiter_on_io_done(ctx->arb, 0, &pcmd->base);
}

static void _on_v3_poll_timer(uv_timer_t *handle) {
    poll_task_t *task = (poll_task_t *)handle->data;
    bus_orchestrator_t *ctx = task->parent;
    if (!ctx->is_running) return;

    v3_poll_cmd_t *pcmd = calloc(1, sizeof(v3_poll_cmd_t));
    pcmd->task = task;
    pcmd->base.priority = ARBITER_PRIO_HIGH;
    pcmd->base.on_start = _on_v3_cmd_start;
    pcmd->base.on_cleanup = _on_v3_cmd_cleanup;
    pcmd->base.user_data = pcmd;

    arbiter_submit_cmd(ctx->arb, 0, &pcmd->base);
}

// --- 公开 API ---

bus_orchestrator_t *bus_orchestrator_create(edge_driver_t *drv_tpl, datahub_t *hub,
                                            arbiter_t *arb) {
    (void)drv_tpl;
    bus_orchestrator_t *ctx = calloc(1, sizeof(struct bus_orchestrator_s));
    ctx->hub = hub;
    ctx->arb = arb;
    ctx->loop = hub->loop;

    for (int i = 0; i < 8; i++) ctx->bus_fds[i] = -1;

    net_buf_pool_init(&ctx->pool, 16, 512, 0, NULL, NULL);
    return ctx;
}

void bus_orchestrator_attach_fd(bus_orchestrator_t *ctx, int bus_id, int fd) {
    if (ctx && bus_id < 8) ctx->bus_fds[bus_id] = fd;
}

int bus_orchestrator_add_poll_v3(bus_orchestrator_t *ctx, const char *protocol, uint32_t device_id,
                                 uint32_t tag_id, int interval_ms) {
    poll_task_t *task = calloc(1, sizeof(poll_task_t));
    task->parent = ctx;
    task->tag_id = tag_id;
    task->device_id = device_id;

    // 创建驱动实例 (V3 核心：有状态上下文)
    task->drv_ctx = driver_instance_create(protocol, device_id);
    if (!task->drv_ctx) {
        free(task);
        return -1;
    }

    task->next = ctx->tasks;
    ctx->tasks = task;

    uv_timer_init(ctx->loop, &task->timer);
    task->timer.data = task;
    uv_timer_start(&task->timer, _on_v3_poll_timer, (uint64_t)interval_ms, (uint64_t)interval_ms);

    LOG_INFO(LOG_CAT_BUS, "V3 Polling added: Device %u (%s) for Tag %u every %dms", device_id,
             protocol, tag_id, interval_ms);
    return 0;
}

// 兼容性包装 (为了不破坏 iems/main.c)
int bus_orchestrator_add_poll(bus_orchestrator_t *ctx, int interval_ms,
                              const edge_poll_group_t *group) {
    return bus_orchestrator_add_poll_v3(ctx, "DLT645-2007", group->slave_id, group->start_addr,
                                        interval_ms);
}

void bus_orchestrator_run(bus_orchestrator_t *ctx) {
    if (ctx) ctx->is_running = 1;
}

void bus_orchestrator_destroy(bus_orchestrator_t *ctx) {
    if (!ctx) return;
    ctx->is_running = 0;

    poll_task_t *t = ctx->tasks;
    while (t) {
        poll_task_t *next = t->next;
        uv_timer_stop(&t->timer);
        driver_instance_destroy(t->drv_ctx, "DLT645-2007");  // 简化：实际应记录 protocol 名
        uv_close((uv_handle_t *)&t->timer, (uv_close_cb)free);
        t = next;
    }
    net_buf_pool_destroy(&ctx->pool);
    free(ctx);
}
