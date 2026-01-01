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

// --- 驱动回调适配 ---
static void on_driver_msg(pipe_msg_t *msg, void *user_data) {
    io_executor_t *ctx = (io_executor_t *)user_data;
    if (ctx && ctx->hub) {
        datahub_publish(ctx->hub, msg);
    }
}

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
    ctx->is_running = 1;
    uint8_t tx_buf[256], rx_buf[512];
    size_t tx_len;
    while (ctx->is_running) {
        for (int i = 0; i < ctx->group_count; i++) {
            edge_poll_group_t *g = &ctx->groups[i];
            if (ctx->driver && ctx->driver->build_batch_req(g, tx_buf, &tx_len) == 0) {
                size_t rx_len = (size_t)(3 + g->count * 2);
                memset(rx_buf, 0, rx_len);
                rx_buf[0] = (uint8_t)g->slave_id;
                rx_buf[1] = 0x03;
                rx_buf[2] = (uint8_t)(g->count * 2);
                for (int j = 0; j < g->count; j++) {
                    uint16_t val = (uint16_t)(g->start_addr + j);
                    rx_buf[3 + j * 2] = (uint8_t)(val >> 8);
                    rx_buf[4 + j * 2] = (uint8_t)(val & 0xFF);
                }
                ctx->driver->parse_batch_res(g, rx_buf, rx_len, on_driver_msg, ctx);
            }
        }
        usleep(1000000);
    }
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
