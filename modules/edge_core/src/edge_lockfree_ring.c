/**
 * @file edge_lockfree_ring.c
 * @brief Lock-Free Zero-Copy RingBuffer Channel for Real-Time Producer-Consumer (AEP-717 / Issue #248)
 */

#include "edge_lockfree_ring.h"

#include <errno.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct edge_lockfree_ring {
    edge_lockfree_ring_config_t config;
    edge_lockfree_ring_port_t port;
    uint32_t mask;
    uint32_t elem_size;

    _Atomic uint64_t head;
    _Atomic uint64_t tail;

    _Atomic uint64_t sequence[EDGE_LOCKFREE_RING_MAX_CAPACITY];
    uint8_t buffer[EDGE_LOCKFREE_RING_MAX_CAPACITY * EDGE_LOCKFREE_RING_MAX_ELEM_SIZE];

    _Atomic uint64_t total_enqueued;
    _Atomic uint64_t total_dequeued;
    _Atomic uint64_t total_dropped;
    _Atomic uint32_t watermark_peak;

    bool initialized;
};

_Static_assert(sizeof(struct edge_lockfree_ring) <= sizeof(edge_lockfree_ring_storage_t),
               "edge_lockfree_ring_storage_t size is insufficient for struct edge_lockfree_ring");

static inline bool is_power_of_two(uint32_t v) {
    return v > 0 && (v & (v - 1)) == 0;
}

edge_lockfree_ring_t *edge_lockfree_ring_create_in(edge_lockfree_ring_storage_t *storage,
                                                   const edge_lockfree_ring_config_t *cfg,
                                                   const edge_lockfree_ring_port_t *port) {
    if (!storage || !cfg) {
        return NULL;
    }

    uint32_t cap = cfg->capacity;
    if (cap == 0 || cap > EDGE_LOCKFREE_RING_MAX_CAPACITY || !is_power_of_two(cap)) {
        return NULL;
    }

    uint32_t elem_sz = cfg->elem_size_bytes;
    if (elem_sz == 0 || elem_sz > EDGE_LOCKFREE_RING_MAX_ELEM_SIZE) {
        return NULL;
    }

    struct edge_lockfree_ring *ring = (struct edge_lockfree_ring *)storage->raw;
    memset(ring, 0x00, sizeof(*ring));
    ring->config = *cfg;
    if (port) {
        ring->port = *port;
    }

    ring->mask = cap - 1;
    ring->elem_size = elem_sz;
    atomic_init(&ring->head, 0);
    atomic_init(&ring->tail, 0);
    atomic_init(&ring->total_enqueued, 0);
    atomic_init(&ring->total_dequeued, 0);
    atomic_init(&ring->total_dropped, 0);
    atomic_init(&ring->watermark_peak, 0);

    for (uint32_t i = 0; i < cap; i++) {
        atomic_init(&ring->sequence[i], (uint64_t)i);
    }

    ring->initialized = true;
    return ring;
}

int edge_lockfree_ring_destroy(edge_lockfree_ring_t *ring) {
    if (!ring || !ring->initialized) {
        return -EINVAL;
    }
    ring->initialized = false;
    return 0;
}

