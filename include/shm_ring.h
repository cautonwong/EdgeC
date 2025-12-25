#pragma once
#include <stdint.h>
#include <stdatomic.h>

// This is a simplified, in-memory, single-producer, single-consumer ring buffer
// based on the usage pattern in impl_0.md. It is NOT thread-safe for multiple
// producers or consumers.

// Define the message types based on impl_0.md
typedef enum {
    PIPE_SCALAR_F64,
    // Add other types as needed
} PipeMsgType;

// Define the message structure based on impl_0.md
typedef struct {
    PipeMsgType type;
    union {
        double val_f;
    } payload;
} PipeMsg;

// Define the ring buffer structure based on impl_0.md
typedef struct {
    PipeMsg *buffer;
    uint32_t capacity;
    atomic_uint head;
    atomic_uint tail;
} shm_ring_t;

/**
 * @brief Initializes a new in-memory ring buffer.
 *
 * @param ring The ring buffer to initialize.
 * @param capacity The number of slots in the buffer. Must be a power of 2.
 * @return 0 on success, -1 on failure (e.g., memory allocation failed).
 */
int shm_ring_init(shm_ring_t *ring, uint32_t capacity);

/**
 * @brief Frees the resources used by the ring buffer.
 *
 * @param ring The ring buffer to destroy.
 */
void shm_ring_destroy(shm_ring_t *ring);

/**
 * @brief Writes a message to the ring buffer.
 * @note This is the producer function. In a multi-threaded scenario, only one
 *       thread should call this.
 *
 * @param ring The ring buffer.
 * @param msg The message to write.
 * @return 0 on success, -1 if the buffer is full.
 */
int shm_ring_write(shm_ring_t *ring, const PipeMsg *msg);

/**
 * @brief Reads a message from the ring buffer.
 * @note This is the consumer function. In a multi-threaded scenario, only one
 *       thread should call this. The 'seq' parameter from the markdown is
 *       omitted as this is a sequential read, not a seqlock.
 *
 * @param ring The ring buffer.
 * @param seq Ignored (for compatibility with markdown code), can be NULL.
 * @param msg Pointer to store the read message.
 * @return 0 on success, -1 if the buffer is empty.
 */
int shm_ring_read(shm_ring_t *ring, uint64_t *seq, PipeMsg *msg);
