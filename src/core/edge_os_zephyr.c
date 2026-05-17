/**
 * @file edge_os_zephyr.c
 * @brief Zephyr RTOS HAL Implementation
 */

#include "edge_os_port.h"

#ifdef __ZEPHYR__

#include <zephyr/kernel.h>

// Bounded memory slab for critical allocations
K_MEM_SLAB_DEFINE(edge_mem_slab, 256, 128, 4);

// Wrapper for slab allocation
void *edge_slab_alloc(size_t size) {
    void *ptr = NULL;
    if (size <= 256) {
        k_mem_slab_alloc(&edge_mem_slab, &ptr, K_NO_WAIT);
    }
    return ptr;
}

void edge_slab_free(void *ptr) {
    if (ptr) {
        k_mem_slab_free(&edge_mem_slab, ptr);
    }
}

int edge_mutex_init(edge_mutex_t *mutex) {
    return k_mutex_init(mutex);
}

int edge_mutex_lock(edge_mutex_t *mutex) {
    return k_mutex_lock(mutex, K_FOREVER);
}

int edge_mutex_unlock(edge_mutex_t *mutex) {
    return k_mutex_unlock(mutex);
}

int edge_mutex_destroy(edge_mutex_t *mutex) {
    (void)mutex;
    return 0; // Zephyr doesn't have a destroy function for mutexes
}

// Global thread stack pool for EdgeOS threads
#define MAX_EDGE_THREADS 4
#define EDGE_THREAD_STACK_SIZE 4096
K_THREAD_STACK_ARRAY_DEFINE(edge_thread_stacks, MAX_EDGE_THREADS, EDGE_THREAD_STACK_SIZE);
static int edge_thread_count = 0;

int edge_thread_create(edge_thread_t *thread, void (*entry)(void *), void *arg) {
    if (edge_thread_count >= MAX_EDGE_THREADS) return -1;
    
    k_thread_create(thread, edge_thread_stacks[edge_thread_count],
                    EDGE_THREAD_STACK_SIZE,
                    (k_thread_entry_t)entry, arg, NULL, NULL,
                    K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
                    
    edge_thread_count++;
    return 0;
}

int edge_thread_join(edge_thread_t *thread) {
    return k_thread_join(thread, K_FOREVER);
}

#endif // __ZEPHYR__
