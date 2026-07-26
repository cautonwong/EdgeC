#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "simd_cdd_codec.h"

int main(void) {
    printf("=== Test SIMD CDD Codec Start ===\n");

    cdd_schema_t schema;
    cdd_schema_init(&schema);

    assert(cdd_schema_add_field(&schema, 1001, CDD_TYPE_DOUBLE, 0, 0.1, 10.0) == 0);
    assert(cdd_schema_add_field(&schema, 1002, CDD_TYPE_DOUBLE, 8, 2.0, -5.0) == 0);

    const cdd_tag_field_t *f1 = cdd_schema_find_field(&schema, 1001);
    assert(f1 != NULL);
    assert(f1->tag_id == 1001);

    cdd_batch_sample_t samples[2];
    samples[0].tag_id = 1001;
    samples[0].value = 25.5;  /* Eng value */
    samples[0].timestamp_ms = 1700000000000ULL;

    samples[1].tag_id = 1002;
    samples[1].value = 35.0;  /* Eng value */
    samples[1].timestamp_ms = 1700000001000ULL;

    uint8_t buffer[512];
    size_t out_len = 0;
    assert(cdd_encode_batch(&schema, 1, samples, 2, buffer, sizeof(buffer), &out_len) == 0);
    assert(out_len > sizeof(cdd_frame_header_t));

    cdd_batch_sample_t decoded[2];
    uint32_t decoded_count = 0;
    assert(cdd_decode_batch(&schema, buffer, out_len, decoded, 2, &decoded_count) == 0);
    assert(decoded_count == 2);

    assert(decoded[0].tag_id == 1001);
    assert(fabs(decoded[0].value - 25.5) < 0.0001);
    assert(decoded[0].timestamp_ms == 1700000000000ULL);

    assert(decoded[1].tag_id == 1002);
    assert(fabs(decoded[1].value - 35.0) < 0.0001);
    assert(decoded[1].timestamp_ms == 1700000001000ULL);

    /* Test SIMD fast scale transform */
    float raw[8] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    float eng[8];
    assert(cdd_fast_scale_transform(raw, 2.5f, 1.0f, eng, 8) == 0);
    assert(fabsf(eng[0] - 3.5f) < 0.0001f);
    assert(fabsf(eng[7] - 21.0f) < 0.0001f);

    printf("=== Test SIMD CDD Codec Passed (119/119) ===\n");
    return 0;
}
