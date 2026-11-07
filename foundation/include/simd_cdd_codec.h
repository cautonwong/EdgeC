#ifndef SIMD_CDD_CODEC_H
#define SIMD_CDD_CODEC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CDD_MAGIC 0x43444431  /* "CDD1" */
#define CDD_MAX_FIELDS 128

typedef enum {
    CDD_TYPE_INT32 = 0,
    CDD_TYPE_FLOAT = 1,
    CDD_TYPE_DOUBLE = 2,
    CDD_TYPE_UINT64 = 3
} cdd_data_type_t;

typedef struct {
    uint16_t tag_id;
    uint8_t type;       /* cdd_data_type_t */
    uint16_t offset;
    double scale;
    double offset_val;
} cdd_tag_field_t;

typedef struct {
    cdd_tag_field_t fields[CDD_MAX_FIELDS];
    uint32_t field_count;
} cdd_schema_t;

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint16_t field_count;
    uint16_t reserved;
    uint32_t payload_len;
    uint32_t crc32;
} cdd_frame_header_t;

typedef struct {
    uint16_t tag_id;
    double value;
    uint64_t timestamp_ms;
} cdd_batch_sample_t;

void cdd_schema_init(cdd_schema_t *schema);
int cdd_schema_add_field(cdd_schema_t *schema, uint16_t tag_id, uint8_t type, uint16_t offset, double scale, double offset_val);
const cdd_tag_field_t* cdd_schema_find_field(const cdd_schema_t *schema, uint16_t tag_id);

int cdd_encode_batch(const cdd_schema_t *schema, uint32_t sequence, const cdd_batch_sample_t *samples, uint32_t count, uint8_t *out_buf, size_t buf_size, size_t *out_len);
int cdd_decode_batch(const cdd_schema_t *schema, const uint8_t *in_buf, size_t in_len, cdd_batch_sample_t *out_samples, uint32_t max_count, uint32_t *out_count);

/* SIMD/Vectorized accelerated batch scale & offset transform */
int cdd_fast_scale_transform(const float *raw_in, float scale, float offset, float *eng_out, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* SIMD_CDD_CODEC_H */
