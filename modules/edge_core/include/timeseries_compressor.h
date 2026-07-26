#ifndef TIMESERIES_COMPRESSOR_H
#define TIMESERIES_COMPRESSOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t timestamp_ms;
    double value;
} ts_sample_t;

int ts_compress_batch(const ts_sample_t *samples, size_t count, uint8_t *out_buf, size_t buf_capacity, size_t *out_compressed_len);
int ts_decompress_batch(const uint8_t *in_buf, size_t compressed_len, ts_sample_t *out_samples, size_t max_count, size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* TIMESERIES_COMPRESSOR_H */
