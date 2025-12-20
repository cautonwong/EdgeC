#include "worker_pool.h"
#include <stdlib.h>

// A wrapper struct to hold all necessary callbacks and data
typedef struct {
    cedgex_job_t *job;
    cedgex_job_work_cb work_cb;
    cedgex_job_done_cb done_cb;
} internal_work_ctx_t;


// This function is executed in the worker thread.
static void internal_work_callback(uv_work_t *req) {
    internal_work_ctx_t *ctx = (internal_work_ctx_t*)req->data;
    if (ctx && ctx->work_cb) {
        // We call the user's work callback, passing them their original job object
        ctx->work_cb(ctx->job);
    }
}

// This function is executed in the main loop thread after the work is done.
static void internal_done_callback(uv_work_t *req, int status) {
    internal_work_ctx_t *ctx = (internal_work_ctx_t*)req->data;
    if (ctx && ctx->done_cb) {
        // We call the user's done callback
        ctx->done_cb(ctx->job, status);
    }
    
    // The internal context is now finished, free it.
    // The user is responsible for freeing their own job context inside the done_cb.
    if (ctx) {
        free(ctx);
    }
    // The req itself is part of the user's job struct, so it is not freed here.
}


int worker_pool_init(cedgex_worker_pool_t *pool, uv_loop_t *loop) {
    if (!pool || !loop) return -1;
    pool->loop = loop;
    return 0;
}

int worker_pool_submit(cedgex_worker_pool_t *pool, cedgex_job_t *job, cedgex_job_work_cb work_cb, cedgex_job_done_cb done_cb) {
    if (!pool || !job || !work_cb || !done_cb) return -1;

    // Create a context to hold all the information for the callbacks
    internal_work_ctx_t *ctx = (internal_work_ctx_t*)malloc(sizeof(internal_work_ctx_t));
    if (!ctx) return -1;

    ctx->job = job;
    ctx->work_cb = work_cb;
    ctx->done_cb = done_cb;

    // Link our internal context to the libuv request
    job->work_req.data = ctx;
    // Link the pool back to the job for context
    job->pool = pool;

    // Queue the work
    int status = uv_queue_work(pool->loop, &job->work_req, internal_work_callback, internal_done_callback);
    if (status != 0) {
        free(ctx);
        return status;
    }

    return 0;
}
