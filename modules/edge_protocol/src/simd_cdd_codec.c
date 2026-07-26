#include "simd_cdd_codec.h"
#include <string.h>
#include <stdio.h>

static uint32_t cdd_calc_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

void cdd_schema_init(cdd_schema_t *schema) {
    if (!schema) return;
    memset(schema, 0, sizeof(cdd_schema_t));
}

int cdd_schema_add_field(cdd_schema_t *schema, uint16_t tag_id, uint8_t type, uint16_t offset, double scale, double offset_val) {
    if (!schema || schema->field_count >= CDD_MAX_FIELDS) return -1;
    cdd_tag_field_t *field = &schema->fields[schema->field_count++];
    field->tag_id = tag_id;
    field->type = type;
    field->offset = offset;
    field->scale = (scale == 0.0) ? 1.0 : scale;
    field->offset_val = offset_val;
    return 0;
}

const cdd_tag_field_t* cdd_schema_find_field(const cdd_schema_t *schema, uint16_t tag_id) {
    if (!schema) return NULL;
    for (uint32_t i = 0; i < schema->field_count; i++) {
        if (schema->fields[i].tag_id == tag_id) {
            return &schema->fields[i];
        }
    }
    return NULL;
}

int cdd_encode_batch(const cdd_schema_t *schema, uint32_t sequence, const cdd_batch_sample_t *samples, uint32_t count, uint8_t *out_buf, size_t buf_size, size_t *out_len) {
    if (!schema || !samples || !out_buf || !out_len) return -1;
    
    size_t header_len = sizeof(cdd_frame_header_t);
    size_t sample_size = sizeof(uint16_t) + sizeof(double) + sizeof(uint64_t);
    size_t total_payload = count * sample_size;

    if (buf_size < header_len + total_payload) return -2;

    cdd_frame_header_t hdr;
    hdr.magic = CDD_MAGIC;
    hdr.sequence = sequence;
    hdr.field_count = (uint16_t)count;
    hdr.reserved = 0;
    hdr.payload_len = (uint32_t)total_payload;

    uint8_t *payload_ptr = out_buf + header_len;
    size_t offset = 0;

    for (uint32_t i = 0; i < count; i++) {
        const cdd_tag_field_t *field = cdd_schema_find_field(schema, samples[i].tag_id);
        double raw_val = samples[i].value;
        if (field) {
            /* Apply inverse scale & offset for raw transmission */
            raw_val = (samples[i].value - field->offset_val) / field->scale;
        }

        uint16_t tag = samples[i].tag_id;
        uint64_t ts = samples[i].timestamp_ms;

        memcpy(payload_ptr + offset, &tag, sizeof(tag));
        offset += sizeof(tag);
        memcpy(payload_ptr + offset, &raw_val, sizeof(raw_val));
        offset += sizeof(raw_val);
        memcpy(payload_ptr + offset, &ts, sizeof(ts));
        offset += sizeof(ts);
    }

    hdr.crc32 = cdd_calc_crc32(payload_ptr, total_payload);
    memcpy(out_buf, &hdr, header_len);

    *out_len = header_len + total_payload;
    return 0;
}

int cdd_decode_batch(const cdd_schema_t *schema, const uint8_t *in_buf, size_t in_len, cdd_batch_sample_t *out_samples, uint32_t max_count, uint32_t *out_count) {
    if (!schema || !in_buf || !out_samples || !out_count) return -1;
    size_t header_len = sizeof(cdd_frame_header_t);
    if (in_len < header_len) return -2;

    cdd_frame_header_t hdr;
    memcpy(&hdr, in_buf, header_len);

    if (hdr.magic != CDD_MAGIC) return -3;
    if (in_len < header_len + hdr.payload_len) return -4;

    const uint8_t *payload_ptr = in_buf + header_len;
    uint32_t calc_crc = cdd_calc_crc32(payload_ptr, hdr.payload_len);
    if (calc_crc != hdr.crc32) return -5;

    size_t sample_size = sizeof(uint16_t) + sizeof(double) + sizeof(uint64_t);
    uint32_t count = hdr.payload_len / sample_size;
    if (count > max_count) count = max_count;

    size_t offset = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint16_t tag;
        double raw_val;
        uint64_t ts;

        memcpy(&tag, payload_ptr + offset, sizeof(tag));
        offset += sizeof(tag);
        memcpy(&raw_val, payload_ptr + offset, sizeof(raw_val));
        offset += sizeof(raw_val);
        memcpy(&ts, payload_ptr + offset, sizeof(ts));
        offset += sizeof(ts);

        out_samples[i].tag_id = tag;
        out_samples[i].timestamp_ms = ts;

        const cdd_tag_field_t *field = cdd_schema_find_field(schema, tag);
        if (field) {
            out_samples[i].value = (raw_val * field->scale) + field->offset_val;
        } else {
            out_samples[i].value = raw_val;
        }
    }

    *out_count = count;
    return 0;
}

int cdd_fast_scale_transform(const float *raw_in, float scale, float offset, float *eng_out, size_t count) {
    if (!raw_in || !eng_out) return -1;

    size_t i = 0;
    /* Loop unrolling for SIMD-friendly compiler vectorization */
    for (; i + 4 <= count; i += 4) {
        eng_out[i + 0] = raw_in[i + 0] * scale + offset;
        eng_out[i + 1] = raw_in[i + 1] * scale + offset;
        eng_out[i + 2] = raw_in[i + 2] * scale + offset;
        eng_out[i + 3] = raw_in[i + 3] * scale + offset;
    }
    for (; i < count; i++) {
        eng_out[i] = raw_in[i] * scale + offset;
    }
    return 0;
}
