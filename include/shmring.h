#ifndef SHMRING_H
#define SHMRING_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "edge_common.h"

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

enum {
    PIPEMSG_TYPE_SCALAR = 1,
    PIPEMSG_TYPE_REF = 2,
    PIPEMSG_TYPE_BATCH = 3,
    PIPEMSG_TYPE_EVENT = 4
};

/* 向后兼容别名 */
#define PIPEMSG_SCALAR_F64 PIPEMSG_TYPE_SCALAR
#define PIPEMSG_REF_DMA PIPEMSG_TYPE_REF
#define PIPEMSG_EVENT PIPEMSG_TYPE_EVENT

typedef struct {
    uint64_t ts;
    uint32_t tag_id;
    uint16_t type;
    uint16_t state;
    uint32_t src_dev_id;
    uint32_t drop_reason;
    uint8_t quality;
    uint8_t _pad[7];

    union {
        struct {
            double f;
            int64_t i;
        } val;

        struct {
            void *ptr;
            int fd;
            size_t len;
            void (*free_cb)(void *);
        } ref;

        struct {
            void *ptr;
            int count;
        } batch;

        struct {
            uint32_t event_id;
            uint32_t param;
        } event;
    } payload;
} PipeMsg;

// --- Builder API (New) ---

static inline void pipe_msg_init_scalar(PipeMsg *msg, uint32_t tag, double val, uint64_t ts) {
    *msg = (PipeMsg){0};
    msg->tag_id = tag;
    msg->ts = ts;
    msg->type = PIPEMSG_TYPE_SCALAR;
    msg->payload.val.f = val;
    msg->state = TRACE_CREATED;
}

static inline void pipe_msg_init_event(PipeMsg *msg, uint32_t eid, uint32_t param, uint64_t ts) {
    *msg = (PipeMsg){0};
    msg->ts = ts;
    msg->type = PIPEMSG_TYPE_EVENT;
    msg->payload.event.event_id = eid;
    msg->payload.event.param = param;
    msg->state = TRACE_CREATED;
}

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

int shmring_create(shm_ring_t *ring, const char *name, uint32_t slots);
int shmring_open(shm_ring_t *ring, const char *name);
void shmring_close(shm_ring_t *ring);
int shmring_destroy(shm_ring_t *ring);
void shm_ring_write(shm_ring_t *ring, const PipeMsg *msg);
int shm_ring_read(shm_ring_t *ring, uint64_t *last_seq, PipeMsg *out);

#endif
