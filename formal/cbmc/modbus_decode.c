/*
 * CBMC harness for modbus_rtu_decode_frame_ex (foundation/src/protocol/modbus_codec.c).
 *
 * Bounds: input length 0..260, every input byte symbolic. The buffer is a VLA of
 * exactly `len` bytes so a read past in->len is a real out-of-bounds access for
 * CBMC. Main gate: --unwind 261 --unwinding-assertions; crc_reject entry point is
 * concrete and needs no unwind. Negative check: check_formal_negative.sh.
 *
 * The production .c is included directly; no test double. MODBUS_CODEC_SOURCE is
 * overridden only by the negative check with a mutated temporary copy.
 */
#ifndef MODBUS_CODEC_SOURCE
#define MODBUS_CODEC_SOURCE "../../foundation/src/protocol/modbus_codec.c"
#endif
#include MODBUS_CODEC_SOURCE

#ifndef FORMAL_MAX_LEN
#define FORMAL_MAX_LEN 260
#endif

int main(void) {
    uint16_t len;
    __CPROVER_assume(len <= FORMAL_MAX_LEN);
    uint8_t buf[len < 1 ? 1 : len];
    struct net_buf_simple in = { .data = buf, .len = len, .size = len, .__buf = buf };
    uint8_t slave_id = 0, fc = 0, *data_out = 0;
    size_t data_len = 0;
    int rc = modbus_rtu_decode_frame_ex(&in, false, &slave_id, &fc, &data_out, &data_len);

    __CPROVER_assert(rc == 0 || rc == 1 || rc == -1 || rc == -2,
                     "modbus_decode: return code in contract");
    if (rc == 0) {
        __CPROVER_assert(data_out >= buf && data_out <= buf + len,
                         "modbus_decode: output pointer inside input");
        __CPROVER_assert(data_len <= len, "modbus_decode: output length within input");
        __CPROVER_assert((size_t)(data_out - buf) + data_len < len,
                         "modbus_decode: output data before CRC field");
    }
    __CPROVER_cover(rc == 0);
    return 0;
}

/* Concrete CRC rejection: the same length guard does not cover this path, so a
 * corrupted CRC must be rejected by the checksum itself. */
int crc_reject(void) {
    uint8_t frame[7] = { 0x01, MODBUS_FC_READ_HOLDING_REGS, 0x02, 0xAA, 0xBB, 0, 0 };
    uint16_t crc = modbus_calculate_crc(frame, 5);
    frame[5] = (uint8_t)(crc & 0xFF);
    frame[6] = (uint8_t)(crc >> 8);
    struct net_buf_simple in = {
        .data = frame, .len = sizeof(frame), .size = sizeof(frame), .__buf = frame
    };
    uint8_t slave_id = 0, fc = 0, *data_out = 0;
    size_t data_len = 0;

    int rc = modbus_rtu_decode_frame_ex(&in, false, &slave_id, &fc, &data_out, &data_len);
    __CPROVER_assert(rc == 0, "modbus_decode: valid frame accepted");
    frame[5] ^= 1u;
    int crc_rc = modbus_rtu_decode_frame_ex(&in, false, &slave_id, &fc, &data_out, &data_len);
    __CPROVER_assert(crc_rc == -1, "modbus_decode: corrupted CRC rejected");
    return 0;
}
