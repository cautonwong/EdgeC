#include "shm_ring.h"
#include <stdlib.h>
#include <string.h>

// Helper to check if a number is a power of 2
static int is_power_of_two(uint32_t n) {
    return (n != 0) && ((n & (n - 1)) == 0);
}

int shm_ring_init(shm_ring_t *ring, uint32_t capacity) {
    if (!ring || !is_power_of_two(capacity)) {
        return -1; // Invalid args or capacity not a power of 2
    }

    ring->buffer = malloc(sizeof(PipeMsg) * capacity);
    if (!ring->buffer) {
        return -1; // Malloc failed
    }

    ring->capacity = capacity;
    atomic_init(&ring->head, 0);
    atomic_init(&ring->tail, 0);

    return 0;
}

void shm_ring_destroy(shm_ring_t *ring) {
    if (ring && ring->buffer) {
        free(ring->buffer);
        ring->buffer = NULL;
    }
}

int shm_ring_write(shm_ring_t *ring, const PipeMsg *msg) {
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (((head + 1) & (ring->capacity - 1)) == tail) {
        return -1; // Buffer is full
    }

    memcpy(&ring->buffer[head], msg, sizeof(PipeMsg));

    atomic_store_explicit(&ring->head, (head + 1) & (ring->capacity - 1), memory_order_release);
    return 0;
}

int shm_ring_read(shm_ring_t *ring, uint64_t *seq, PipeMsg *msg) {
    // The 'seq' parameter is ignored to match the markdown's call signature,
    // but the implementation is a simple sequential read.
    (void)seq;

    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);

    if (head == tail) {
        return -1; // Buffer is empty
    }

    memcpy(msg, &ring->buffer[tail], sizeof(PipeMsg));

    atomic_store_explicit(&ring->tail, (tail + 1) & (ring->capacity - 1), memory_order_release);
    return 0;
}
