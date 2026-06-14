#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ws_proto.h"

static void test_ws_binary_telemetry(void** state) {
    (void)state;

    uint32_t count = 1024;
    double* arr = malloc(count * sizeof(double));
    for (uint32_t i = 0; i < count; i++) {
        arr[i] = (double)i * 3.14;
    }

    // 1. Serialize Double Array
    size_t bin_len;
    uint8_t* bin_buf = malloc(sizeof(ws_bin_header_t) + count * sizeof(double));
    assert_int_equal(ws_serialize_double_array(999, arr, count, bin_buf, &bin_len), 0);

    assert_int_equal(bin_len, 9 + 1024 * 8);  // 9 bytes header + 8192 bytes payload = 8201

    // Check Header
    ws_bin_header_t* hdr = (ws_bin_header_t*)bin_buf;
    assert_int_equal(hdr->tag_id, 999);
    assert_int_equal(hdr->element_count, 1024);
    assert_int_equal(hdr->data_type, 0x01);

    // 2. Build WebSocket Frame
    // 8201 is > 125 and < 65535, so it should use the 16-bit length header (4 bytes)
    size_t frame_len;
    uint8_t* frame_buf = malloc(bin_len + 10);
    assert_int_equal(ws_build_frame(WS_OP_BIN, bin_buf, bin_len, frame_buf, &frame_len), 0);

    assert_int_equal(frame_len, bin_len + 4);
    assert_int_equal(frame_buf[0], 0x80 | WS_OP_BIN);
    assert_int_equal(frame_buf[1], 126);
    uint16_t parsed_len = (frame_buf[2] << 8) | frame_buf[3];
    assert_int_equal(parsed_len, bin_len);

    // 3. Test Huge Payload (Force 64-bit length)
    size_t huge_len = 70000;
    uint8_t* huge_buf = calloc(1, huge_len);
    size_t huge_frame_len;
    uint8_t* huge_frame_buf = malloc(huge_len + 10);

    assert_int_equal(ws_build_frame(WS_OP_BIN, huge_buf, huge_len, huge_frame_buf, &huge_frame_len),
                     0);
    assert_int_equal(huge_frame_len, huge_len + 10);
    assert_int_equal(huge_frame_buf[0], 0x80 | WS_OP_BIN);
    assert_int_equal(huge_frame_buf[1], 127);

    uint64_t parsed_huge_len = 0;
    for (int i = 0; i < 8; i++) {
        parsed_huge_len = (parsed_huge_len << 8) | huge_frame_buf[2 + i];
    }
    assert_int_equal(parsed_huge_len, huge_len);

    free(arr);
    free(bin_buf);
    free(frame_buf);
    free(huge_buf);
    free(huge_frame_buf);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ws_binary_telemetry),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
