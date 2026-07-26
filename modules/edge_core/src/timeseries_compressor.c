#include "timeseries_compressor.h"
#include <string.h>

/* Header magic 0x54534442 ("TSDB") */
#define TSDB_MAGIC 0x54534442

typedef struct {
    uint32_t magic;
    uint32_t count;
} ts_header_t;

int ts_compress_batch(const ts_sample_t *samples, size_t count, uint8_t *out_buf, size_t buf_capacity, size_t *out_compressed_len) {
    if (!samples || !out_buf || !out_compressed_len || count == 0) return -1;

    size_t header_len = sizeof(ts_header_t);
    size_t payload_len = 0;

    /* Write Header */
    ts_header_t hdr = { .magic = TSDB_MAGIC, .count = (uint32_t)count };
    if (buf_capacity < header_len) return -2;
    memcpy(out_buf, &hdr, header_len);

    uint8_t *ptr = out_buf + header_len;
    size_t rem = buf_capacity - header_len;

    /* Delta & XOR Compression Pass */
    uint64_t prev_ts = 0;
    int64_t prev_delta = 0;
    uint64_t prev_bits = 0;

    for (size_t i = 0; i < count; i++) {
        uint64_t curr_ts = samples[i].timestamp_ms;
        double curr_val = samples[i].value;
        uint64_t curr_bits;
        memcpy(&curr_bits, &curr_val, sizeof(curr_bits));

        if (i == 0) {
            /* First sample: Store raw TS & Value */
            if (rem < sizeof(curr_ts) + sizeof(curr_bits)) return -3;
            memcpy(ptr + payload_len, &curr_ts, sizeof(curr_ts));
            payload_len += sizeof(curr_ts);
            memcpy(ptr + payload_len, &curr_bits, sizeof(curr_bits));
            payload_len += sizeof(curr_bits);

            prev_ts = curr_ts;
            prev_bits = curr_bits;
        } else if (i == 1) {
            /* Second sample: Store TS Delta & Value XOR */
            int64_t delta = (int64_t)(curr_ts - prev_ts);
            uint64_t xor_val = curr_bits ^ prev_bits;

            if (rem < payload_len + sizeof(delta) + sizeof(xor_val)) return -3;
            memcpy(ptr + payload_len, &delta, sizeof(delta));
            payload_len += sizeof(delta);
            memcpy(ptr + payload_len, &xor_val, sizeof(xor_val));
            payload_len += sizeof(xor_val);

            prev_delta = delta;
            prev_ts = curr_ts;
            prev_bits = curr_bits;
        } else {
            /* Subsequent samples: Store Delta-of-Delta & Value XOR */
            int64_t curr_delta = (int64_t)(curr_ts - prev_ts);
            int64_t dod = curr_delta - prev_delta;
            uint64_t xor_val = curr_bits ^ prev_bits;

            if (rem < payload_len + sizeof(dod) + sizeof(xor_val)) return -3;
            memcpy(ptr + payload_len, &dod, sizeof(dod));
            payload_len += sizeof(dod);
            memcpy(ptr + payload_len, &xor_val, sizeof(xor_val));
            payload_len += sizeof(xor_val);

            prev_delta = curr_delta;
            prev_ts = curr_ts;
            prev_bits = curr_bits;
        }
    }

    *out_compressed_len = header_len + payload_len;
    return 0;
}

int ts_decompress_batch(const uint8_t *in_buf, size_t compressed_len, ts_sample_t *out_samples, size_t max_count, size_t *out_count) {
    if (!in_buf || !out_samples || !out_count) return -1;
    size_t header_len = sizeof(ts_header_t);
    if (compressed_len < header_len) return -2;

    ts_header_t hdr;
    memcpy(&hdr, in_buf, header_len);
    if (hdr.magic != TSDB_MAGIC) return -3;

    size_t count = hdr.count;
    if (count > max_count) count = max_count;

    const uint8_t *ptr = in_buf + header_len;
    size_t offset = 0;

    uint64_t prev_ts = 0;
    int64_t prev_delta = 0;
    uint64_t prev_bits = 0;

    for (size_t i = 0; i < count; i++) {
        if (i == 0) {
            uint64_t ts;
            uint64_t bits;
            memcpy(&ts, ptr + offset, sizeof(ts));
            offset += sizeof(ts);
            memcpy(&bits, ptr + offset, sizeof(bits));
            offset += sizeof(bits);

            double val;
            memcpy(&val, &bits, sizeof(val));
            out_samples[i].timestamp_ms = ts;
            out_samples[i].value = val;

            prev_ts = ts;
            prev_bits = bits;
        } else if (i == 1) {
            int64_t delta;
            uint64_t xor_val;
            memcpy(&delta, ptr + offset, sizeof(delta));
            offset += sizeof(delta);
            memcpy(&xor_val, ptr + offset, sizeof(xor_val));
            offset += sizeof(xor_val);

            uint64_t ts = prev_ts + delta;
            uint64_t bits = prev_bits ^ xor_val;
            double val;
            memcpy(&val, &bits, sizeof(val));

            out_samples[i].timestamp_ms = ts;
            out_samples[i].value = val;

            prev_delta = delta;
            prev_ts = ts;
            prev_bits = bits;
        } else {
            int64_t dod;
            uint64_t xor_val;
            memcpy(&dod, ptr + offset, sizeof(dod));
            offset += sizeof(dod);
            memcpy(&xor_val, ptr + offset, sizeof(xor_val));
            offset += sizeof(xor_val);

            int64_t curr_delta = prev_delta + dod;
            uint64_t ts = prev_ts + curr_delta;
            uint64_t bits = prev_bits ^ xor_val;
            double val;
            memcpy(&val, &bits, sizeof(val));

            out_samples[i].timestamp_ms = ts;
            out_samples[i].value = val;

            prev_delta = curr_delta;
            prev_ts = ts;
            prev_bits = bits;
        }
    }

    *out_count = count;
    return 0;
}
