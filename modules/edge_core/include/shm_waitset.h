/**
 * @file shm_waitset.h
 * @brief Zero-Copy Wait-Set Event Engine for EdgeC Inter-Process Communication
 */

#ifndef SHM_WAITSET_H
#define SHM_WAITSET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define SHM_WAITSET_MAX_SUBSCRIBERS 16
#define SHM_WAITSET_CHUNK_SIZE      1024

typedef struct {
    uint32_t chunk_id;
    uint32_t data_len;
    uint64_t timestamp_ns;
    uint8_t payload[SHM_WAITSET_CHUNK_SIZE];
} shm_chunk_t;

typedef struct {
    uint32_t subscriber_id;
    bool active;
    uint32_t last_read_chunk_id;
} shm_subscriber_t;

typedef struct {
    uint32_t active_subscribers;
    uint32_t current_chunk_id;
    shm_chunk_t current_chunk;
    shm_subscriber_t subscribers[SHM_WAITSET_MAX_SUBSCRIBERS];
} shm_waitset_t;

/**
 * @brief Initializes Zero-Copy Wait-Set structure
 * @param ws Pointer to waitset instance
 * @return 0 on success, negative error code on failure
 */
int shm_waitset_init(shm_waitset_t *ws);

/**
 * @brief Registers a new subscriber to the wait-set
 * @param ws Pointer to waitset instance
 * @param subscriber_id Output subscriber ID assigned
 * @return 0 on success, negative error code on failure
 */
int shm_waitset_register_subscriber(shm_waitset_t *ws, uint32_t *subscriber_id);

/**
 * @brief Unregisters a subscriber from the wait-set
 * @param ws Pointer to waitset instance
 * @param subscriber_id ID of the subscriber to unregister
 * @return 0 on success, negative error code on failure
 */
int shm_waitset_unregister_subscriber(shm_waitset_t *ws, uint32_t subscriber_id);

/**
 * @brief Zero-copy publishes a chunk to all active subscribers
 * @param ws Pointer to waitset instance
 * @param data Pointer to raw payload data
 * @param len Length of payload data
 * @param timestamp_ns Timestamp in nanoseconds
 * @return 0 on success, negative error code on failure
 */
int shm_waitset_publish_chunk(shm_waitset_t *ws, const uint8_t *data, uint32_t len, uint64_t timestamp_ns);

/**
 * @brief Polls for the latest available chunk pointer without payload copy (Zero-Copy)
 * @param ws Pointer to waitset instance
 * @param subscriber_id Subscriber ID polling
 * @param out_chunk Output pointer to the latest chunk
 * @return 0 on success (new chunk available), 1 if no new chunk, negative on error
 */
int shm_waitset_poll_chunk(shm_waitset_t *ws, uint32_t subscriber_id, const shm_chunk_t **out_chunk);

#ifdef __cplusplus
}
#endif

#endif /* SHM_WAITSET_H */
