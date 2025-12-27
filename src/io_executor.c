#include "io_executor.h"
#include "driver_manager.h"
#include "shmring.h"
#include "edge_event.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <uv.h>

// Internal context for each IO thread
typedef struct {
    io_bus_id_t bus_id;
    uv_thread_t thread_id;
    uv_loop_t *loop;        
    uv_async_t async_submit; 
    
    // 监听任务专用 (S2 Active Push)
    uv_poll_t poll_handle;
    int bus_fd;
    bool is_listening;
    
    // Job Queue
    uv_mutex_t queue_lock; // libuv mutex
    io_job_t *head;
    io_job_t *tail;
    
    // Main loop reference
    uv_loop_t *main_loop;
    uv_async_t async_complete; 
    
    // Completed Queue
    uv_mutex_t done_lock; // libuv mutex
    io_job_t *done_head;
    io_job_t *done_tail;
    
    bool running;
} io_thread_ctx_t;

static io_thread_ctx_t g_threads[IO_BUS_MAX];
static uv_loop_t *g_main_loop = NULL;

// --- Helper Queue Functions ---
static void enqueue_job(io_job_t **head, io_job_t **tail, io_job_t *job) {
    job->next = NULL;
    if (*tail) {
        (*tail)->next = job;
        *tail = job;
    } else {
        *head = *tail = job;
    }
}

static io_job_t* dequeue_job(io_job_t **head, io_job_t **tail) {
    if (!*head) return NULL;
    io_job_t *job = *head;
    *head = job->next;
    if (!*head) *tail = NULL;
    job->next = NULL;
    return job;
}

// --- IO Thread Logic ---

static void on_bus_readable(uv_poll_t *handle, int status, int events) {
    (void)status; (void)events;
    io_thread_ctx_t *ctx = (io_thread_ctx_t *)handle->data;
    
    uint8_t buf[256];
    // read 依然是原生的，因为 uv_poll 接管了异步事件，但读取动作通常针对串口/设备
    ssize_t n = read(ctx->bus_fd, buf, sizeof(buf));
    
    if (n > 0) {
        io_job_t *push_job = io_job_create(ctx->bus_id, 0, (size_t)n);
        memcpy(push_job->rx_buf, buf, (size_t)n);
        push_job->rx_len = (size_t)n;
        push_job->result = IO_RESULT_SUCCESS;
        
        uv_mutex_lock(&ctx->done_lock);
        enqueue_job(&ctx->done_head, &ctx->done_tail, push_job);
        uv_mutex_unlock(&ctx->done_lock);
        
        uv_async_send(&ctx->async_complete);
    }
}

static void perform_io(io_thread_ctx_t *ctx, io_job_t *job) {
    if (ctx->is_listening) uv_poll_stop(&ctx->poll_handle);

    int fd = ctx->bus_fd;
    // 如果 fd 无效（/dev/null 或模拟），直接模拟成功
    if (fd < 0 || ctx->bus_id >= IO_BUS_MAX) {
        uv_sleep(10000); // 模拟 10ms 延迟
        if (job->tx_buf && job->rx_buf && job->rx_cap >= 5) {
            // 模拟一个合法的 Modbus 响应：[ID, FC, ByteCnt, DataH, DataL, ...]
            job->rx_buf[0] = job->tx_buf[0];
            job->rx_buf[1] = job->tx_buf[1];
            job->rx_buf[2] = 2; // 2 bytes
            job->rx_buf[3] = 0x00;
            job->rx_buf[4] = 0x7B; // 123 decimal
            job->rx_len = 5;
        }
        job->result = IO_RESULT_SUCCESS;
    } else {
        // 真实 IO 逻辑
        if (job->tx_len > 0) write(fd, job->tx_buf, job->tx_len);
        if (job->rx_cap > 0) {
            ssize_t n = read(fd, job->rx_buf, job->rx_cap);
            if (n > 0) {
                job->rx_len = (size_t)n;
                job->result = IO_RESULT_SUCCESS;
            } else {
                job->result = IO_RESULT_ERROR;
            }
        }
    }

    if (ctx->is_listening && fd >= 0) {
        uv_poll_start(&ctx->poll_handle, UV_READABLE, on_bus_readable);
    }
}

static void on_dummy_close(uv_handle_t *handle) {
    (void)handle;
}

static void on_async_submit(uv_async_t *handle) {
    io_thread_ctx_t *ctx = (io_thread_ctx_t *)handle->data;

    // Graceful shutdown signal
    if (!ctx->running) {
        if (ctx->is_listening && !uv_is_closing((uv_handle_t *)&ctx->poll_handle)) {
            uv_close((uv_handle_t *)&ctx->poll_handle, on_dummy_close);
        }
        if (!uv_is_closing((uv_handle_t *)&ctx->async_submit)) {
            uv_close((uv_handle_t *)&ctx->async_submit, on_dummy_close);
        }
        return;
    }

    while (true) {
        io_job_t *job = NULL;
        uv_mutex_lock(&ctx->queue_lock);
        job = dequeue_job(&ctx->head, &ctx->tail);
        uv_mutex_unlock(&ctx->queue_lock);
        if (!job) break;
        perform_io(ctx, job);
        uv_mutex_lock(&ctx->done_lock);
        enqueue_job(&ctx->done_head, &ctx->done_tail, job);
        uv_mutex_unlock(&ctx->done_lock);
        uv_async_send(&ctx->async_complete);
    }
}

