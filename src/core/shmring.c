#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "shmring.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"

// 兼容旧内核的 memfd 定义
#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif
#ifndef MFD_ALLOW_SEALING
#define MFD_ALLOW_SEALING 0x0002U
#endif
#ifndef F_SEAL_SEAL
#define F_SEAL_SEAL 0x0001
#endif
#ifndef F_SEAL_SHRINK
#define F_SEAL_SHRINK 0x0002
#endif
#ifndef F_SEAL_GROW
#define F_SEAL_GROW 0x0004
#endif

int shmring_create(shm_ring_t *ring, const char *name, uint32_t slots) {
    if (!ring || slots == 0 || slots > SHMRING_MAX_SLOTS) {
        return -1;
    }

    // 1. 物理计算：Page 对齐
    size_t size = sizeof(ShmRingHeader) + (size_t)slots * sizeof(PipeMsg);
    size = (size + 4095) & ~4095UL;

    // 2. 创建匿名内存 (memfd)
    const char *mem_name = name ? name : "edge_bus";
    int fd = (int)syscall(SYS_memfd_create, mem_name, MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) {
        LOG_ERROR(LOG_CAT_BUS, "memfd_create failed: %s", strerror(errno));
        return -1;
    }

    if (ftruncate(fd, (off_t)size) < 0) {
        LOG_ERROR(LOG_CAT_BUS, "ftruncate failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    // 3. 内核级密封 (Sealing)
    fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL);

    // 4. 建立映射
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        LOG_ERROR(LOG_CAT_BUS, "mmap failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    ring->fd = fd;
    ring->hdr = (ShmRingHeader *)ptr;
    ring->slots = (PipeMsg *)((uint8_t *)ptr + sizeof(ShmRingHeader));
    ring->map_size = size;
    if (name) {
        strncpy(ring->name, name, sizeof(ring->name) - 1);
    } else {
        ring->name[0] = '\0';
    }

    // 5. 初始化元数据
    ring->hdr->magic = SHMRING_MAGIC;
    ring->hdr->slot_count = slots;
    atomic_store_explicit(&ring->hdr->seq, 0, memory_order_relaxed);
    atomic_store_explicit(&ring->hdr->write_idx, 0, memory_order_relaxed);
    atomic_store_explicit(&ring->hdr->total_writes, 0, memory_order_relaxed);

    LOG_INFO(LOG_CAT_BUS, "Sealed memfd Ring created (FD: %d, Size: %zu KB)", fd, size / 1024);
    return 0;
}

int shmring_init_from_fd(shm_ring_t *ring, int fd, size_t size) {
    if (!ring || fd < 0 || size == 0) {
        return -1;
    }

    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        LOG_ERROR(LOG_CAT_BUS, "mmap from FD failed: %s", strerror(errno));
        return -1;
    }

    ring->fd = fd;
    ring->hdr = (ShmRingHeader *)ptr;
    ring->slots = (PipeMsg *)((uint8_t *)ptr + sizeof(ShmRingHeader));
    ring->map_size = size;
    ring->name[0] = '\0';

    if (ring->hdr->magic != SHMRING_MAGIC) {
        LOG_ERROR(LOG_CAT_BUS, "ShmRing magic mismatch from FD: 0x%X", ring->hdr->magic);
        munmap(ptr, size);
        return -1;
    }

    return 0;
}

int shmring_get_fd(shm_ring_t *ring) {
    return ring ? ring->fd : -1;
}

int shmring_open(shm_ring_t *ring, const char *name) {
    (void)ring;
    (void)name;
    LOG_WARN(LOG_CAT_BUS, "shmring_open is deprecated for anonymous memfd");
    return -1;
}

void shmring_close(shm_ring_t *ring) {
    if (ring && ring->hdr) {
        munmap(ring->hdr, ring->map_size);
        if (ring->fd >= 0) {
            close(ring->fd);
            ring->fd = -1;
        }
        ring->hdr = NULL;
        ring->slots = NULL;
    }
}

int shmring_destroy(shm_ring_t *ring) {
    if (!ring) return -1;
    shmring_close(ring);
    return 0;
}

void shm_ring_write(shm_ring_t *ring, const PipeMsg *msg) {
    if (!ring || !ring->hdr || !msg) return;

    // 10kHz 优化：使用原子操作预占索引
    uint32_t idx = atomic_fetch_add_explicit(&ring->hdr->write_idx, 1, memory_order_relaxed) %
                   ring->hdr->slot_count;

    // 物理写入 (DMA 友好型内存)
    ring->slots[idx] = *msg;

    // 屏障确保存储可见性
    atomic_fetch_add_explicit(&ring->hdr->seq, 1, memory_order_release);
    atomic_fetch_add_explicit(&ring->hdr->total_writes, 1, memory_order_relaxed);
}

int shm_ring_read(shm_ring_t *ring, uint64_t *last_seq, PipeMsg *out) {
    if (!ring || !ring->hdr || !out) return -1;

    uint64_t head = (uint64_t)atomic_load_explicit(&ring->hdr->seq, memory_order_acquire);
    if (head <= *last_seq) return -1;

    // 自动追赶逻辑
    if (head > *last_seq + ring->hdr->slot_count) {
        *last_seq = head - ring->hdr->slot_count;
    }

    uint32_t idx = (uint32_t)((*last_seq) % ring->hdr->slot_count);
    *out = ring->slots[idx];
    (*last_seq)++;

    return 0;
}

#include <sys/socket.h>
#include <sys/un.h>

int edge_fd_send(int sock_fd, int fd_to_send) {
    struct msghdr msg = {0};
    char buf[CMSG_SPACE(sizeof(int))];
    memset(buf, 0, sizeof(buf));

    struct iovec io = {.iov_base = "FD", .iov_len = 2};

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    *((int *)CMSG_DATA(cmsg)) = fd_to_send;

    if (sendmsg(sock_fd, &msg, 0) < 0) {
        LOG_ERROR(LOG_CAT_BUS, "edge_fd_send failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int edge_fd_recv(int sock_fd) {
    struct msghdr msg = {0};
    char buf[CMSG_SPACE(sizeof(int))];
    struct iovec io = {.iov_base = buf, .iov_len = sizeof(buf)};

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    if (recvmsg(sock_fd, &msg, 0) < 0) {
        LOG_ERROR(LOG_CAT_BUS, "edge_fd_recv failed: %s", strerror(errno));
        return -1;
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
        return *((int *)CMSG_DATA(cmsg));
    }
    return -1;
}