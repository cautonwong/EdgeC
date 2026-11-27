#ifndef DLT698_CODEC_SOURCE
#define DLT698_CODEC_SOURCE "../../foundation/src/protocol/dlt698_codec.c"
#endif
#include DLT698_CODEC_SOURCE

#ifndef FORMAL_MAX_LEN
#define FORMAL_MAX_LEN 32
#endif

int main(void) {
    uint16_t len;
    __CPROVER_assume(len <= FORMAL_MAX_LEN);
    uint8_t buf[len < 1 ? 1 : len];
    struct net_buf_simple in = { .data = buf, .len = len, .size = len, .__buf = buf };
    dlt698_addr_t sa = { 0 };
    uint8_t svc = 0, choice = 0, piid = 0, *payload_out = 0;
    size_t payload_len = 0;
    int rc = dlt698_decode_frame_ex(&in, &sa, &svc, &choice, &piid, &payload_out, &payload_len);

    __CPROVER_assert(rc == 0 || rc == 1 || rc == -1 || rc == -2,
                     "dlt698_decode: return code in contract");
    if (rc == 0) {
        __CPROVER_assert(sa.len >= 1 && sa.len <= 16, "dlt698_decode: sa.len in [1, 16]");
        __CPROVER_assert(payload_out >= buf && payload_out < buf + len,
                         "dlt698_decode: payload pointer inside input");
        __CPROVER_assert(payload_len <= len, "dlt698_decode: payload length within input");
        __CPROVER_assert((size_t)(payload_out - buf) + payload_len < len - 2,
                         "dlt698_decode: payload before trailing FCS and 0x16");
    }
    __CPROVER_cover(rc == 0);
    return 0;
}

int fcs_reject(void) {
    uint8_t frame[16] = {
        DLT698_HEAD, 0x0E, 0x00, 0x43, 0x00, 0x01, 0x10, 0x00, 0x00,
        DLT698_SVC_GET_REQ, 0x01, 0x01, 0xAA, 0x00, 0x00, DLT698_END
    };
    uint16_t hcs = dlt698_calculate_fcs(frame + 1, 6);
    frame[7] = (uint8_t)(hcs & 0xFF);
    frame[8] = (uint8_t)(hcs >> 8);
    uint16_t fcs = dlt698_calculate_fcs(frame + 1, 12);
    frame[13] = (uint8_t)(fcs & 0xFF);
    frame[14] = (uint8_t)(fcs >> 8);

    struct net_buf_simple in = {
        .data = frame, .len = sizeof(frame), .size = sizeof(frame), .__buf = frame
    };
    dlt698_addr_t sa = { 0 };
    uint8_t svc = 0, choice = 0, piid = 0, *payload_out = 0;
    size_t payload_len = 0;

    int rc = dlt698_decode_frame_ex(&in, &sa, &svc, &choice, &piid, &payload_out, &payload_len);
    __CPROVER_assert(rc == 0, "dlt698_decode: valid frame accepted");

    frame[7] ^= 1u;
    int hcs_rc = dlt698_decode_frame_ex(&in, &sa, &svc, &choice, &piid, &payload_out, &payload_len);
    __CPROVER_assert(hcs_rc < 0, "dlt698_decode: corrupted HCS rejected");
    frame[7] ^= 1u;

    frame[13] ^= 1u;
    int fcs_rc = dlt698_decode_frame_ex(&in, &sa, &svc, &choice, &piid, &payload_out, &payload_len);
    __CPROVER_assert(fcs_rc < 0, "dlt698_decode: corrupted FCS rejected");
    return 0;
}
