#ifndef SHM_RING_H
#define SHM_RING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Cache line size, typically 64 bytes on modern CPUs (x86/ARM64)
#define CACHE_LINE_SIZE 64

typedef struct {
    // ---- Producer-only cache line ----
    _Atomic size_t head;
    uint8_t pad1[CACHE_LINE_SIZE - sizeof(size_t)];

    // ---- Consumer-only cache line ----
    _Atomic size_t tail;
    uint8_t pad2[CACHE_LINE_SIZE - sizeof(size_t)];

    // ---- Constant configuration ----
    size_t size;      // Total capacity of the buffer (must be a power of 2)
    size_t mask;      // (size - 1), for fast modulo operations
    size_t item_size; // Size of a single element in bytes
    
    // Flexible array member for the actual data buffer
    uint8_t data[];   
} shm_ring_t;

/**
 * @brief Initializes or attaches to a shared memory ring buffer.
 * 
 * @param name The name of the shared memory object (e.g., "/my_ring_buffer").
 * @param count The number of elements the buffer should hold (will be rounded up to the next power of 2).
 * @param item_size The size of a single element.
 * @param is_producer True if this process is creating the buffer, false if it's a consumer attaching to an existing one.
 * @return A pointer to the mapped shared memory ring buffer, or NULL on failure.
 */
shm_ring_t* shm_ring_init(const char* name, size_t count, size_t item_size, bool is_producer);

/**
 * @brief Unmaps and optionally destroys the shared memory ring buffer.
 * 
 * @param ring A pointer to the ring buffer.
 * @param name The name of the shared memory object.
 * @param is_producer If true, the underlying shared memory object will be unlinked (destroyed).
 */
void shm_ring_free(shm_ring_t* ring, const char* name, bool is_producer);

/**
 * @brief Pushes a single item into the ring buffer. (Producer only)
 * 
 * This is a lock-free, single-producer-safe operation.
 * 
 * @param ring A pointer to the ring buffer.
 * @param data A pointer to the data item to be copied into the buffer.
 * @return true on success, false if the buffer is full.
 */
bool shm_ring_push(shm_ring_t* ring, const void* data);

/**
 * @brief Pops a single item from the ring buffer. (Consumer only)
 * 
 * This is a lock-free, single-consumer-safe operation.
 * 
 * @param ring A pointer to the ring buffer.
 * @param data A pointer to a buffer where the popped item will be copied.
 * @return true on success, false if the buffer is empty.
 */
bool shm_ring_pop(shm_ring_t* ring, void* data);

/**
 * @brief Gets the current number of items in the buffer.
 * 
 * Note: This value can be stale immediately after being read in a concurrent environment.
 * 
 * @param ring A pointer to the ring buffer.
 * @return The number of items currently in the buffer.
 */
size_t shm_ring_count(shm_ring_t* ring);

#endif // SHM_RING_H
