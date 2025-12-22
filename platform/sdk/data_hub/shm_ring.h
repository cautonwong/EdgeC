#ifndef SHM_RING_H
#define SHM_RING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define CACHE_LINE_SIZE 64 // x86/ARM64 上的典型缓存行大小

typedef struct {
    // ---- 生产者写入区域 ----
    _Atomic size_t head;
    uint8_t pad1[CACHE_LINE_SIZE - sizeof(size_t)];

    // ---- 消费者写入区域 ----
    _Atomic size_t tail;
    uint8_t pad2[CACHE_LINE_SIZE - sizeof(size_t)];

    // ---- 常量配置区 ----
    size_t size;      // 缓冲区元素总数 (必须是 2 的幂)
    size_t mask;      // size - 1
    size_t item_size; // 单个元素的大小

    // 柔性数组，指向数据区的起点
    uint8_t data[];
} shm_ring_t;

/**
 * @brief 初始化共享内存环形缓冲区
 * @param name 共享内存名称，如 "/my_data_hub"
 * @param count 期望的元素个数 (会自动向上取整为2的幂)
 * @param item_size 单个元素的大小
 * @param is_producer true: 作为生产者创建共享内存, false: 作为消费者附加到已存在的共享内存
 * @return 成功时返回 shm_ring_t 指针，失败返回 NULL
 */
shm_ring_t* shm_ring_init(const char* name, size_t count, size_t item_size, bool is_producer);

/**
 * @brief 销毁/分离环形缓冲区
 */
void shm_ring_free(shm_ring_t* ring, const char* name, bool is_producer);

/**
 * @brief 向环形缓冲区推入一个元素 (生产者调用)
 * @return true 成功, false 队列满
 */
bool shm_ring_push(shm_ring_t* ring, const void* item);

/**
 * @brief 从环形缓冲区弹出一个元素 (消费者调用)
 * @return true 成功, false 队列空
 */
bool shm_ring_pop(shm_ring_t* ring, void* item);

#endif /* SHM_RING_H */