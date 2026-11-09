#ifndef PROTOCOL_CONVERTER_H
#define PROTOCOL_CONVERTER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PROTO_FMT_RAW_HEX = 0,
    PROTO_FMT_MODBUS_RTU,
    PROTO_FMT_DLT645_2007,
    PROTO_FMT_SPARKPLUG_B_JSON
} protocol_format_t;

typedef struct {
    protocol_format_t src_fmt;
    protocol_format_t dst_fmt;
    size_t converted_bytes;
} protocol_converter_t;

void protocol_converter_init(protocol_converter_t *conv, protocol_format_t src, protocol_format_t dst);

int protocol_converter_convert(protocol_converter_t *conv,
                                 const uint8_t *in_buf, size_t in_len,
                                 char *out_buf, size_t out_max_len,
                                 size_t *out_written);

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_CONVERTER_H
