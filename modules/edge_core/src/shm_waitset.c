/**
 * @file shm_waitset.c
 * @brief Zero-Copy Wait-Set Event Engine Implementation
 */

#include "shm_waitset.h"
#include <string.h>

int shm_waitset_init(shm_waitset_t *ws) {
    if (!ws) {
        return -1;
    }

    memset(ws, 0, sizeof(shm_waitset_t));
    return 0;
}

int shm_waitset_register_subscriber(shm_waitset_t *ws, uint32_t *subscriber_id) {
    if (!ws || !subscriber_id) {
        return -1;
    }

    for (uint32_t i = 0; i < SHM_WAITSET_MAX_SUBSCRIBERS; i++) {
        if (!ws->subscribers[i].active) {
            ws->subscribers[i].active = true;
            ws->subscribers[i].subscriber_id = i + 1;
            ws->subscribers[i].last_read_chunk_id = ws->current_chunk_id;
            ws->active_subscribers++;
            *subscriber_id = ws->subscribers[i].subscriber_id;
            return 0;
        }
    }

    return -2; /* No free slot */
}

int shm_waitset_unregister_subscriber(shm_waitset_t *ws, uint32_t subscriber_id) {
    if (!ws || subscriber_id == 0 || subscriber_id > SHM_WAITSET_MAX_SUBSCRIBERS) {
        return -1;
    }

    uint32_t idx = subscriber_id - 1;
    if (ws->subscribers[idx].active) {
        ws->subscribers[idx].active = false;
        if (ws->active_subscribers > 0) {
            ws->active_subscribers--;
        }
        return 0;
    }

    return -2; /* Not active */
}

int shm_waitset_publish_chunk(shm_waitset_t *ws, const uint8_t *data, uint32_t len, uint64_t timestamp_ns) {
    if (!ws || !data || len > SHM_WAITSET_CHUNK_SIZE) {
        return -1;
    }

    ws->current_chunk_id++;
    ws->current_chunk.chunk_id = ws->current_chunk_id;
    ws->current_chunk.data_len = len;
    ws->current_chunk.timestamp_ns = timestamp_ns;
    memcpy(ws->current_chunk.payload, data, len);

    return 0;
}

int shm_waitset_poll_chunk(shm_waitset_t *ws, uint32_t subscriber_id, const shm_chunk_t **out_chunk) {
    if (!ws || !out_chunk || subscriber_id == 0 || subscriber_id > SHM_WAITSET_MAX_SUBSCRIBERS) {
        return -1;
    }

    uint32_t idx = subscriber_id - 1;
    if (!ws->subscribers[idx].active) {
        return -2;
    }

    if (ws->subscribers[idx].last_read_chunk_id < ws->current_chunk_id) {
        ws->subscribers[idx].last_read_chunk_id = ws->current_chunk_id;
        *out_chunk = &ws->current_chunk; /* Zero-copy pointer assignment */
        return 0; /* New chunk available */
    }

    return 1; /* No new chunk */
}
