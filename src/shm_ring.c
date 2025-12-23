#include "shm_ring.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdatomic.h>

// 辅助函数：向上取整到2的幂
static size_t roundup_pow_of_two(size_t n) {
    if (n == 0) return 2;
    n--;
    n |= n >> 1; n |= n >> 2; n |= n >> 4;
    n |= n >> 8; n |= n >> 16;
    #if __SIZEOF_SIZE_T__ == 8
    n |= n >> 32;
    #endif
    return n + 1;
}

shm_ring_t* shm_ring_init(const char* name, size_t count, size_t item_size, bool is_producer) {
    int fd;
    size_t ring_count = roundup_pow_of_two(count);
    size_t total_size = sizeof(shm_ring_t) + (ring_count * item_size);

    if (is_producer) {
        shm_unlink(name);
        fd = shm_open(name, O_CREAT | O_RDWR, 0666);
        if (fd < 0) return NULL;
        if (ftruncate(fd, total_size) < 0) { close(fd); return NULL; }
    } else {
        fd = shm_open(name, O_RDWR, 0666);
        if (fd < 0) return NULL;
    }

    void* ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (ptr == MAP_FAILED) return NULL;

    shm_ring_t* ring = (shm_ring_t*)ptr;

    if (is_producer) {
        atomic_init(&ring->head, 0);
        atomic_init(&ring->tail, 0);
        ring->size = ring_count;
        ring->mask = ring_count - 1;
        ring->item_size = item_size;
    }
    return ring;
}

void shm_ring_free(shm_ring_t* ring, const char* name, bool is_producer) {
    if (!ring) return;
    size_t total_size = sizeof(shm_ring_t) + (ring->size * ring->item_size);
    munmap(ring, total_size);
    if (is_producer) {
        shm_unlink(name);
    }
}

bool shm_ring_push(shm_ring_t* ring, const void* item) {
    const size_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    const size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    if (head - tail >= ring->size) {
        return false; // Full
    }
    const size_t offset = (head & ring->mask) * ring->item_size;
    memcpy(ring->data + offset, item, ring->item_size);
    atomic_store_explicit(&ring->head, head + 1, memory_order_release);
    return true;
}

bool shm_ring_pop(shm_ring_t* ring, void* item) {
    const size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    const size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    if (tail >= head) {
        return false; // Empty
    }
    const size_t offset = (tail & ring->mask) * ring->item_size;
    memcpy(item, ring->data + offset, ring->item_size);
    atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
    return true;
}