static void check_watermark(edge_lockfree_ring_t *ring, uint32_t depth) {
    uint32_t peak = atomic_load_explicit(&ring->watermark_peak, memory_order_relaxed);
    while (depth > peak) {
        if (atomic_compare_exchange_weak_explicit(&ring->watermark_peak, &peak, depth,
                                                  memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }

    if (ring->config.high_watermark_percent > 0) {
        uint32_t threshold = (ring->config.capacity * ring->config.high_watermark_percent) / 100;
        if (depth >= threshold && ring->port.on_watermark_high) {
            ring->port.on_watermark_high(ring->port.ctx, depth, ring->config.capacity);
        }
    }
}

int edge_lockfree_ring_enqueue(edge_lockfree_ring_t *ring, const void *elem) {
    if (!ring || !ring->initialized || !elem) {
        return -EINVAL;
    }

    if (ring->config.mode == EDGE_RING_MODE_SPSC) {
        uint64_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
        uint64_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

        if ((head - tail) >= ring->config.capacity) {
            if (ring->config.overflow_policy == EDGE_RING_OVERFLOW_OVERWRITE) {
                atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
            } else {
                uint64_t drops = atomic_fetch_add_explicit(&ring->total_dropped, 1, memory_order_relaxed) + 1;
                if (ring->port.on_overflow) {
                    ring->port.on_overflow(ring->port.ctx, drops, "SPSC queue full");
                }
                return -ENOSPC;
            }
        }

        uint8_t *slot = &ring->buffer[(head & ring->mask) * ring->elem_size];
        memcpy(slot, elem, ring->elem_size);
        atomic_store_explicit(&ring->head, head + 1, memory_order_release);
        atomic_fetch_add_explicit(&ring->total_enqueued, 1, memory_order_relaxed);

        check_watermark(ring, (uint32_t)(head + 1 - tail));
        return 0;
    }

    // MPMC mode (Vyukov Bounded Lock-Free Queue)
    uint64_t pos = atomic_load_explicit(&ring->head, memory_order_relaxed);
    for (;;) {
        _Atomic uint64_t *cell = &ring->sequence[pos & ring->mask];
        uint64_t seq = atomic_load_explicit(cell, memory_order_acquire);
        int64_t diff = (int64_t)seq - (int64_t)pos;

        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&ring->head, &pos, pos + 1,
                                                      memory_order_relaxed, memory_order_relaxed)) {
                uint8_t *slot = &ring->buffer[(pos & ring->mask) * ring->elem_size];
                memcpy(slot, elem, ring->elem_size);
                atomic_store_explicit(cell, pos + 1, memory_order_release);
                atomic_fetch_add_explicit(&ring->total_enqueued, 1, memory_order_relaxed);

                uint64_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
                if (pos >= tail) {
                    check_watermark(ring, (uint32_t)(pos + 1 - tail));
                }
                return 0;
            }
        } else if (diff < 0) {
            // Queue is full
            uint64_t drops = atomic_fetch_add_explicit(&ring->total_dropped, 1, memory_order_relaxed) + 1;
            if (ring->port.on_overflow) {
                ring->port.on_overflow(ring->port.ctx, drops, "MPMC queue full");
            }
            return -ENOSPC;
        } else {
            pos = atomic_load_explicit(&ring->head, memory_order_relaxed);
        }
    }
}

int edge_lockfree_ring_dequeue(edge_lockfree_ring_t *ring, void *out_elem) {
    if (!ring || !ring->initialized || !out_elem) {
        return -EINVAL;
    }

    if (ring->config.mode == EDGE_RING_MODE_SPSC) {
        uint64_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
        uint64_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

        if (tail >= head) {
            return -ENOENT; // Empty
        }

        const uint8_t *slot = &ring->buffer[(tail & ring->mask) * ring->elem_size];
        memcpy(out_elem, slot, ring->elem_size);
        atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
        atomic_fetch_add_explicit(&ring->total_dequeued, 1, memory_order_relaxed);
        return 0;
    }

    // MPMC mode
    uint64_t pos = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    for (;;) {
        _Atomic uint64_t *cell = &ring->sequence[pos & ring->mask];
        uint64_t seq = atomic_load_explicit(cell, memory_order_acquire);
        int64_t diff = (int64_t)seq - (int64_t)(pos + 1);

        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&ring->tail, &pos, pos + 1,
                                                      memory_order_relaxed, memory_order_relaxed)) {
                const uint8_t *slot = &ring->buffer[(pos & ring->mask) * ring->elem_size];
                memcpy(out_elem, slot, ring->elem_size);
                atomic_store_explicit(cell, pos + ring->mask + 1, memory_order_release);
                atomic_fetch_add_explicit(&ring->total_dequeued, 1, memory_order_relaxed);
                return 0;
            }
        } else if (diff < 0) {
            return -ENOENT; // Empty
        } else {
            pos = atomic_load_explicit(&ring->tail, memory_order_relaxed);
        }
    }
}

int edge_lockfree_ring_reserve(edge_lockfree_ring_t *ring, void **out_slot_ptr, uint64_t *out_ticket) {
    if (!ring || !ring->initialized || !out_slot_ptr || !out_ticket) {
        return -EINVAL;
    }

    if (ring->config.mode == EDGE_RING_MODE_SPSC) {
        uint64_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
        uint64_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

        if ((head - tail) >= ring->config.capacity) {
            uint64_t drops = atomic_fetch_add_explicit(&ring->total_dropped, 1, memory_order_relaxed) + 1;
            if (ring->port.on_overflow) {
                ring->port.on_overflow(ring->port.ctx, drops, "SPSC reserve full");
            }
            return -ENOSPC;
        }

        *out_slot_ptr = &ring->buffer[(head & ring->mask) * ring->elem_size];
        *out_ticket = head;
        return 0;
    }

    // MPMC reserve
    uint64_t pos = atomic_load_explicit(&ring->head, memory_order_relaxed);
    for (;;) {
        _Atomic uint64_t *cell = &ring->sequence[pos & ring->mask];
        uint64_t seq = atomic_load_explicit(cell, memory_order_acquire);
        int64_t diff = (int64_t)seq - (int64_t)pos;

        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&ring->head, &pos, pos + 1,
                                                      memory_order_relaxed, memory_order_relaxed)) {
                *out_slot_ptr = &ring->buffer[(pos & ring->mask) * ring->elem_size];
                *out_ticket = pos;
                return 0;
            }
        } else if (diff < 0) {
            return -ENOSPC;
        } else {
            pos = atomic_load_explicit(&ring->head, memory_order_relaxed);
        }
    }
}

