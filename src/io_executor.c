#include "io_executor.h"
#include "driver_manager.h" // Assuming this provides FDs
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

// Internal context for each IO thread
typedef struct {
    io_bus_id_t bus_id;
    uv_thread_t thread_id;
    uv_loop_t *loop;        // Loop running in the IO thread
    uv_async_t async_submit; // Async handle to wake up loop for new jobs
    
    // Job Queue (Protected by mutex)
    pthread_mutex_t queue_lock;
    io_job_t *head;
    io_job_t *tail;
    
    // Main loop reference for callbacks
    uv_loop_t *main_loop;
    uv_async_t async_complete; // Async handle to notify main loop
    
    // Completed Queue (passing back to main loop)
    pthread_mutex_t done_lock;
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

static void perform_io(io_thread_ctx_t *ctx, io_job_t *job) {
    // 1. Get FD from Driver Manager (Mock or Real)
    // In a real implementation, we would likely keep the FD open in the ctx
    // For now, let's assume we get it or open it.
    // NOTE: This relies on driver_manager.h/c being compatible or mocked.
    int fd = -1; 
    // fd = driver_manager_get_fd(job->bus_id); 
    // Fallback for demo: use a pipe or dummy if DM not ready
    if (fd < 0) {
        // Simulation mode
        usleep(10000); // Simulate 10ms bus time
        // Loopback mock
        if (job->tx && job->rx) {
            size_t copy_len = job->tx_len < job->rx_cap ? job->tx_len : job->rx_cap;
            memcpy(job->rx, job->tx, copy_len);
            job->rx_len = copy_len;
        }
        job->result = IO_RESULT_SUCCESS;
        return;
    }

    // Real IO Logic (Blocking or Polling)
    // Since we are in a dedicated thread, blocking read/write is acceptable and simplest for RTU.
    
    // Write
    if (job->tx_len > 0) {
        ssize_t n = write(fd, job->tx, job->tx_len);
        if (n < 0) {
            job->result = IO_RESULT_ERROR;
            job->system_errno = errno;
            return;
        }
    }
    
    // Read
    if (job->rx_cap > 0) {
        // RS485 specific: Wait for silence or timeout?
        // Simple blocking read for now
        // Set timeout via setsockopt or select if needed.
        ssize_t n = read(fd, job->rx, job->rx_cap);
        if (n < 0) {
            job->result = IO_RESULT_ERROR;
            job->system_errno = errno;
        } else {
            job->rx_len = (size_t)n;
            job->result = IO_RESULT_SUCCESS;
        }
    } else {
        job->result = IO_RESULT_SUCCESS;
    }
}

// Executed in IO Thread
static void on_async_submit(uv_async_t *handle) {
    io_thread_ctx_t *ctx = (io_thread_ctx_t *)handle->data;
    
    while (true) {
        io_job_t *job = NULL;
        pthread_mutex_lock(&ctx->queue_lock);
        job = dequeue_job(&ctx->head, &ctx->tail);
        pthread_mutex_unlock(&ctx->queue_lock);
        
        if (!job) break;
        
        // Execute Job
        perform_io(ctx, job);
        
        // Push to done queue
        pthread_mutex_lock(&ctx->done_lock);
        enqueue_job(&ctx->done_head, &ctx->done_tail, job);
        pthread_mutex_unlock(&ctx->done_lock);
        
        // Notify Main Thread
        uv_async_send(&ctx->async_complete);
    }
}

static void io_thread_entry(void *arg) {
    io_thread_ctx_t *ctx = (io_thread_ctx_t *)arg;
    
    ctx->loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(ctx->loop);
    
    uv_async_init(ctx->loop, &ctx->async_submit, on_async_submit);
    ctx->async_submit.data = ctx;
    
    ctx->running = true;
    
    // Run loop
    uv_run(ctx->loop, UV_RUN_DEFAULT);
    
    // Cleanup
    uv_loop_close(ctx->loop);
    free(ctx->loop);
    ctx->running = false;
}

// --- Main Thread Logic ---

// Executed in Main Thread
static void on_async_complete(uv_async_t *handle) {
    io_thread_ctx_t *ctx = (io_thread_ctx_t *)handle->data;
    
    while (true) {
        io_job_t *job = NULL;
        pthread_mutex_lock(&ctx->done_lock);
        job = dequeue_job(&ctx->done_head, &ctx->done_tail);
        pthread_mutex_unlock(&ctx->done_lock);
        
        if (!job) break;
        
        // Callback
        if (job->cb) {
            job->cb(job, job->user_data);
        }
    }
}

// --- Public API ---

int io_executor_init(uv_loop_t *main_loop) {
    if (!main_loop) return -1;
    g_main_loop = main_loop;
    
    for (int i = 0; i < IO_BUS_MAX; i++) {
        io_thread_ctx_t *ctx = &g_threads[i];
        ctx->bus_id = (io_bus_id_t)i;
        ctx->main_loop = main_loop;
        ctx->head = ctx->tail = NULL;
        ctx->done_head = ctx->done_tail = NULL;
        
        pthread_mutex_init(&ctx->queue_lock, NULL);
        pthread_mutex_init(&ctx->done_lock, NULL);
        
        // Init complete async in MAIN loop
        uv_async_init(main_loop, &ctx->async_complete, on_async_complete);
        ctx->async_complete.data = ctx;
        
        // Start IO Thread
        uv_thread_create(&ctx->thread_id, io_thread_entry, ctx);
    }
    return 0;
}

void io_executor_shutdown(void) {
    for (int i = 0; i < IO_BUS_MAX; i++) {
        io_thread_ctx_t *ctx = &g_threads[i];
        if (ctx->running) {
            // In a real app, we need a way to stop the IO loop (uv_stop or close async)
            // For now, we assume process exit cleans up or we implement graceful stop later
            // uv_async_send(&ctx->async_submit); // Trigger wake up then stop?
        }
        pthread_mutex_destroy(&ctx->queue_lock);
        pthread_mutex_destroy(&ctx->done_lock);
        
        // Close handles
        uv_close((uv_handle_t*)&ctx->async_complete, NULL);
    }
}

int io_executor_submit(io_job_t *job) {
    if (!job || job->bus_id >= IO_BUS_MAX) return -1;
    
    io_thread_ctx_t *ctx = &g_threads[job->bus_id];
    
    pthread_mutex_lock(&ctx->queue_lock);
    enqueue_job(&ctx->head, &ctx->tail, job);
    pthread_mutex_unlock(&ctx->queue_lock);
    
    // Notify IO thread
    if (ctx->loop) {
        uv_async_send(&ctx->async_submit);
    }
    return 0;
}

// --- Job Helpers ---

io_job_t* io_job_create(io_bus_id_t bus, size_t tx_size, size_t rx_size) {
    io_job_t *job = calloc(1, sizeof(io_job_t));
    if (!job) return NULL;
    
    job->bus_id = bus;
    
    if (tx_size > 0) {
        job->tx_buf = malloc(tx_size);
        job->tx_len = tx_size;
    }
    
    if (rx_size > 0) {
        job->rx_buf = malloc(rx_size);
        job->rx_cap = rx_size;
    }
    
    return job;
}

void io_job_free(io_job_t *job) {
    if (!job) return;
    if (job->tx_buf) free(job->tx_buf);
    if (job->rx_buf) free(job->rx_buf);
    free(job);
}