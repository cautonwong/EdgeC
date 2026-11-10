#ifndef SHMRING_H
#define SHMRING_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_common.h"
#include "pipe_msg.h"

#define SHMRING_MAGIC 0x53484D52
#define CACHELINE_SIZE 64
#define SHMRING_MAX_SLOTS 65536

typedef enum {
    TRACE_CREATED = 0x01,
    TRACE_IN_SHM = 0x02,
    TRACE_PROCESSING = 0x03,
    TRACE_STORED = 0x04,
    TRACE_EXPORTED = 0x05,
    TRACE_DROP_BUSY = 0x10,
    TRACE_DROP_STALE = 0x11,
    TRACE_DROP_FILTER = 0x12
} TraceState;


typedef struct {
    uint32_t magic;
    uint32_t slot_count;
    atomic_uint seq;
    atomic_uint write_idx;
    atomic_uint drop_count;
    atomic_uint_fast64_t total_writes;
    uint8_t _pad[CACHELINE_SIZE - 32];
} ShmRingHeader;

typedef struct {
    ShmRingHeader *hdr;
    PipeMsg *slots;
    size_t map_size;
    int fd;
    int event_fd;        // [ADR-0001] eventfd for O(1) wakeup
    char name[256];
} ShmRing;

typedef ShmRing shm_ring_t;
typedef PipeMsg pipe_msg_t;

int shmring_create(shm_ring_t *ring, const char *name, uint32_t slots);
int shmring_init_from_fd(shm_ring_t *ring, int fd, size_t size);
int shmring_get_fd(shm_ring_t *ring);
int shmring_get_event_fd(shm_ring_t *ring); // [ADR-0001] eventfd for O(1) wakeup
int shmring_open(shm_ring_t *ring, const char *name);
void shmring_close(shm_ring_t *ring);
int shmring_destroy(shm_ring_t *ring);
void shm_ring_write(shm_ring_t *ring, const PipeMsg *msg);
int shm_ring_read(shm_ring_t *ring, uint64_t *last_seq, PipeMsg *out);

// 💡 [Iceoryx Pattern] 物理首地址无关相对指针 (Relative Pointer Translation)
static inline ptrdiff_t shmring_abs_to_rel(const void *base, const void *abs_ptr) {
    if (!abs_ptr || !base) return 0;
    return (const char *)abs_ptr - (const char *)base;
}

static inline void *shmring_rel_to_abs(const void *base, ptrdiff_t rel_offset) {
    if (rel_offset == 0 || !base) return NULL;
    return (void *)((const char *)base + rel_offset);
}

int edge_fd_send(int sock_fd, int fd_to_send);
int edge_fd_recv(int sock_fd);


#endif
