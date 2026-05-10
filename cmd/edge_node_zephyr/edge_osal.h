#ifndef EDGE_OSAL_H
#define EDGE_OSAL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/* --- 容器检索宏 (抹平 CONTAINER_OF 差异) --- */
#define edge_container_of(ptr, type, member) ((type*)((char*)(ptr)-offsetof(type, member)))

/* --- 类型抽象 --- */
typedef struct edge_mutex_s* edge_mutex_t;
typedef struct edge_sem_s* edge_sem_t;
typedef struct edge_thread_t* edge_thread_id_t;
typedef struct edge_poller_s* edge_poller_t;

/* --- 系统常量 --- */
#define EDGE_OS_MAX_POLL_FDS 64

/* --- 互斥锁 --- */
edge_mutex_t edge_mutex_create(void);
void edge_mutex_lock(edge_mutex_t mutex);
void edge_mutex_unlock(edge_mutex_t mutex);

/* --- 信号量 --- */
edge_sem_t edge_sem_create(uint32_t initial_count);
void edge_sem_give(edge_sem_t sem);
void edge_sem_take(edge_sem_t sem);

/* --- 时间与 Tick --- */
uint64_t edge_now(void);
void edge_os_sleep(uint32_t ms);

/* --- 系统 I/O & 日志 --- */
void edge_os_log(const char* fmt, ...);
int edge_os_pipe(int fds[2]);
int edge_os_read(int fd, void* buf, size_t len);
int edge_os_write(int fd, const void* buf, size_t len);

/* --- 线程管理 --- */
typedef void (*edge_thread_entry_t)(void* arg);
edge_thread_id_t edge_thread_create(const char* name, edge_thread_entry_t entry, void* arg,
                                    int priority, size_t stack_size);

/* --- 原子操作 --- */
int32_t edge_atomic_cas(volatile int32_t* ptr, int32_t expected, int32_t desired);
void edge_atomic_set(volatile int32_t* ptr, int32_t val);
int32_t edge_atomic_get(volatile int32_t* ptr);

/* --- I/O 多路复用 (Poller) --- */
#define EDGE_POLL_IN 0x01
#define EDGE_POLL_OUT 0x02
#define EDGE_POLL_ERR 0x04

edge_poller_t edge_poller_create(void);
int edge_poller_ctl(edge_poller_t p, int fd, int events, void* user_data);
int edge_poller_wait(edge_poller_t p, int timeout_ms);
int edge_poller_get_event(edge_poller_t p, int index, int* fd, int* events, void** user_data);

/* --- EventFD --- */
int edge_eventfd_create(void);
void edge_eventfd_write(int fd);
void edge_eventfd_read(int fd);

#endif