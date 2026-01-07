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
    uint8_t priority;  /**< S18: Arbitration priority (0-255) */
    uint8_t source_id; /**< S19: Source link identification */
    uint8_t _res[5];   /**< Future expansion (e.g., security signatures) */
} PipeMsgAttr;

typedef struct {
    uint64_t ts;
    uint32_t tag_id;
    uint16_t type;
    uint16_t state;
    uint32_t src_dev_id;
    uint32_t drop_reason;
    uint8_t quality;
    PipeMsgAttr attr; /**< Replacing former _pad[7] */

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
    // attr already zeroed by compound literal
}

static inline void pipe_msg_init_event(PipeMsg *msg, uint32_t eid, uint32_t param, uint64_t ts) {
    *msg = (PipeMsg){0};
    msg->ts = ts;
    msg->type = PIPEMSG_TYPE_EVENT;
    msg->payload.event.event_id = eid;
    msg->payload.event.param = param;
    msg->state = TRACE_CREATED;
    // attr already zeroed by compound literal
}

static inline void pipe_msg_init_ref(PipeMsg *msg, uint32_t tag, void *ptr, size_t len,
                                     uint64_t ts) {
    *msg = (PipeMsg){0};
    msg->tag_id = tag;
    msg->ts = ts;
    msg->type = PIPEMSG_TYPE_REF;
    msg->payload.ref.ptr = ptr;
    msg->payload.ref.len = len;
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
    int fd;  // [新增] 匿名内存 FD
    char name[256];
} ShmRing;

typedef ShmRing shm_ring_t;
typedef PipeMsg pipe_msg_t;

int shmring_create(shm_ring_t *ring, const char *name, uint32_t slots);
int shmring_init_from_fd(shm_ring_t *ring, int fd, size_t size);
int shmring_get_fd(shm_ring_t *ring);
int shmring_open(shm_ring_t *ring, const char *name);
void shmring_close(shm_ring_t *ring);
int shmring_destroy(shm_ring_t *ring);
void shm_ring_write(shm_ring_t *ring, const PipeMsg *msg);
int shm_ring_read(shm_ring_t *ring, uint64_t *last_seq, PipeMsg *out);

/**
 * @brief 通过 Unix Domain Socket 发送文件描述符 (用于 FD 分发中心)
 */
int edge_fd_send(int sock_fd, int fd_to_send);

/**
 * @brief 通过 Unix Domain Socket 接收文件描述符
 */
int edge_fd_recv(int sock_fd);

#endif
