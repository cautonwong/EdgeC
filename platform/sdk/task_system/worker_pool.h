#ifndef CEDGE_WORKER_POOL_H
#define CEDGE_WORKER_POOL_H

#include <uv.h>

// Forward declarations
typedef struct cedgex_worker_pool_t cedgex_worker_pool_t;

// The context for a single job. User should embed this in their own struct.
typedef struct {
    void *user_data;
    // Internal use
    uv_work_t work_req;
    cedgex_worker_pool_t *pool;
} cedgex_job_t;

// Callback function types
// Runs in a worker thread. MUST NOT use the event loop or any non-thread-safe functions.
typedef void (*cedgex_job_work_cb)(cedgex_job_t *job); 
// Runs in the main event loop thread after the work is done.
typedef void (*cedgex_job_done_cb)(cedgex_job_t *job, int status); 

// The worker pool structure
struct cedgex_worker_pool_t {
    uv_loop_t *loop;
};

/**
 * @brief Initializes the worker pool.
 *
 * @param pool The worker pool instance.
 * @param loop The libuv event loop.
 * @return 0 on success.
 */
int worker_pool_init(cedgex_worker_pool_t *pool, uv_loop_t *loop);

/**
 * @brief Submits a job to the worker pool.
 *
 * @param pool The worker pool instance.
 * @param job A pointer to a job context. The caller must allocate this and keep it valid
 *            until the `done_cb` is called. The `done_cb` is responsible for freeing it.
 * @param work_cb The function to execute in a worker thread.
 * @param done_cb The function to execute in the main loop thread when work is complete.
 * @return 0 on success, -1 on failure.
 */
int worker_pool_submit(cedgex_worker_pool_t *pool, cedgex_job_t *job, cedgex_job_work_cb work_cb, cedgex_job_done_cb done_cb);

// Note: A simple cleanup function is not provided as the pool itself doesn't
// own any resources besides the loop reference. Jobs are managed by the user.

#endif // CEDGE_WORKER_POOL_H
