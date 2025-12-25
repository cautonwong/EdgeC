#include "io_executor.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

/* ===== Internal Wrapper ===== */

typedef struct {
    uv_work_t req;
    io_job_t *job;
    io_done_cb cb;
    void *user;
} io_ctx_t;

/* ===== Worker Thread (阻塞区) ===== */

static void io_work(uv_work_t *req) {
    io_ctx_t *ctx = req->data;
    io_job_t *job = ctx->job;

    /* 写 */
    ssize_t n = write(job->fd, job->tx, job->tx_len);
    if (n < 0) {
        job->result = -errno;
        return;
    }

    /* 读（简化：一次 read） */
    n = read(job->fd, job->rx, job->rx_cap);
    if (n < 0) {
        job->result = -errno;
        return;
    }

    job->rx_len = n;
    job->result = 0;
}

/* ===== Back to Loop ===== */

static void io_after(uv_work_t *req, int status) {
    io_ctx_t *ctx = req->data;
    if (ctx->cb) {
        ctx->cb(ctx->job, ctx->user);
    }
    free(ctx);
}

/* ===== Public API ===== */

void io_submit(
    uv_loop_t *loop,
    io_job_t *job,
    io_done_cb cb,
    void *user
) {
    io_ctx_t *ctx = calloc(1, sizeof(*ctx));
    ctx->job = job;
    ctx->cb = cb;
    ctx->user = user;

    ctx->req.data = ctx;
    uv_queue_work(loop, &ctx->req, io_work, io_after);
}
