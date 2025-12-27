#ifndef SHMRING_H
#define SHMRING_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SHMRING_MAGIC 0x53484D52
#define CACHELINE_SIZE 64
#define SHMRING_MAX_SLOTS 1024

enum {
    PIPEMSG_SCALAR_F64 = 1,
    PIPEMSG_SCALAR_I64 = 2,
    PIPEMSG_REF_DMA = 3,  // Scenario 3: High-Perf Waveform (fd + offset)
    PIPEMSG_EVENT = 4     // Scenario 8/9: System events
};

typedef struct {
    uint64_t ts;
    uint32_t tag_id;
    uint16_t type;
    uint16_t flags;

    union {
        double val_f;
        int64_t val_i;
        struct {
            int32_t fd;  // dma-buf file descriptor
            uint32_t offset;
            uint32_t len;
        } ref;
        struct {
            uint32_t event_id;
            uint32_t param;
        } event;
    } payload;

    uint8_t _pad[32];
} PipeMsg;

_Static_assert(sizeof(PipeMsg) == 64, "PipeMsg must be 64 bytes");

typedef struct {
    uint32_t magic;
    uint32_t slot_count;
    atomic_uint seq;
    atomic_uint write_idx;
    atomic_uint_fast64_t total_writes;
    uint8_t _pad[CACHELINE_SIZE - 24];
} ShmRingHeader;

typedef struct {
    ShmRingHeader *hdr;
    PipeMsg *slots;
    size_t map_size;
    char name[256];
} ShmRing;

typedef ShmRing shm_ring_t;
typedef PipeMsg pipe_msg_t;

/* Backward compatibility */
#define PIPE_SCALAR_F64 PIPEMSG_SCALAR_F64

int shmring_create(shm_ring_t *ring, const char *name, uint32_t slots);
int shmring_open(shm_ring_t *ring, const char *name);
void shmring_close(shm_ring_t *ring);
int shmring_destroy(shm_ring_t *ring);
void shm_ring_write(shm_ring_t *ring, const PipeMsg *msg);
int shm_ring_read(shm_ring_t *ring, uint64_t *last_seq, PipeMsg *out);

#endif
