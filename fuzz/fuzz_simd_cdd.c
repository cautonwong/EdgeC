#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "simd_cdd_codec.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    cdd_schema_t schema;
    cdd_schema_init(&schema);
    cdd_schema_add_field(&schema, 1, CDD_TYPE_DOUBLE, 0, 1.0, 0.0);

    cdd_batch_sample_t out[8];
    uint32_t count = 0;
    cdd_decode_batch(&schema, data, size, out, 8, &count);
    return 0;
}

#ifdef STANDALONE_FUZZER
int main(void) {
    const uint8_t seed[] = {0x31, 0x44, 0x44, 0x43, 0x01, 0x00, 0x00, 0x00};
    LLVMFuzzerTestOneInput(seed, sizeof(seed));
    printf("[FUZZ_STANDALONE] SIMD CDD production codec executed with seed corpus.\n");
    return 0;
}
#endif
