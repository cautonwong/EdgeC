#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "edge_osal.h"

LOG_MODULE_REGISTER(edge_osal, LOG_LEVEL_INF);

/* --- 互斥锁 --- */
struct edge_mutex_s {
    struct k_mutex k_mtx;
};
edge_mutex_t edge_mutex_create(void) {
    struct edge_mutex_s* m = k_malloc(sizeof(struct edge_mutex_s));
    if (!m) return NULL;
    k_mutex_init(&m->k_mtx);
    return m;
}
void edge_mutex_lock(edge_mutex_t mutex) {
    if (mutex) k_mutex_lock(&mutex->k_mtx, K_FOREVER);
}
void edge_mutex_unlock(edge_mutex_t mutex) {
    if (mutex) k_mutex_unlock(&mutex->k_mtx);
}

/* --- 信号量 --- */
struct edge_sem_s {
    struct k_sem k_s;
};
edge_sem_t edge_sem_create(uint32_t initial_count) {
    struct edge_sem_s* s = k_malloc(sizeof(struct edge_sem_s));
    if (!s) return NULL;
    k_sem_init(&s->k_s, initial_count, K_SEM_MAX_LIMIT);
    return s;
}
void edge_sem_give(edge_sem_t sem) {
    if (sem) k_sem_give(&sem->k_s);
}
void edge_sem_take(edge_sem_t sem) {
    if (sem) k_sem_take(&sem->k_s, K_FOREVER);
}

/* --- 事件信号 --- */
struct edge_signal_s {
    struct k_poll_signal k_sig;
    struct k_poll_event k_evt;
};
edge_signal_t edge_signal_create(void) {
    struct edge_signal_s* s = k_malloc(sizeof(struct edge_signal_s));
    if (!s) return NULL;
    k_poll_signal_init(&s->k_sig);
    k_poll_event_init(&s->k_evt, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &s->k_sig);
    return s;
}
void edge_signal_raise(edge_signal_t sig) {
    if (sig) k_poll_signal_raise(&sig->k_sig, 1);
}
void edge_signal_wait(edge_signal_t sig, int32_t timeout_ms) {
    if (sig) k_poll(&sig->k_evt, 1, (timeout_ms < 0) ? K_FOREVER : K_MSEC(timeout_ms));
}
void edge_signal_clear(edge_signal_t sig) {
    if (sig) {
        sig->k_sig.signaled = 0;
        sig->k_evt.state = K_POLL_STATE_NOT_READY;
    }
}

/* --- 线程管理 --- */
struct edge_thread_s {
    k_tid_t tid;
};
edge_thread_t edge_thread_create(const char* name, edge_thread_entry_t entry, void* arg,
                                 int priority, size_t stack_size) {
    struct edge_thread_s* t = k_malloc(sizeof(struct edge_thread_s));
    if (!t) return NULL;

    // 使用动态分配的栈，要求 CONFIG_DYNAMIC_THREAD=y
    void* stack = k_thread_stack_alloc(stack_size, 0);
    if (!stack) {
        k_free(t);
        return NULL;
    }

    t->tid = k_thread_create(k_malloc(sizeof(struct k_thread)), stack, stack_size,
                             (k_thread_entry_t)entry, arg, NULL, NULL, priority, 0, K_NO_WAIT);
    if (t->tid) {
        k_thread_name_set(t->tid, name);
    }
    return t;
}

/* --- 原子操作 --- */
int32_t edge_atomic_cas(volatile int32_t* ptr, int32_t expected, int32_t desired) {
    return (int32_t)atomic_cas((atomic_t*)ptr, (atomic_val_t)expected, (atomic_val_t)desired);
}
void edge_atomic_set(volatile int32_t* ptr, int32_t val) {
    atomic_set((atomic_t*)ptr, (atomic_val_t)val);
}
int32_t edge_atomic_get(volatile int32_t* ptr) {
    return (int32_t)atomic_get((atomic_t*)ptr);
}

#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/eventfd.h>
#include <zephyr/posix/unistd.h>

/* --- Poller 实现 (基于 poll) --- */

#define MAX_POLL_FDS 64 /* 根据内存大小调整 */

