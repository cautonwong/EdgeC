#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

// Sample data structure (Input)
typedef struct {
    uint32_t meter_id;
    double value;
    uint64_t ts;
} algo_sample_t;

// Result data structure (Output)
typedef struct {
    double total_sum;
    uint32_t sample_count;
    double average;
    uint64_t compute_ts;
} algo_result_t;

// Forward declaration
typedef struct algo_ctx_s algo_ctx_t;

// Callback type for results (executed on the main loop)
typedef void (*algo_result_cb)(algo_ctx_t *ctx, const algo_result_t *result, void *user_data);

// Context structure
struct algo_ctx_s {
    uv_loop_t *loop;

    // Threading primitives
    uv_thread_t thread;
    uv_mutex_t lock;
    uv_cond_t cond;
    uv_async_t async_result;

    // Data buffers (Double buffering or Ring buffer could be used, here simple buffer)
    algo_sample_t *samples;
    size_t capacity;
    size_t count;

    // Output callback
    algo_result_cb cb;
    void *user_data;

    // Internal state
    algo_result_t pending_res;
    bool running;
    bool flush_requested;
};

/**
 * @brief Initializes the fusion algorithm engine.
 *
 * @param ctx Pointer to the context pointer to be allocated.
 * @param loop The main event loop.
 * @param capacity Maximum number of samples to buffer before auto-flush.
 * @param cb Callback function for results.
 * @param user_data User data for the callback.
 * @return 0 on success, < 0 on failure.
 */
int algo_init(algo_ctx_t **ctx, uv_loop_t *loop, size_t capacity, algo_result_cb cb,
              void *user_data);

/**
 * @brief Submits a sample to the algorithm engine. Thread-safe.
 *
 * @param ctx The algorithm context.
 * @param sample The sample data.
 * @return 0 on success.
 */
int algo_submit(algo_ctx_t *ctx, const algo_sample_t *sample);

/**
 * @brief Forces the engine to process currently buffered samples immediately.
 *
 * @param ctx The algorithm context.
 */
void algo_flush(algo_ctx_t *ctx);

/**
 * @brief Stops the engine and cleans up resources.
 *
 * @param ctx The algorithm context.
 */
void algo_destroy(algo_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
