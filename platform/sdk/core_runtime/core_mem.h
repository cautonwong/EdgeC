#ifndef CORE_MEM_H
#define CORE_MEM_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

/**
 * @brief 核心网络缓冲区结构 (net_buf)
 *
 * 借鉴 Zephyr net_buf 设计，为协议栈零拷贝操作优化。
 * 包含引用计数，支持高效的 Pub/Sub 数据分发。
 */
typedef struct net_buf {
    union {
        struct net_buf *next; // 用于构成链表，被内存池或队列使用
        void *_reserved;
    };

    // 引用计数，当减至 0 时，buf 被归还给内存池
    atomic_int ref;

    // --- 核心指针 ---
    uint8_t *buffer_start; // 指向所分配内存块的物理起点 (固定)
    uint8_t *data;         // 指向当前有效数据的逻辑起点 (可变)
    uint16_t len;          // 当前有效数据的长度
    uint16_t size;         // 内存块的总容量 (固定)

    uint8_t pool_id;       // 标识此 buf 属于哪个内存池
} net_buf_t;

/**
 * @brief 初始化内存管理系统
 *
 * 创建多个不同规格的内存池 (e.g., 256B, 4KB)。
 * @return 0 on success, -1 on error.
 */
int core_mem_init(void);

/**
 * @brief 从指定 ID 的内存池中分配一个 net_buf
 *
 * @param pool_id 内存池 ID
 * @return 指向 net_buf 的指针，如果池满则返回 NULL
 */
net_buf_t* core_net_buf_alloc(uint8_t pool_id);

/**
 * @brief 增加 net_buf 的引用计数
 *
 * @param buf 目标 net_buf
 * @return 传入的 buf 指针
 */
net_buf_t* core_net_buf_ref(net_buf_t *buf);

/**
 * @brief 减少 net_buf 的引用计数
 *
 * 当引用计数降为 0 时，buf 会被自动归还到其所属的内存池。
 * @param buf 目标 net_buf
 */
void core_net_buf_unref(net_buf_t *buf);

/**
 * @brief 在数据区头部增加空间 (用于添加协议头)
 *
 * 此操作不移动任何内存，仅移动 data 指针。
 * @param buf 目标 net_buf
 * @param len 要增加的长度
 * @return 指向新头部起点的指针，失败返回 NULL
 */
void* core_net_buf_push(net_buf_t *buf, size_t len);

/**
 * @brief 从数据区头部移除数据 (用于剥离协议头)
 *
 * @param buf 目标 net_buf
 * @param len 要移除的长度
 * @return 指向原头部起点的指针，失败返回 NULL
 */
void* core_net_buf_pull(net_buf_t *buf, size_t len);

/**
 * @brief 在数据区尾部增加空间，并返回可写入的指针
 *
 * @param buf 目标 net_buf
 * @param len 要增加的长度
 * @return 指向新增空间起点的指针，失败返回 NULL
 */
void* core_net_buf_add(net_buf_t *buf, size_t len);

/**
 * @brief 向数据区尾部拷贝数据
 *
 * @param buf 目标 net_buf
 * @param data 要拷贝的数据源
 * @param len 数据长度
 * @return 0 on success, -1 on error.
 */
int core_net_buf_append(net_buf_t *buf, const void *data, size_t len);

#endif /* CORE_MEM_H */
