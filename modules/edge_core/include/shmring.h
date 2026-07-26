#ifndef SHMRING_H
#define SHMRING_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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

// 💡 [Nexus Legacy Compatibility] 恢复存量算子库依赖的消息类型宏
enum {
    PIPEMSG_TYPE_SCALAR = 1,
    PIPEMSG_TYPE_REF = 2,
    PIPEMSG_TYPE_BATCH = 3,
    PIPEMSG_TYPE_EVENT = 4,
    PIPEMSG_TYPE_METRIC = 5,   // 💡 [ADR-0001] 系统性能指标

    // 别名支持
    PIPEMSG_SCALAR_F64 = 1,
    PIPEMSG_SCALAR_I64 = 1,
    PIPEMSG_REF = 2,
    PIPEMSG_BATCH = 3,
    PIPEMSG_EVENT = 4,
    PIPEMSG_METRIC = 5
};

typedef struct {
    uint8_t priority;
    uint8_t source_id;
    uint8_t _res[5];   // 💡 [Omnis Align] 严格占用 7 字节，确保 Header 总长 32 字节
} PipeMsgAttr;

typedef struct {
    uint64_t ts;          // 0..7
    uint32_t tag_id;      // 8..11
    uint32_t device_id;   // 12..15
    uint16_t type;        // 16..17
    uint16_t state;       // 18..19
    uint32_t drop_reason; // 20..23
    uint8_t quality;      // 24
    PipeMsgAttr attr;     // 25..31 (7 bytes)

    union {
        struct {
            double f;
            int64_t i;
        } val;            // 32..47

        struct {
            void *ptr;
            int fd;
            size_t len;
            void (*free_cb)(void *);
        } ref;            // 32..63 (ptr=8, fd=4, pad=4, len=8, cb=8)

        struct {
            void *ptr;
            int count;
        } batch;          // 32..43

        struct {
            uint32_t event_id;
            uint32_t param;
        } event;          // 32..39
    } payload;            // 起始地址 32 (由于之前的结构体总长是 32 且对齐需求)
} PipeMsg;

// 💡 [Iron Rule] 严格强制 64 字节对齐，这是跨进程共享内存的生命线
_Static_assert(sizeof(PipeMsg) == 64, "PipeMsg must be 64 bytes");

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

// --- Convenience Constructors ---
static inline void pipe_msg_init_scalar(PipeMsg *msg, uint32_t tag_id, double value, uint64_t ts) {
    memset(msg, 0, sizeof(PipeMsg));
    msg->ts = ts;
    msg->tag_id = tag_id;
    msg->type = PIPEMSG_TYPE_SCALAR;
    msg->payload.val.f = value;
}

static inline void pipe_msg_init_event(PipeMsg *msg, uint32_t event_id, uint32_t param, uint64_t ts) {
    memset(msg, 0, sizeof(PipeMsg));
    msg->ts = ts;
    msg->type = PIPEMSG_TYPE_EVENT;
    msg->payload.event.event_id = event_id;
    msg->payload.event.param = param;
}

static inline void pipe_msg_init_batch(PipeMsg *msg, uint32_t tag_id, void *ptr, int count, uint64_t ts) {
    memset(msg, 0, sizeof(PipeMsg));
    msg->ts = ts;
    msg->tag_id = tag_id;
    msg->type = PIPEMSG_TYPE_BATCH;
    msg->payload.batch.ptr = ptr;
    msg->payload.batch.count = count;
}

#endif
