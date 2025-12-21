#include "core_mem.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

#define BLOCK_SIZE 256
#define POOL_COUNT 2000

// 简单的空闲链表栈
static net_buf_t *free_list = NULL;
static pthread_spinlock_t lock;

// 预分配的大块内存
static uint8_t *heap_area = NULL;

int mem_sys_init(void) {
    size_t node_size = sizeof(net_buf_t) + BLOCK_SIZE;
    // 1. 一次性申请大内存，减少碎片
    heap_area = calloc(POOL_COUNT, node_size);
    if (!heap_area) return -1;

    pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

    // 2. 切割并串联到空闲链表
    for (int i = 0; i < POOL_COUNT; i++) {
        net_buf_t *node = (net_buf_t *)(heap_area + i * node_size);
        node->size = BLOCK_SIZE;
        node->next = free_list;
        free_list = node;
    }
    printf("[Mem] Initialized %d blocks of %d bytes.\n", POOL_COUNT, BLOCK_SIZE);
    return 0;
}

net_buf_t *net_buf_alloc(void) {
    pthread_spin_lock(&lock);
    if (!free_list) {
        pthread_spin_unlock(&lock);
        return NULL; // OOM (Out of Memory)
    }
    net_buf_t *buf = free_list;
    free_list = buf->next;
    pthread_spin_unlock(&lock);

    // Reset 状态
    buf->next = NULL;
    buf->ref = 1;
    buf->data = buf->_storage; // 指向柔性数组开始
    buf->len = 0;
    return buf;
}

void net_buf_unref(net_buf_t *buf) {
    if (!buf) return;
    
    pthread_spin_lock(&lock);
    if (--buf->ref == 0) {
        buf->next = free_list;
        free_list = buf;
    }
    pthread_spin_unlock(&lock);
}

net_buf_t *net_buf_ref(net_buf_t *buf) {
    if (!buf) return NULL;
    pthread_spin_lock(&lock);
    buf->ref++;
    pthread_spin_unlock(&lock);
    return buf;
}

void *net_buf_add(net_buf_t *buf, const void *data, size_t len) {
    if (buf->len + len > buf->size) return NULL;
    void *ptr = buf->data + buf->len;
    memcpy(ptr, data, len);
    buf->len += len;
    return ptr;
}
