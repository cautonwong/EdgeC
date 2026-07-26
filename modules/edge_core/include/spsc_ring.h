#ifndef SPSC_RING_H
#define SPSC_RING_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPSC_RING_CAPACITY 256

typedef struct {
    uint64_t timestamp;
    uint32_t tag_id;
    double value;
    uint32_t flags;
} spsc_event_t;

typedef struct {
    spsc_event_t buffer[SPSC_RING_CAPACITY];
    _Atomic uint32_t head; // Written by Producer (GPIO/AXP209 IRQ)
    _Atomic uint32_t tail; // Written by Consumer (DataHub task)
} spsc_ring_t;

static inline void spsc_ring_init(spsc_ring_t *ring) {
    if (!ring) return;
    atomic_store_explicit(&ring->head, 0, memory_order_relaxed);
    atomic_store_explicit(&ring->tail, 0, memory_order_relaxed);
}

static inline bool spsc_ring_push(spsc_ring_t *ring, const spsc_event_t *event) {
    if (!ring || !event) return false;
    uint32_t current_head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t current_tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (((current_head + 1) % SPSC_RING_CAPACITY) == current_tail) {
        return false; // Ring buffer full
    }

    ring->buffer[current_head] = *event;
    atomic_store_explicit(&ring->head, (current_head + 1) % SPSC_RING_CAPACITY, memory_order_release);
    return true;
}

static inline bool spsc_ring_pop(spsc_ring_t *ring, spsc_event_t *out_event) {
    if (!ring || !out_event) return false;
    uint32_t current_tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t current_head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if (current_tail == current_head) {
        return false; // Ring buffer empty
    }

    *out_event = ring->buffer[current_tail];
    atomic_store_explicit(&ring->tail, (current_tail + 1) % SPSC_RING_CAPACITY, memory_order_release);
    return true;
}

static inline size_t spsc_ring_size(const spsc_ring_t *ring) {
    if (!ring) return 0;
    uint32_t current_head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t current_tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    if (current_head >= current_tail) {
        return current_head - current_tail;
    }
    return SPSC_RING_CAPACITY - (current_tail - current_head);
}

#ifdef __cplusplus
}
#endif

#endif // SPSC_RING_H
