/**
 * @file pipe_msg.h
 * @brief Shared 64-byte telemetry message value type (foundation).
 *
 * PipeMsg is the cross-process bus payload. It is pure POD and must stay
 * 64-byte aligned; the ring-buffer implementation stays in edge_core/shmring.
 */
#ifndef PIPE_MSG_H
#define PIPE_MSG_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef PipeMsg pipe_msg_t;


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

#ifdef __cplusplus
}
#endif

#endif /* PIPE_MSG_H */
