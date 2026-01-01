#include "shmring.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"

int shmring_create(shm_ring_t *ring, const char *name, uint32_t slots) {
    if (!ring || !name || slots == 0 || slots > SHMRING_MAX_SLOTS) {
        LOG_ERROR(LOG_CAT_BUS, "Invalid SHM parameters for '%s'", name ? name : "NULL");
        return -1;
    }

    size_t size = sizeof(ShmRingHeader) + slots * sizeof(PipeMsg);
    shm_unlink(name);

    int fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, 0666);
    if (fd < 0) {
        LOG_ERROR(LOG_CAT_BUS, "shm_open failed: %s", strerror(errno));
        return -1;
    }

    if (ftruncate(fd, (off_t)size) < 0) {
        LOG_ERROR(LOG_CAT_BUS, "ftruncate failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (ptr == MAP_FAILED) {
        LOG_ERROR(LOG_CAT_BUS, "mmap failed: %s", strerror(errno));
        return -1;
    }

    ring->hdr = (ShmRingHeader *)ptr;
    ring->slots = (PipeMsg *)((uint8_t *)ptr + sizeof(ShmRingHeader));
    ring->map_size = size;
    strncpy(ring->name, name, sizeof(ring->name) - 1);

    // [卓越设计] 显式初始化原子序列
    ring->hdr->magic = SHMRING_MAGIC;
    ring->hdr->slot_count = slots;
    atomic_init(&ring->hdr->seq, 0);
    atomic_init(&ring->hdr->write_idx, 0);
    atomic_init(&ring->hdr->total_writes, 0);

    LOG_INFO(LOG_CAT_BUS, "Lock-free SHM Ring '%s' created (Slots: %u)", name, slots);
    return 0;
}

int shmring_open(shm_ring_t *ring, const char *name) {
    if (!ring || !name) return -1;

    int fd = shm_open(name, O_RDWR, 0666);
    if (fd < 0) return -1;

    ShmRingHeader header;
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return -1;
    }

    if (header.magic != SHMRING_MAGIC) {
        LOG_ERROR(LOG_CAT_BUS, "SHM Magic mismatch");
        close(fd);
        return -1;
    }

    size_t size = sizeof(ShmRingHeader) + header.slot_count * sizeof(PipeMsg);
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (ptr == MAP_FAILED) return -1;

    ring->hdr = (ShmRingHeader *)ptr;
    ring->slots = (PipeMsg *)((uint8_t *)ptr + sizeof(ShmRingHeader));
    ring->map_size = size;
    strncpy(ring->name, name, sizeof(ring->name) - 1);

    return 0;
}

void shmring_close(shm_ring_t *ring) {
    if (ring && ring->hdr) {
        munmap(ring->hdr, ring->map_size);
        ring->hdr = NULL;
        ring->slots = NULL;
    }
}

int shmring_destroy(shm_ring_t *ring) {
    if (!ring) return -1;
    char name[256];
    strncpy(name, ring->name, 255);
    shmring_close(ring);
    shm_unlink(name);
    LOG_INFO(LOG_CAT_BUS, "SHM Ring '%s' physical storage removed", name);
    return 0;
}

void shm_ring_write(shm_ring_t *ring, const PipeMsg *msg) {
    if (!ring || !ring->hdr || !msg) return;

    // 1. 原子预占槽位 (Relaxed because order doesn't matter yet)
    uint32_t idx = atomic_fetch_add_explicit(&ring->hdr->write_idx, 1, memory_order_relaxed) %
                   ring->hdr->slot_count;

    // 2. 物理写入
    PipeMsg *target = &ring->slots[idx];
    *target = *msg;

    // 3. 发布可见性 (Release Barrier)
    // 此处确保所有先前的写操作(数据拷贝)在 seq 增加前完成
    atomic_fetch_add_explicit(&ring->hdr->seq, 1, memory_order_release);
    atomic_fetch_add_explicit(&ring->hdr->total_writes, 1, memory_order_relaxed);
}

int shm_ring_read(shm_ring_t *ring, uint64_t *last_seq, PipeMsg *out) {
    if (!ring || !ring->hdr || !out) return -1;

    // 1. 获取最新可见序列号 (Acquire Barrier)
    // 确证其后的读操作不会被重排到此操作之前
    uint64_t head = (uint64_t)atomic_load_explicit(&ring->hdr->seq, memory_order_acquire);
    if (head <= *last_seq) return -1;

    // 2. 追赶处理 (工业级鲁棒性：防止过慢读者被写者覆盖)
    if (head > *last_seq + ring->hdr->slot_count) {
        *last_seq = head - ring->hdr->slot_count;
    }

    // 3. 物理拷贝
    uint32_t idx = (uint32_t)((*last_seq) % ring->hdr->slot_count);
    *out = ring->slots[idx];
    (*last_seq)++;

    return 0;
}