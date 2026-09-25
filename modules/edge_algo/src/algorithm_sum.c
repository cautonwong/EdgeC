#include "algorithm_sum.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void worker_thread(void *arg) {
    algo_ctx_t *ctx = (algo_ctx_t *)arg;

    uv_mutex_lock(&ctx->lock);

    while (ctx->running) {
        // Wait for condition: buffer full, flush requested, or stopping
        while (ctx->running && !ctx->flush_requested && ctx->count < ctx->capacity) {
            uv_cond_wait(&ctx->cond, &ctx->lock);
        }

        if (!ctx->running) break;

        // Process data
        if (ctx->count > 0) {
            double sum = 0.0;
            for (size_t i = 0; i < ctx->count; i++) {
                sum += ctx->samples[i].value;
            }

            // Prepare result without dynamic allocation
            ctx->pending_res.total_sum = sum;
            ctx->pending_res.sample_count = (uint32_t)ctx->count;
            ctx->pending_res.average = sum / (double)ctx->count;
            ctx->pending_res.compute_ts = uv_now(ctx->loop);

            ctx->async_result.data = ctx;
            uv_async_send(&ctx->async_result);

            // Clear buffer
            ctx->count = 0;
            ctx->flush_requested = false;
        } else {
            // Spurious wake or flush with empty buffer
            ctx->flush_requested = false;
        }
    }

    uv_mutex_unlock(&ctx->lock);
}

static void on_result_async(uv_async_t *handle) {
    algo_ctx_t *ctx = (algo_ctx_t *)handle->data;
    if (ctx && ctx->cb) {
        algo_result_t res_copy;
        uv_mutex_lock(&ctx->lock);
        res_copy = ctx->pending_res;
        uv_mutex_unlock(&ctx->lock);
        ctx->cb(ctx, &res_copy, ctx->user_data);
    }
}

static void on_async_close(uv_handle_t *handle) {
    // Retrieve ctx using pointer arithmetic (offsetof)
    algo_ctx_t *ctx = (algo_ctx_t *)((char *)handle - offsetof(algo_ctx_t, async_result));

    uv_mutex_destroy(&ctx->lock);
    uv_cond_destroy(&ctx->cond);
    free(ctx->samples);
    free(ctx);
}

int algo_init(algo_ctx_t **out_ctx, uv_loop_t *loop, size_t capacity, algo_result_cb cb,
              void *user_data) {
    algo_ctx_t *ctx = calloc(1, sizeof(algo_ctx_t));
    if (!ctx) return -1;

    ctx->loop = loop;
    ctx->capacity = (capacity > 0) ? capacity : 1024;
    ctx->cb = cb;
    ctx->user_data = user_data;

    ctx->samples = calloc(ctx->capacity, sizeof(algo_sample_t));
    if (!ctx->samples) {
        free(ctx);
        return -1;
    }

    if (uv_mutex_init(&ctx->lock) != 0) {
        free(ctx->samples);
        free(ctx);
        return -1;
    }

    if (uv_cond_init(&ctx->cond) != 0) {
        uv_mutex_destroy(&ctx->lock);
        free(ctx->samples);
        free(ctx);
        return -1;
    }

    if (uv_async_init(loop, &ctx->async_result, on_result_async) != 0) {
        uv_cond_destroy(&ctx->cond);
        uv_mutex_destroy(&ctx->lock);
        free(ctx->samples);
        free(ctx);
        return -1;
    }

    ctx->running = true;
    if (uv_thread_create(&ctx->thread, worker_thread, ctx) != 0) {
        uv_close((uv_handle_t *)&ctx->async_result, NULL);
        uv_cond_destroy(&ctx->cond);
        uv_mutex_destroy(&ctx->lock);
        free(ctx->samples);
        free(ctx);
        return -1;
    }

    *out_ctx = ctx;
    return 0;
}

int algo_submit(algo_ctx_t *ctx, const algo_sample_t *sample) {
    if (!ctx || !sample) return -1;

    uv_mutex_lock(&ctx->lock);

    if (ctx->count < ctx->capacity) {
        ctx->samples[ctx->count++] = *sample;
    }

    if (ctx->count >= ctx->capacity) {
        uv_cond_signal(&ctx->cond);
    }

    uv_mutex_unlock(&ctx->lock);
    return 0;
}

void algo_flush(algo_ctx_t *ctx) {
    if (!ctx) return;
    uv_mutex_lock(&ctx->lock);
    ctx->flush_requested = true;
    uv_cond_signal(&ctx->cond);
    uv_mutex_unlock(&ctx->lock);
}

void algo_destroy(algo_ctx_t *ctx) {
    if (!ctx) return;

    // Stop the thread
    uv_mutex_lock(&ctx->lock);
    ctx->running = false;
    uv_cond_signal(&ctx->cond);
    uv_mutex_unlock(&ctx->lock);

    uv_thread_join(&ctx->thread);

    // Cleanup resources
    // The handle close callback (on_async_close) will free the memory
    uv_close((uv_handle_t *)&ctx->async_result, on_async_close);
}
