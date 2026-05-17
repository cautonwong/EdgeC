/**
 * @file edge_os_port.h
 * @brief EdgeOS Operating System Abstraction Layer (HAL)
 */

#ifndef EDGE_OS_PORT_H
#define EDGE_OS_PORT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __ZEPHYR__

#include <zephyr/kernel.h>

/* --- Zephyr RTOS Abstraction --- */
#define EDGE_MALLOC(size)       k_malloc(size)
#define EDGE_CALLOC(n, size)    k_calloc(n, size)
#define EDGE_FREE(ptr)          k_free(ptr)

typedef struct k_mutex edge_mutex_t;
typedef struct k_thread edge_thread_t;

/* Zephyr Thread Wrapper */
int edge_mutex_init(edge_mutex_t *mutex);
int edge_mutex_lock(edge_mutex_t *mutex);
int edge_mutex_unlock(edge_mutex_t *mutex);
int edge_mutex_destroy(edge_mutex_t *mutex);

int edge_thread_create(edge_thread_t *thread, void (*entry)(void *), void *arg);
int edge_thread_join(edge_thread_t *thread);

#else

/* --- Linux / POSIX (libuv) Abstraction --- */
#include <stdlib.h>
#include <uv.h>

#define EDGE_MALLOC(size)       malloc(size)
#define EDGE_CALLOC(n, size)    calloc(n, size)
#define EDGE_FREE(ptr)          free(ptr)

typedef uv_mutex_t edge_mutex_t;
typedef uv_thread_t edge_thread_t;

/* POSIX Thread Wrapper */
static inline int edge_mutex_init(edge_mutex_t *mutex) {
    return uv_mutex_init(mutex) == 0 ? 0 : -1;
}

static inline int edge_mutex_lock(edge_mutex_t *mutex) {
    uv_mutex_lock(mutex);
    return 0;
}

static inline int edge_mutex_unlock(edge_mutex_t *mutex) {
    uv_mutex_unlock(mutex);
    return 0;
}

static inline int edge_mutex_destroy(edge_mutex_t *mutex) {
    uv_mutex_destroy(mutex);
    return 0;
}

static inline int edge_thread_create(edge_thread_t *thread, void (*entry)(void *), void *arg) {
    return uv_thread_create(thread, entry, arg) == 0 ? 0 : -1;
}

static inline int edge_thread_join(edge_thread_t *thread) {
    return uv_thread_join(thread) == 0 ? 0 : -1;
}

#endif

#endif // EDGE_OS_PORT_H