struct edge_poller_s {
    struct pollfd fds[MAX_POLL_FDS];

    void* user_datas[MAX_POLL_FDS];

    int count;
};

edge_poller_t edge_poller_create(void) {
    struct edge_poller_s* p = k_malloc(sizeof(struct edge_poller_s));

    if (p) {
        memset(p, 0, sizeof(*p));
    }

    return p;
}

int edge_poller_ctl(edge_poller_t p, int fd, int events, void* user_data) {
    if (!p) return -1;

    /* 1. 查找是否存在 */

    int idx = -1;

    for (int i = 0; i < p->count; i++) {
        if (p->fds[i].fd == fd) {
            idx = i;

            break;
        }
    }

    /* 删除操作 */

    if (events ==
        0) {  // Assume 0 means Remove for simplicity in this abstract layer or use dedicated flag

        if (idx >= 0) {
            // 移动最后一个元素填补空缺

            p->fds[idx] = p->fds[p->count - 1];

            p->user_datas[idx] = p->user_datas[p->count - 1];

            p->count--;

            return 0;
        }

        return -1;
    }

    /* 新增或修改 */

    if (idx == -1) {
        if (p->count >= MAX_POLL_FDS) return -1;  // Full

        idx = p->count++;
    }

    p->fds[idx].fd = fd;

    p->fds[idx].events = 0;

    if (events & EDGE_POLL_IN) p->fds[idx].events |= POLLIN;

    if (events & EDGE_POLL_OUT) p->fds[idx].events |= POLLOUT;

    p->user_datas[idx] = user_data;

    return 0;
}

int edge_poller_wait(edge_poller_t p, int timeout_ms) {
    if (!p || p->count == 0) {
        if (timeout_ms > 0) k_sleep(K_MSEC(timeout_ms));

        return 0;
    }

    return poll(p->fds, p->count, timeout_ms);
}

int edge_poller_get_event(edge_poller_t p, int index, int* fd, int* events, void** user_data) {
    if (!p || index >= p->count) return -1;

    // 注意：poll 返回后，我们需要遍历找到第 N 个发生事件的 fd

    // 为了简化 OSAL，这里我们让调用者自己遍历所有 fd，或者我们假设调用者知道 wait 返回的数量

    // 更好的方式：edge_poller_wait 内部整理好 active list。

    // 但为了性能，我们直接暴露原始数组的遍历状态。

    // 修正策略：由于 poll 返回的是“就绪个数”，用户不知道是哪几个。

    // 标准做法是遍历整个 fds 数组检查 revents。

    // 在这个 API 中，我们将 index 映射为 fds 数组的下标，让用户层去过滤 revents != 0

    if (p->fds[index].revents == 0) return 0;  // Not ready

    *fd = p->fds[index].fd;

    *events = 0;

    if (p->fds[index].revents & POLLIN) *events |= EDGE_POLL_IN;

    if (p->fds[index].revents & POLLOUT) *events |= EDGE_POLL_OUT;

    if (p->fds[index].revents & POLLERR) *events |= EDGE_POLL_ERR;

    *user_data = p->user_datas[index];

    // 清除 revents 以便下次使用 (虽然 poll 会覆盖，但为了安全)

    p->fds[index].revents = 0;

    return 1;  // Ready
}

/* --- EventFD 实现 --- */

int edge_eventfd_create(void) {
    return eventfd(0, EFD_NONBLOCK);
}

void edge_eventfd_write(int fd) {
    eventfd_write(fd, 1);
}

void edge_eventfd_read(int fd) {
    eventfd_t val;

    eventfd_read(fd, &val);
}

/* --- 时间、日志与基础 I/O --- */

uint64_t edge_now(void) {
    return (uint64_t)k_uptime_get();
}

void edge_os_sleep(uint32_t ms) {
    k_sleep(K_MSEC(ms));
}

void edge_os_log(const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);

    vprintk(fmt, args);

    va_end(args);
}

int edge_os_pipe(int fds[2]) {
    return pipe(fds);
}

int edge_os_read(int fd, void* buf, size_t len) {
    return read(fd, buf, len);
}

int edge_os_write(int fd, const void* buf, size_t len) {
    return write(fd, buf, len);
}