static void io_thread_entry(void *arg) {
    io_thread_ctx_t *ctx = (io_thread_ctx_t *)arg;
    ctx->loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(ctx->loop);
    uv_async_init(ctx->loop, &ctx->async_submit, on_async_submit);
    ctx->async_submit.data = ctx;
    
    ctx->bus_fd = driver_manager_get_fd(NULL, (arbiter_bus_t)ctx->bus_id);
    if (ctx->bus_fd >= 0) {
        uv_poll_init(ctx->loop, &ctx->poll_handle, ctx->bus_fd);
        ctx->poll_handle.data = ctx;
        uv_poll_start(&ctx->poll_handle, UV_READABLE, on_bus_readable);
        ctx->is_listening = true;
    }
    
    ctx->running = true;
    uv_run(ctx->loop, UV_RUN_DEFAULT);
    
    // Cleanup after loop exits
    uv_loop_close(ctx->loop);
    free(ctx->loop);
    ctx->loop = NULL;
    ctx->running = false;
}

static void on_async_complete(uv_async_t *handle) {
    io_thread_ctx_t *ctx = (io_thread_ctx_t *)handle->data;
    while (true) {
        io_job_t *job = NULL;
        uv_mutex_lock(&ctx->done_lock);
        job = dequeue_job(&ctx->done_head, &ctx->done_tail);
        uv_mutex_unlock(&ctx->done_lock);
        if (!job) break;
        if (job->cb) job->cb(job, job->user_data);
        else io_job_free(job);
    }
}

int io_executor_init(uv_loop_t *main_loop) {
    if (!main_loop) return -1;
    g_main_loop = main_loop;
    for (int i = 0; i < IO_BUS_MAX; i++) {
        io_thread_ctx_t *ctx = &g_threads[i];
        ctx->bus_id = (io_bus_id_t)i;
        ctx->main_loop = main_loop;
        ctx->head = ctx->tail = NULL;
        ctx->done_head = ctx->done_tail = NULL;
        uv_mutex_init(&ctx->queue_lock);
        uv_mutex_init(&ctx->done_lock);
        uv_async_init(main_loop, &ctx->async_complete, on_async_complete);
        ctx->async_complete.data = ctx;
        uv_thread_create(&ctx->thread_id, io_thread_entry, ctx);
    }
    return 0;
}

void io_executor_shutdown(void) {
    for (int i = 0; i < IO_BUS_MAX; i++) {
        io_thread_ctx_t *ctx = &g_threads[i];
        if (ctx->running) {
            ctx->running = false;
            // Wake up the loop so it can check the flag and exit
            uv_async_send(&ctx->async_submit); 
            uv_thread_join(&ctx->thread_id);
        }
    }

    for (int i = 0; i < IO_BUS_MAX; i++) {
        io_thread_ctx_t *ctx = &g_threads[i];
        uv_mutex_destroy(&ctx->queue_lock);
        uv_mutex_destroy(&ctx->done_lock);
        if (!uv_is_closing((uv_handle_t*)&ctx->async_complete)) {
            uv_close((uv_handle_t*)&ctx->async_complete, NULL);
        }
    }
    // Final loop run to process close callbacks
    if(g_main_loop) {
        uv_run(g_main_loop, UV_RUN_NOWAIT);
    }
}

int io_executor_submit(io_job_t *job) {
    if (!job || job->bus_id >= IO_BUS_MAX) return -1;
    io_thread_ctx_t *ctx = &g_threads[job->bus_id];
    uv_mutex_lock(&ctx->queue_lock);
    enqueue_job(&ctx->head, &ctx->tail, job);
    uv_mutex_unlock(&ctx->queue_lock);
    if (ctx->loop) uv_async_send(&ctx->async_submit);
    return 0;
}

io_job_t* io_job_create(io_bus_id_t bus, size_t tx_size, size_t rx_size) {
    io_job_t *job = calloc(1, sizeof(io_job_t));
    if (!job) return NULL;
    job->bus_id = bus;
    if (tx_size > 0) { job->tx_buf = malloc(tx_size); job->tx_len = tx_size; }
    if (rx_size > 0) { job->rx_buf = malloc(rx_size); job->rx_cap = rx_size; }
    return job;
}

void io_job_free(io_job_t *job) {
    if (!job) return;
    if (job->tx_buf) free(job->tx_buf);
    if (job->rx_buf) free(job->rx_buf);
    free(job);
}
