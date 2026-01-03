#include "io_executor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "datahub.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "shmring.h"

#define MAX_POLL_GROUPS 128

struct io_executor_s {
    edge_driver_t *driver;
    datahub_t *hub;
    edge_poll_group_t groups[MAX_POLL_GROUPS];
    int group_count;
    int is_running;
};

// --- 驱动回调适配 (已迁移至 Orchestrator) ---
#if 0
static void on_driver_msg(pipe_msg_t *msg, void *user_data) {
    io_executor_t *ctx = (io_executor_t *)user_data;
    if (ctx && ctx->hub) {
        datahub_publish(ctx->hub, msg);
    }
}
#endif

io_executor_t *io_executor_create(edge_driver_t *driver, datahub_t *hub) {
    io_executor_t *ctx = calloc(1, sizeof(struct io_executor_s));
    ctx->driver = driver;
    ctx->hub = hub;
    return ctx;
}

void io_executor_plan_polling(io_executor_t *ctx, uint16_t start, uint16_t count) {
    if (ctx->group_count >= MAX_POLL_GROUPS) return;
    edge_poll_group_t *g = &ctx->groups[ctx->group_count++];
    g->slave_id = 1;
    g->start_addr = start;
    g->count = count;
    g->func_code = 0x03;
}

void io_executor_run(io_executor_t *ctx) {
    if (!ctx) return;
    ctx->is_running = 1;
    LOG_INFO(LOG_CAT_IO, "IO Executor Plane started (Event-driven mode)");

    // 原有的 while(1) 循环极其低效且阻塞，现已移除。
    // 在 V3 架构中，IO 的触发应由 Arbiter 在主 Loop 中通过 uv_poll 触发。
}

// --- V1 接口实装 (支持测试与同步执行) ---

int io_executor_init(uv_loop_t *main_loop) {
    (void)main_loop;
    return 0;
}
void io_executor_shutdown(void) {
}

int io_executor_submit(io_job_t *job) {
    if (!job) return -1;
    // 模拟同步完成：设置结果并触发回调
    job->result = IO_RESULT_SUCCESS;
    if (job->cb) job->cb(job, job->user_data);
    return 0;
}

void io_executor_cancel(io_job_t *job) {
    (void)job;
}

io_job_t *io_job_create(io_bus_id_t bus, size_t tx_size, size_t rx_size) {
    io_job_t *job = calloc(1, sizeof(struct io_job_s));
    job->bus_id = bus;
    if (tx_size) job->tx_buf = malloc(tx_size);
    if (rx_size) job->rx_buf = malloc(rx_size);
    job->rx_cap = rx_size;
    return job;
}

void io_job_free(io_job_t *job) {
    if (job) {
        if (job->tx_buf) free(job->tx_buf);
        if (job->rx_buf) free(job->rx_buf);
        free(job);
    }
}

void io_executor_stop(io_executor_t *ctx) {
    if (ctx) ctx->is_running = 0;
}
void io_executor_destroy(io_executor_t *ctx) {
    if (ctx) free(ctx);
}
