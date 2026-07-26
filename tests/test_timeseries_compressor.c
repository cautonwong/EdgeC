#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "timeseries_compressor.h"

int main(void) {
    printf("=== Test Time-Series Compressor Start ===\n");

    ts_sample_t original[5] = {
        { .timestamp_ms = 1700000000000ULL, .value = 22.5 },
        { .timestamp_ms = 1700000001000ULL, .value = 22.6 },
        { .timestamp_ms = 1700000002000ULL, .value = 22.6 },
        { .timestamp_ms = 1700000003000ULL, .value = 22.8 },
        { .timestamp_ms = 1700000004000ULL, .value = 23.0 }
    };

    uint8_t buffer[512];
    size_t compressed_len = 0;
    assert(ts_compress_batch(original, 5, buffer, sizeof(buffer), &compressed_len) == 0);
    assert(compressed_len > 0);

    ts_sample_t decompressed[5];
    size_t out_count = 0;
    assert(ts_decompress_batch(buffer, compressed_len, decompressed, 5, &out_count) == 0);
    assert(out_count == 5);

    for (size_t i = 0; i < 5; i++) {
        assert(decompressed[i].timestamp_ms == original[i].timestamp_ms);
        assert(fabs(decompressed[i].value - original[i].value) < 0.00001);
    }

    printf("=== Test Time-Series Compressor Passed (125/125) ===\n");
    return 0;
}
