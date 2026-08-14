/**
 * @file resilient_buffer.c
 * @brief Resilient Telemetry Store-and-Forward Buffer Implementation (Crash-Resilient WAL Support)
 */

#include "resilient_buffer.h"
#include <stdio.h>
#include <stdlib.h>
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

int resilient_buffer_flush_to_file(const resilient_buffer_t *buf, const char *filepath) {
    if (!buf || !filepath) return -1;

    FILE *f = fopen(filepath, "wb");
    if (!f) return -1;

    resilient_wal_header_t hdr;
    hdr.magic = RESILIENT_WAL_MAGIC;
    hdr.count = (uint32_t)buf->count;
    hdr.head = (uint32_t)buf->head;
    hdr.tail = (uint32_t)buf->tail;

    if (fwrite(&hdr, sizeof(hdr), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    if (buf->count > 0) {
        if (fwrite(buf->records, sizeof(telemetry_record_t), RESILIENT_BUFFER_CAPACITY, f) != RESILIENT_BUFFER_CAPACITY) {
            fclose(f);
            return -1;
        }
    }

    fflush(f);
    fclose(f);
    return 0;
}

int resilient_buffer_load_from_file(resilient_buffer_t *buf, const char *filepath) {
    if (!buf || !filepath) return -1;

    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;

    resilient_wal_header_t hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    if (hdr.magic != RESILIENT_WAL_MAGIC) {
        fclose(f);
        return -2; /* Corrupted header */
    }

    if (fread(buf->records, sizeof(telemetry_record_t), RESILIENT_BUFFER_CAPACITY, f) != RESILIENT_BUFFER_CAPACITY) {
        fclose(f);
        return -1;
    }

    buf->count = (size_t)hdr.count;
    buf->head = (size_t)hdr.head;
    buf->tail = (size_t)hdr.tail;

    fclose(f);
    return 0;
}
