/*
 * shmring.h
 *
 * ========================= DESIGN CONTRACT =========================
 *
 * This file implements a fixed-size shared-memory ring buffer
 * optimized for HIGH-FREQUENCY DATA STREAMING.
 *
 * It is designed for:
 *   - Telemetry (voltage/current/state)
 *   - Algorithm input (aggregation / FFT / control loop)
 *   - One-writer, multi-reader pattern
 *
 * It is NOT designed for:
 *   - Key-Value storage
 *   - Random access lookup
 *   - Variable-sized payload per slot
 *   - Persistence or durability
 *
 * ========================= CORE RULES =========================
 *
 * 1. Fixed-size slots (Cache-line aligned)
 *    - Every slot is exactly the same size
 *    - Prevents cache pollution and false sharing
 *
 * 2. Seqlock synchronization
 *    - Writer is single-thread/process
 *    - Readers are lock-free
 *    - Readers may retry but never block
 *
 * 3. In-band metadata, Out-of-band large data
 *    - Slot stores scalar or reference ONLY
 *    - Large payloads live outside the ring
 *
 * 4. Overwrite-on-full semantics
 *    - Slow readers lose old data
 *    - System prioritizes real-time freshness
 *
 * ===================================================================
 */
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
    PIPEMSG_SCALAR = 1,
    PIPEMSG_REF    = 2,
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

    atomic_uint seq;      /* seqlock sequence */
    atomic_uint write_idx;

    uint8_t _pad[CACHELINE_SIZE - 16];
} ShmRingHeader;

typedef struct {
    ShmRingHeader *hdr;
    PipeMsg       *slots;
    size_t         map_size;
    char           name[256];
} ShmRing;

/**
 * @brief Creates and initializes a new shared memory ring buffer.
 *
 * @param ring Pointer to the ShmRing structure to initialize.
 * @param name The name of the shared memory segment (e.g., "/my_ring").
 * @param slots The number of slots in the ring buffer.
 * @return 0 on success, -1 on failure.
 */
int shmring_create(ShmRing *ring, const char *name, uint32_t slots);

/**
 * @brief Opens an existing shared memory ring buffer.
 *
 * @param ring Pointer to the ShmRing structure to initialize.
 * @param name The name of the shared memory segment.
 * @return 0 on success, -1 on failure.
 */
int shmring_open(ShmRing *ring, const char *name);

/**
 * @brief Closes the shared memory ring buffer, unmapping it from memory.
 *
 * @param ring The ring buffer to close.
 */
void shmring_close(ShmRing *ring);

/**
 * @brief Destroys the shared memory segment. Should only be called by one
 *        process (usually the creator) after all others have closed it.
 *
 * @param ring The ring buffer to destroy.
 * @return 0 on success, -1 on failure.
 */
int shmring_destroy(ShmRing *ring);

/**
 * @brief Writes a message to the ring buffer. This is the single-writer function.
 *        It overwrites the oldest data if the buffer is full.
 *
 * @param ring The ring buffer.
 * @param msg The message to write.
 */
void shmring_write(ShmRing *ring, const PipeMsg *msg);

/**
 * @brief Reads the most recently written message from the ring buffer.
 *        This is a lock-free, non-blocking function for multiple readers.
 *
 * @param ring The ring buffer.
 * @param out Pointer to a PipeMsg structure to store the read message.
 * @return true if a message was successfully read, false otherwise.
 */
bool shmring_read_latest(ShmRing *ring, PipeMsg *out);


#endif // SHMRING_H
