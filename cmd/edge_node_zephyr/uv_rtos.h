#ifndef EDGE_UV_RTOS_H
#define EDGE_UV_RTOS_H

#include "edge_osal.h"
#include "queue.h"

typedef struct uv_loop_s uv_loop_t;
typedef struct uv_work_s uv_work_t;
typedef struct uv_poll_s uv_poll_t;
typedef struct uv_async_s uv_async_t;
typedef struct uv_prepare_s uv_prepare_t;
typedef struct uv_check_s uv_check_t;

typedef void (*uv_work_cb)(uv_work_t* req);
typedef void (*uv_after_work_cb)(uv_work_t* req, int status);
typedef void (*uv_timer_cb)(uv_timer_t* handle);
typedef void (*uv_poll_cb)(uv_poll_t* handle, int status, int events);
typedef void (*uv_async_cb)(uv_async_t* handle);
typedef void (*uv_prepare_cb)(uv_prepare_t* handle);
typedef void (*uv_check_cb)(uv_check_t* handle);

/* 任务状态枚举 */
enum { UV_STATE_IDLE = 0, UV_STATE_PENDING, UV_STATE_RUNNING, UV_STATE_DONE, UV_STATE_CANCELED };

struct uv_async_s {
    struct uv__queue queue;
    uv_async_cb async_cb;
    uv_loop_t* loop;
    int efd;
    volatile int pending;
};

struct uv_prepare_s {
    struct uv__queue queue;
    uv_prepare_cb prepare_cb;
    uv_loop_t* loop;
};

struct uv_check_s {
    struct uv__queue queue;
    uv_check_cb check_cb;
    uv_loop_t* loop;
};

struct uv_poll_s {
    int fd;
    uv_poll_cb poll_cb;
    uv_loop_t* loop;
    void* data;
};

struct uv_timer_s {
    struct uv__queue tree_node;
    uint64_t timeout;
    uint64_t repeat;
    uv_timer_cb timer_cb;
    uv_loop_t* loop;
};

struct uv_work_s {
    struct uv__queue queue;         /* 挂载在线程池工作队列 */
    struct uv__queue pending_queue; /* 挂载在主循环完成队列 */
    uv_work_cb work_cb;
    uv_after_work_cb after_work_cb;
    uv_loop_t* loop;
    volatile int32_t status;
    void* data;
};

struct uv_loop_s {
    struct uv__queue pending_queue;
    edge_mutex_t pending_mutex;

    /* I/O 多路复用后端 */
    edge_poller_t poller;
    int async_efd; /* EventFD 用于内部唤醒 */
    uv_poll_t async_watcher;

    struct uv__queue timer_queue;
    struct uv__queue async_handles;
    struct uv__queue prepare_handles;
    struct uv__queue check_handles;

    /* 线程池逻辑 */
    struct uv__queue wq;
    edge_mutex_t wq_mutex;
    edge_sem_t wq_sem;
    volatile int32_t stop_flag;
};

/* API 接口 */
int uv_loop_init(uv_loop_t* loop);
int uv_run(uv_loop_t* loop);

/* Async API (ISR 安全) */
int uv_async_init(uv_loop_t* loop, uv_async_t* handle, uv_async_cb cb);
int uv_async_send(uv_async_t* handle);

/* Prepare & Check API */
int uv_prepare_init(uv_loop_t* loop, uv_prepare_t* handle);
int uv_prepare_start(uv_prepare_t* handle, uv_prepare_cb cb);
int uv_prepare_stop(uv_prepare_t* handle);

int uv_check_init(uv_loop_t* loop, uv_check_t* handle);
int uv_check_start(uv_check_t* handle, uv_check_cb cb);
int uv_check_stop(uv_check_t* handle);

/* Timer & Work API (保持不变) */
int uv_queue_work(uv_loop_t* loop, uv_work_t* req, uv_work_cb work_cb,
                  uv_after_work_cb after_work_cb);
int uv_timer_init(uv_loop_t* loop, uv_timer_t* handle);
int uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat);
int uv_poll_init(uv_loop_t* loop, uv_poll_t* handle, int fd);
int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb);

#endif
