#include "bus_orchestrator.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arbiter.h"
#include "driver_manager.h"
#include "edge_log.h"

typedef struct poll_task_s poll_task_t;

struct bus_orchestrator_s {
    datahub_t *hub;
    arbiter_t *arb;
    uv_loop_t *loop;
    net_buf_pool_t pool;
    int max_buses;
    int *bus_fds;
    uv_poll_t *bus_polls;
    poll_task_t **current_tasks;
};

struct poll_task_s {
    bus_orchestrator_t *parent;
    edge_driver_ctx_t *drv_ctx;
    uint32_t tag_id;
    int bus_id;
    uv_timer_t poll_timer;
    uv_timer_t io_watchdog;
    cmd_msg_t msg;
};

static void _release_bus(poll_task_t *task) {
    if (!task) return;
    uv_poll_stop(&task->parent->bus_polls[task->bus_id]);
    uv_timer_stop(&task->io_watchdog);
    task->parent->current_tasks[task->bus_id] = NULL;
    arbiter_on_io_done(task->parent->arb, task->bus_id, &task->msg);
}

static void _on_io_timeout(uv_timer_t *handle) {
    poll_task_t *task = (poll_task_t *)handle->data;
    LOG_WARN(LOG_CAT_BUS, "Bus %d: Task TIMEOUT. Releasing...", task->bus_id);
    _release_bus(task);
}

static void _on_v3_poll_readable(uv_poll_t *handle, int status, int events) {
    (void)events;
    poll_task_t *task = (poll_task_t *)handle->data;
    if (status < 0) {
        _release_bus(task);
        return;
    }

    net_buf_t *rx_buf = net_buf_alloc(&task->parent->pool, 0);
    if (!rx_buf) {
        _release_bus(task);
        return;
    }

    ssize_t n = read(task->parent->bus_fds[task->bus_id], rx_buf->b.data, rx_buf->b.size);
    if (n > 0) {
        rx_buf->b.len = (uint16_t)n;
        pipe_msg_t out_msgs[4];
        int count = 0;
        int ret = task->drv_ctx->driver->decode_response(task->drv_ctx, rx_buf, out_msgs, &count);

        if (ret == 0) {
            for (int i = 0; i < count; i++) datahub_publish(task->parent->hub, &out_msgs[i]);
            // 特别注意：如果 decode_response 成功，无论是否有业务数据，都释放总线
            _release_bus(task);
        } else if (ret < 0) {
            LOG_WARN(LOG_CAT_BUS, "Bus %d: Protocol Error %d", task->bus_id, ret);
            _release_bus(task);
        }
    } else if (n < 0 && errno != EAGAIN) {
        _release_bus(task);
    }
    net_buf_unref(rx_buf);
}

static void _on_v3_cmd_start(void *user_data) {
    poll_task_t *task = (poll_task_t *)user_data;
    bus_orchestrator_t *ctx = task->parent;
    int bus_id = task->bus_id;

    if (ctx->current_tasks[bus_id]) return;  // 总线正忙

    net_buf_t *tx_buf = net_buf_alloc(&ctx->pool, 0);
    if (task->drv_ctx->driver->encode_request(task->drv_ctx, &task->tag_id, 1, tx_buf) == 0 &&
        tx_buf->b.len > 0) {
        LOG_INFO(LOG_CAT_BUS, "[V3] Dispatching %s", task->drv_ctx->driver->name);
        write(ctx->bus_fds[bus_id], tx_buf->b.data, tx_buf->b.len);
        ctx->current_tasks[bus_id] = task;
        ctx->bus_polls[bus_id].data = task;
        uv_poll_start(&ctx->bus_polls[bus_id], UV_READABLE, _on_v3_poll_readable);
        uv_timer_start(&task->io_watchdog, _on_io_timeout, 2000, 0);
    } else {
        arbiter_on_io_done(ctx->arb, bus_id, &task->msg);
    }
    net_buf_unref(tx_buf);
}

static void _on_v3_poll_timer(uv_timer_t *handle) {
    poll_task_t *task = (poll_task_t *)handle->data;
    arbiter_submit_cmd(task->parent->arb, task->bus_id, &task->msg);
}

bus_orchestrator_t *bus_orchestrator_create(datahub_t *hub, arbiter_t *arb, int max_buses,
                                            uint32_t pool_size, uint32_t buf_size) {
    bus_orchestrator_t *ctx = calloc(1, sizeof(struct bus_orchestrator_s));
    ctx->hub = hub;
    ctx->arb = arb;
    ctx->loop = hub->loop;
    ctx->max_buses = max_buses > 0 ? max_buses : 8;
    ctx->bus_fds = malloc(sizeof(int) * (size_t)ctx->max_buses);
    ctx->bus_polls = malloc(sizeof(uv_poll_t) * (size_t)ctx->max_buses);
    ctx->current_tasks = calloc((size_t)ctx->max_buses, sizeof(poll_task_t *));

    for (int i = 0; i < ctx->max_buses; i++) ctx->bus_fds[i] = -1;
    net_buf_pool_init(&ctx->pool, (uint16_t)pool_size, (uint16_t)buf_size, 0, NULL, NULL);
    return ctx;
}

void bus_orchestrator_attach_fd(bus_orchestrator_t *ctx, int bus_id, int fd) {
    if (ctx && bus_id < ctx->max_buses) {
        ctx->bus_fds[bus_id] = fd;
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        uv_poll_init(ctx->loop, &ctx->bus_polls[bus_id], fd);
    }
}

int bus_orchestrator_add_poll(bus_orchestrator_t *ctx, const char *protocol, uint32_t device_id,
                              uint32_t tag_id, int interval_ms) {
    poll_task_t *task = calloc(1, sizeof(poll_task_t));
    task->parent = ctx;
    task->tag_id = tag_id;
    task->bus_id = 0;  // 默认使用第一条总线
    task->drv_ctx = driver_instance_create(protocol, device_id);
    if (!task->drv_ctx) {
        free(task);
        return -1;
    }
    strncpy(task->msg.req_id, "POLL_V3", sizeof(task->msg.req_id) - 1);
    task->msg.priority = ARBITER_PRIO_NORMAL;
    task->msg.action = CMD_ACTION_POLL;
    task->msg.on_start = _on_v3_cmd_start;
    task->msg.user_data = task;
    uv_timer_init(ctx->loop, &task->poll_timer);
    task->poll_timer.data = task;
    uv_timer_init(ctx->loop, &task->io_watchdog);
    task->io_watchdog.data = task;
    uv_timer_start(&task->poll_timer, _on_v3_poll_timer, 0, (uint64_t)interval_ms);
    return 0;
}

void bus_orchestrator_run(bus_orchestrator_t *ctx) {
    (void)ctx;
}
void bus_orchestrator_destroy(bus_orchestrator_t *ctx) {
    if (ctx) {
        net_buf_pool_destroy(&ctx->pool);
        free(ctx->bus_fds);
        free(ctx->bus_polls);
        free(ctx->current_tasks);
        free(ctx);
    }
}
