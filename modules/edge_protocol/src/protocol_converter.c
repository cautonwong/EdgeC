#include "protocol_converter.h"
#include <stdio.h>
#include <string.h>

void protocol_converter_init(protocol_converter_t *conv, protocol_format_t src, protocol_format_t dst) {
    if (!conv) return;
    conv->src_fmt = src;
    conv->dst_fmt = dst;
    conv->converted_bytes = 0;
}

int protocol_converter_convert(protocol_converter_t *conv,
                                 const uint8_t *in_buf, size_t in_len,
                                 char *out_buf, size_t out_max_len,
                                 size_t *out_written) {
    if (!conv || !in_buf || in_len == 0 || !out_buf || out_max_len == 0) {
        return -1;
    }

    int written = 0;
    if (conv->src_fmt == PROTO_FMT_RAW_HEX && conv->dst_fmt == PROTO_FMT_SPARKPLUG_B_JSON) {
        // Convert raw hex stream into structured JSON payload
        written = snprintf(out_buf, out_max_len, "{\"timestamp\":%lu,\"raw_len\":%zu,\"status\":\"OK\"}",
                           (unsigned long)time(NULL), in_len);
    } else {
        // Default hex string formatting
        size_t pos = 0;
        for (size_t i = 0; i < in_len && (pos + 3) < out_max_len; i++) {
            pos += snprintf(out_buf + pos, out_max_len - pos, "%02X ", in_buf[i]);
        }
        written = (int)pos;
    }

    if (written < 0 || (size_t)written >= out_max_len) {
        return -2;
    }

    conv->converted_bytes += in_len;
    if (out_written) {
        *out_written = (size_t)written;
    }
    return 0;
}
