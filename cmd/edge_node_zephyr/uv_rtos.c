#include "uv_rtos.h"

#include <string.h>

#define UV_WORKER_COUNT 4
#define UV_THREAD_STACK_SIZE 2048

/* --- 内部逻辑：阶段分发 --- */
static void uv__run_prepare(uv_loop_t* loop) {
    struct uv__queue* q;
    uv__queue_foreach(q, &loop->prepare_handles) {
        uv_prepare_t* h = edge_container_of(q, uv_prepare_t, queue);
        if (h->prepare_cb) h->prepare_cb(h);
    }
}

static void uv__run_check(uv_loop_t* loop) {
    struct uv__queue* q;
    uv__queue_foreach(q, &loop->check_handles) {
        uv_check_t* h = edge_container_of(q, uv_check_t, queue);
        if (h->check_cb) h->check_cb(h);
    }
}

static void uv__run_async(uv_loop_t* loop) {
    struct uv__queue* q;
    uv__queue_foreach(q, &loop->async_handles) {
        uv_async_t* h = edge_container_of(q, uv_async_t, queue);
        if (edge_atomic_get(&h->pending)) {
            edge_atomic_set(&h->pending, 0);
            if (h->async_cb) h->async_cb(h);
        }
    }
}

/* --- API: Async (ISR 安全) --- */
int uv_async_init(uv_loop_t* loop, uv_async_t* handle, uv_async_cb cb) {
    handle->loop = loop;
    handle->async_cb = cb;
    edge_atomic_set(&handle->pending, 0);
    uv__queue_insert_tail(&loop->async_handles, &handle->queue);
    return 0;
}

int uv_async_send(uv_async_t* handle) {
    edge_atomic_set(&handle->pending, 1);
    edge_eventfd_write(handle->loop->async_efd);
    return 0;
}

/* --- API: Prepare/Check --- */
int uv_prepare_init(uv_loop_t* loop, uv_prepare_t* handle) {
    handle->loop = loop;
    uv__queue_init(&handle->queue);
    return 0;
}
int uv_prepare_start(uv_prepare_t* handle, uv_prepare_cb cb) {
    handle->prepare_cb = cb;
    uv__queue_insert_tail(&handle->loop->prepare_handles, &handle->queue);
    return 0;
}
int uv_prepare_stop(uv_prepare_t* handle) {
    uv__queue_remove(&handle->queue);
    uv__queue_init(&handle->queue);
    return 0;
}

int uv_check_init(uv_loop_t* loop, uv_check_t* handle) {
    handle->loop = loop;
    uv__queue_init(&handle->queue);
    return 0;
}
int uv_check_start(uv_check_t* handle, uv_check_cb cb) {
    handle->check_cb = cb;
    uv__queue_insert_tail(&handle->loop->check_handles, &handle->queue);
    return 0;
}

/* --- 核心 Loop 升级 --- */
int uv_loop_init(uv_loop_t* loop) {
    if (!loop) return -1;
    uv__queue_init(&loop->pending_queue);
    uv__queue_init(&loop->wq);
    uv__queue_init(&loop->timer_queue);
    uv__queue_init(&loop->async_handles);
    uv__queue_init(&loop->prepare_handles);
    uv__queue_init(&loop->check_handles);

    loop->pending_mutex = edge_mutex_create();
    loop->wq_mutex = edge_mutex_create();
    loop->wq_sem = edge_sem_create(0);
    loop->poller = edge_poller_create();
    loop->async_efd = edge_eventfd_create();
    edge_atomic_set(&loop->stop_flag, 0);

    /* 内部异步分发器 */
    uv_poll_init(loop, &loop->async_watcher, loop->async_efd);
    uv_poll_start(&loop->async_watcher, EDGE_POLL_IN, NULL);  // 我们在 run 里统一手动分发

    /* 启动线程池略... (保持之前的 worker 入口) */
    return 0;
}

int uv_run(uv_loop_t* loop) {
    while (!edge_atomic_get(&loop->stop_flag)) {
        /* 1. 更新定时器 */
        uv__run_timers(loop);

        /* 2. Prepare 阶段 (Poll 之前) */
        uv__run_prepare(loop);

        /* 3. Poll 阶段 */
        int32_t timeout = uv__backend_timeout(loop);
        edge_poller_wait(loop->poller, timeout);

        /* 4. Check 阶段 (Poll 之后) */
        uv__run_check(loop);

        /* 5. 异步 I/O 事件处理 */
        int fd, events;
        void* user_data;
        for (int i = 0; i < EDGE_OS_MAX_POLL_FDS; i++) {
            if (edge_poller_get_event(loop->poller, i, &fd, &events, &user_data) > 0) {
                if (fd == loop->async_efd) {
                    edge_eventfd_read(fd); /* 消费 */
                } else {
                    uv_poll_t* w = (uv_poll_t*)user_data;
                    if (w && w->poll_cb) w->poll_cb(w, 0, events);
                }
            }
        }

        /* 6. 处理 Async 句柄 (ISR 来的通知) */
        uv__run_async(loop);

        /* 7. 处理 Worker Callbacks (Pending Queue) */
        /* (复用之前的 pending_queue 处理逻辑) */
        while (1) {
            edge_mutex_lock(loop->pending_mutex);
            if (uv__queue_empty(&loop->pending_queue)) {
                edge_mutex_unlock(loop->pending_mutex);
                break;
            }
            struct uv__queue* q = uv__queue_head(&loop->pending_queue);
            uv_work_t* req = edge_container_of(q, uv_work_t, pending_queue);
            uv__queue_remove(q);
            edge_mutex_unlock(loop->pending_mutex);
            if (req->after_work_cb) req->after_work_cb(req, 0);
        }
    }
    return 0;
}
