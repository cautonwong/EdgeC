/**
 * @file net_buf.h
 * @brief Network Buffer Library (Ported from Zephyr RTOS)
 *
 * This implementation provides a pool-based memory management scheme
 * optimized for handling network packets and data fragments.
 * It supports zero-copy operations, fragmentation, and user data context.
 */

#ifndef EDGE_NET_BUF_H
#define EDGE_NET_BUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Alignment needed for various fields */
#define __net_buf_align __attribute__((__aligned__(sizeof(void *))))

struct net_buf_pool;

/**
 * @brief Network buffer representation.
 *
 * This struct represents a buffer fragment. It allows for efficient
 * zero-copy manipulation of data (headroom/tailroom).
 */
typedef struct net_buf {
    /** Pointer to the next fragment in the list or chain. */
    struct net_buf *next;

    /** Pointer to the start of the fragment chain (first buffer only). */
    struct net_buf *frags;

    /** The memory pool this buffer belongs to. */
    struct net_buf_pool *pool;

    /** Pointer to the start of data in the buffer. */
    uint8_t *data;

    /** Length of the data currently in the buffer. */
    uint16_t len;

    /** Total size of the data buffer. */
    uint16_t size;

    /** Reference count. */
    uint8_t ref;

    /** Internal flags (used for pool management). */
    uint8_t flags;

    /** User specific data, opaque to the net_buf API. */
    void *user_data;

    /** Start of the data storage (flexible array member). */
    uint8_t __buf_start[];
} net_buf_t;

/**
 * @brief Network buffer pool representation.
 *
 * A pool is a collection of pre-allocated buffers of a fixed size.
 */
typedef struct net_buf_pool {
    /** Lock to protect the free list and internal state. */
    uv_mutex_t lock;

    /** Condition variable for waiting on available buffers. */
    uv_cond_t cond;

    /** List of free buffers. */
    net_buf_t *free_list;

    /** Total number of buffers in the pool. */
    uint16_t buf_count;

    /** Size of each buffer's data area. */
    uint16_t buf_size;

    /** Number of uninitialized buffers remaining in storage. */
    uint16_t uninit_count;

    /** Function called when a buffer is destroyed (optional). */
    void (*destroy_cb)(net_buf_t *buf);

    /** User data for the destroy callback. */
    void *user_data;

    /** Backing storage memory block. */
    uint8_t *storage;

    /** Flag to indicate if storage was dynamically allocated. */
    bool is_dynamic;
} net_buf_pool_t;

/**
 * @brief Initialize a network buffer pool.
 *
 * @param pool Pointer to the pool structure.
 * @param count Number of buffers to allocate.
 * @param size Size of the data payload for each buffer.
 * @param destroy_cb Optional callback invoked when a buffer is returned to the pool.
 * @param user_data User data passed to destroy_cb.
 *
 * @return 0 on success, negative error code on failure.
 */
int net_buf_pool_init(net_buf_pool_t *pool, uint16_t count, uint16_t size,
                      void (*destroy_cb)(net_buf_t *buf), void *user_data);

/**
 * @brief Destroy a network buffer pool.
 *
 * Frees the backing storage and synchronization primitives.
 * CAUTION: Ensure all buffers are returned before calling this.
 *
 * @param pool Pointer to the pool structure.
 */
void net_buf_pool_destroy(net_buf_pool_t *pool);

/**
 * @brief Allocate a new buffer from a pool.
 *
 * @param pool The pool to allocate from.
 * @param timeout_ms Maximum time to wait for a buffer (0 for no wait, -1 for forever).
 *
 * @return Pointer to the allocated buffer, or NULL if timeout occurred.
 */
net_buf_t *net_buf_alloc(net_buf_pool_t *pool, int64_t timeout_ms);

/**
 * @brief Allocate a new buffer with a specific headroom reservation.
 *
 * @param pool The pool to allocate from.
 * @param reserve Amount of headroom to reserve.
 * @param timeout_ms Maximum time to wait.
 *
 * @return Pointer to the allocated buffer, or NULL.
 */
net_buf_t *net_buf_alloc_with_data(net_buf_pool_t *pool, size_t reserve, int64_t timeout_ms);

/**
 * @brief Get a reference to a buffer.
 *
 * Increments the reference count.
 *
 * @param buf The buffer to reference.
 * @return The buffer pointer.
 */
net_buf_t *net_buf_ref(net_buf_t *buf);

/**
 * @brief Release a buffer.
 *
 * Decrements the reference count. If it reaches zero, the buffer is returned to the pool.
 * Recursively unreferences fragments if present.
 *
 * @param buf The buffer to release.
 */
void net_buf_unref(net_buf_t *buf);

/**
 * @brief Add a fragment to the chain.
 *
 * @param head The head of the chain.
 * @param frag The fragment to add.
 */
void net_buf_frag_add(net_buf_t *head, net_buf_t *frag);

/**
 * @brief Insert a fragment into the chain.
 *
 * @param parent The parent buffer (insert after this).
 * @param frag The fragment to insert.
 */
void net_buf_frag_insert(net_buf_t *parent, net_buf_t *frag);

/**
 * @brief Remove a fragment from the chain.
 *
 * @param parent The parent buffer.
 * @param frag The fragment to remove.
 * @return The removed fragment.
 */
net_buf_t *net_buf_frag_del(net_buf_t *parent, net_buf_t *frag);

/**
 * @brief Reserve headroom in the buffer.
 *
 * Adjusts the data pointer to leave space at the beginning.
 * Must be called on an empty buffer.
 *
 * @param buf The buffer.
 * @param reserve Amount of bytes to reserve.
 */
void net_buf_reserve(net_buf_t *buf, size_t reserve);

/**
 * @brief Append data to the end of the buffer.
 *
 * @param buf The buffer.
 * @param len Number of bytes to add.
 * @return Pointer to the start of the added data.
 */
void *net_buf_add(net_buf_t *buf, size_t len);

/**
 * @brief Append a memory block to the buffer.
 *
 * @param buf The buffer.
 * @param mem Pointer to the memory to copy.
 * @param len Length of the memory.
 * @return 0 on success, -1 if not enough space.
 */
int net_buf_add_mem(net_buf_t *buf, const void *mem, size_t len);

/**
 * @brief Prepend data to the beginning of the buffer.
 *
 * @param buf The buffer.
 * @param len Number of bytes to prepend.
 * @return Pointer to the new start of data.
 */
void *net_buf_push(net_buf_t *buf, size_t len);

/**
 * @brief Prepend a memory block.
 *
 * @param buf The buffer.
 * @param mem Pointer to the memory.
 * @param len Length of memory.
 * @return 0 on success, -1 if no headroom.
 */
int net_buf_push_mem(net_buf_t *buf, const void *mem, size_t len);

/**
 * @brief Remove data from the beginning of the buffer.
 *
 * @param buf The buffer.
 * @param len Number of bytes to remove.
 * @return Pointer to the removed data (original start).
 */
void *net_buf_pull(net_buf_t *buf, size_t len);

/**
 * @brief Get amount of free space at the end.
 */
size_t net_buf_tailroom(const net_buf_t *buf);

/**
 * @brief Get amount of free space at the beginning.
 */
size_t net_buf_headroom(const net_buf_t *buf);

/**
 * @brief Get the last fragment in the chain.
 */
net_buf_t *net_buf_frag_last(net_buf_t *buf);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_NET_BUF_H
