/**
 * @file resilient_buffer.c
 * @brief Resilient Telemetry Store-and-Forward Buffer Implementation
 */

#include "resilient_buffer.h"
#include <string.h>

int resilient_buffer_init(resilient_buffer_t *buf, uint32_t max_rate) {
    if (!buf) {
        return -1;
    }

    memset(buf, 0, sizeof(resilient_buffer_t));
    buf->network_online = true;
    buf->max_rate_per_sec = (max_rate > 0) ? max_rate : 50;
    return 0;
}

void resilient_buffer_set_net_state(resilient_buffer_t *buf, bool online) {
    if (buf) {
        buf->network_online = online;
    }
}

int resilient_buffer_push(resilient_buffer_t *buf, uint32_t tag_id, double val, uint64_t timestamp_ms) {
    if (!buf) {
        return -1;
    }

    if (buf->count >= RESILIENT_BUFFER_CAPACITY) {
        return -2; /* Buffer full */
    }

    telemetry_record_t *rec = &buf->records[buf->tail];
    rec->record_id = (uint32_t)(buf->tail + 1);
    rec->tag_id = tag_id;
    rec->val = val;
    rec->timestamp_ms = timestamp_ms;

    buf->tail = (buf->tail + 1) % RESILIENT_BUFFER_CAPACITY;
    buf->count++;
    return 0;
}

int resilient_buffer_pop(resilient_buffer_t *buf, telemetry_record_t *out_record) {
    if (!buf || !out_record) {
        return -1;
    }

    if (!buf->network_online || buf->count == 0) {
        return 1; /* Network offline or buffer empty */
    }

    *out_record = buf->records[buf->head];
    buf->head = (buf->head + 1) % RESILIENT_BUFFER_CAPACITY;
    buf->count--;
    return 0;
}