int edge_lockfree_ring_commit(edge_lockfree_ring_t *ring, uint64_t ticket) {
    if (!ring || !ring->initialized) {
        return -EINVAL;
    }

    if (ring->config.mode == EDGE_RING_MODE_SPSC) {
        atomic_store_explicit(&ring->head, ticket + 1, memory_order_release);
        atomic_fetch_add_explicit(&ring->total_enqueued, 1, memory_order_relaxed);
        return 0;
    }

    _Atomic uint64_t *cell = &ring->sequence[ticket & ring->mask];
    atomic_store_explicit(cell, ticket + 1, memory_order_release);
    atomic_fetch_add_explicit(&ring->total_enqueued, 1, memory_order_relaxed);
    return 0;
}

int edge_lockfree_ring_peek(edge_lockfree_ring_t *ring, const void **out_slot_ptr) {
    if (!ring || !ring->initialized || !out_slot_ptr) {
        return -EINVAL;
    }

    uint64_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint64_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if (tail >= head) {
        return -ENOENT;
    }

    *out_slot_ptr = &ring->buffer[(tail & ring->mask) * ring->elem_size];
    return 0;
}

int edge_lockfree_ring_advance(edge_lockfree_ring_t *ring) {
    if (!ring || !ring->initialized) {
        return -EINVAL;
    }

    if (ring->config.mode == EDGE_RING_MODE_SPSC) {
        uint64_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
        uint64_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
        if (tail >= head) {
            return -ENOENT;
        }
        atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
        atomic_fetch_add_explicit(&ring->total_dequeued, 1, memory_order_relaxed);
        return 0;
    }

    // MPMC advance
    uint64_t pos = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    for (;;) {
        _Atomic uint64_t *cell = &ring->sequence[pos & ring->mask];
        uint64_t seq = atomic_load_explicit(cell, memory_order_acquire);
        int64_t diff = (int64_t)seq - (int64_t)(pos + 1);

        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&ring->tail, &pos, pos + 1,
                                                      memory_order_relaxed, memory_order_relaxed)) {
                atomic_store_explicit(cell, pos + ring->mask + 1, memory_order_release);
                atomic_fetch_add_explicit(&ring->total_dequeued, 1, memory_order_relaxed);
                return 0;
            }
        } else if (diff < 0) {
            return -ENOENT;
        } else {
            pos = atomic_load_explicit(&ring->tail, memory_order_relaxed);
        }
    }
}

uint32_t edge_lockfree_ring_depth(const edge_lockfree_ring_t *ring) {
    if (!ring || !ring->initialized) {
        return 0;
    }
    uint64_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint64_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    return (head >= tail) ? (uint32_t)(head - tail) : 0;
}

uint32_t edge_lockfree_ring_capacity(const edge_lockfree_ring_t *ring) {
    return ring ? ring->config.capacity : 0;
}

int edge_lockfree_ring_get_stats(const edge_lockfree_ring_t *ring, edge_lockfree_ring_stats_t *stats) {
    if (!ring || !stats || !ring->initialized) {
        return -EINVAL;
    }

    stats->capacity = ring->config.capacity;
    stats->elem_size_bytes = ring->elem_size;
    stats->current_depth = edge_lockfree_ring_depth(ring);
    stats->watermark_peak = atomic_load_explicit(&ring->watermark_peak, memory_order_relaxed);
    stats->total_enqueued = atomic_load_explicit(&ring->total_enqueued, memory_order_relaxed);
    stats->total_dequeued = atomic_load_explicit(&ring->total_dequeued, memory_order_relaxed);
    stats->total_dropped = atomic_load_explicit(&ring->total_dropped, memory_order_relaxed);
    stats->mode = ring->config.mode;

    return 0;
}
