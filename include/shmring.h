#ifndef SHMRING_H
#define SHMRING_H

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

#define SHMRING_MAGIC 0x53484D52  /* 'SHMR' */
#define CACHELINE_SIZE 64
#define SHMRING_MAX_SLOTS 1024

/* Message type */
enum {
    PIPEMSG_SCALAR  = 1,
    PIPEMSG_REF     = 2,
    PIPE_SCALAR_F64 = 3, /* Added for compatibility with Algorithm Sum */
};

/* 64-byte fixed slot */
typedef struct {
    uint64_t ts;
    uint32_t tag_id;
    uint16_t type;
    uint16_t flags;

    union {
        double   val_f;
        int64_t  val_i;
        struct {
            uint32_t offset;
            uint32_t len;
            int32_t  fd;
        } ref;
    } payload;

    uint8_t _pad[32]; /* force 64 bytes */
} PipeMsg;

// Ensure the struct is exactly 64 bytes
_Static_assert(sizeof(PipeMsg) == 64, "PipeMsg must be 64 bytes");

typedef struct {
    uint32_t magic;
    uint32_t slot_count;

    atomic_uint seq;        /* seqlock sequence for concurrency */
    atomic_uint write_idx;  /* current write index */
    atomic_uint_fast64_t total_writes; /* Monotonic counter of all writes */

    uint8_t _pad[CACHELINE_SIZE - 24]; /* Ensure 64-byte alignment */
} ShmRingHeader;

typedef struct {
    ShmRingHeader *hdr;
    PipeMsg       *slots;
    size_t         map_size;
    char           name[256];
} ShmRing;

/* Typedefs for consistency with user request */
typedef ShmRing shm_ring_t;
typedef PipeMsg pipe_msg_t;

/**
 * @brief Creates and initializes a new shared memory ring buffer.
 */
int shmring_create(shm_ring_t *ring, const char *name, uint32_t slots);

/**
 * @brief Opens an existing shared memory ring buffer.
 */
int shmring_open(shm_ring_t *ring, const char *name);

/**
 * @brief Closes the shared memory ring buffer.
 */
void shmring_close(shm_ring_t *ring);

/**
 * @brief Destroys the shared memory segment.
 */
int shmring_destroy(shm_ring_t *ring);

/**
 * @brief Writes a message to the ring buffer.
 */
void shm_ring_write(shm_ring_t *ring, const PipeMsg *msg);
/* Alias for existing API */
#define shmring_write shm_ring_write

/**
 * @brief Reads the next message after 'last_seq'.
 *        Updates 'last_seq' to the new sequence number.
 * @return 0 on success (new data read), -1 if no new data.
 */
int shm_ring_read(shm_ring_t *ring, uint64_t *last_seq, PipeMsg *out);

#endif // SHMRING_H