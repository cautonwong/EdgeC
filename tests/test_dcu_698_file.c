/**
 * @file test_dcu_698_file.c
 * @brief Unit tests for DCU-D4-06b Class 18 (F000..F002) file transfer software domain engine.
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_file.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

/* Keep large test buffers in static storage to satisfy check_stack_usage.py. */
static uint8_t s_payload_512[512];

static void fill_pattern_512(void) {
    for (size_t i = 0; i < sizeof(s_payload_512); i++) {
        s_payload_512[i] = (uint8_t)((i * 37u + 13u) & 0xFFu);
    }
}

static void test_file_start_write_read_verify_512b(void **state) {
    (void)state;
    dcu_698_file_reset();
    fill_pattern_512();

    uint32_t expected_crc = dcu_698_file_crc32(s_payload_512, sizeof(s_payload_512));
    assert_int_equal(dcu_698_file_start("fw_v1.bin", 512u, 256u, expected_crc), 0);

    assert_int_equal(dcu_698_file_write_block(0u, s_payload_512, 256u), 0);
    assert_int_equal(dcu_698_file_write_block(1u, s_payload_512 + 256u, 256u), 0);

    uint8_t rd_buf[256];
    size_t rd_len = 0u;
    assert_int_equal(dcu_698_file_read_block(0u, rd_buf, sizeof(rd_buf), &rd_len), 0);
    assert_int_equal(rd_len, 256u);
    assert_memory_equal(rd_buf, s_payload_512, 256u);

    assert_int_equal(dcu_698_file_read_block(1u, rd_buf, sizeof(rd_buf), &rd_len), 0);
    assert_int_equal(rd_len, 256u);
    assert_memory_equal(rd_buf, s_payload_512 + 256u, 256u);

    assert_int_equal(dcu_698_file_verify(), 0);
    const dcu_file_session_t *sess = dcu_698_file_get_session();
    assert_true(sess->verified);
    assert_int_equal(sess->cmd_result, DCU_FILE_RES_OK);
}

static void test_file_idempotent_overwrite_and_tamper_fail(void **state) {
    (void)state;
    dcu_698_file_reset();
    fill_pattern_512();

    uint32_t expected_crc = dcu_698_file_crc32(s_payload_512, sizeof(s_payload_512));
    assert_int_equal(dcu_698_file_start("fw.bin", 512u, 256u, expected_crc), 0);

    /* Write corrupted block 0 first, then idempotently overwrite with valid block 0. */
    uint8_t bad_blk[256];
    memcpy(bad_blk, s_payload_512, 256u);
    bad_blk[10] ^= 0xFFu;
    assert_int_equal(dcu_698_file_write_block(0u, bad_blk, 256u), 0);
    assert_int_equal(dcu_698_file_write_block(0u, s_payload_512, 256u), 0);
    assert_int_equal(dcu_698_file_write_block(1u, s_payload_512 + 256u, 256u), 0);
    assert_int_equal(dcu_698_file_verify(), 0);

    /* Now tamper 1 byte in block 1 and verify must fail. */
    uint8_t tampered_blk[256];
    memcpy(tampered_blk, s_payload_512 + 256u, 256u);
    tampered_blk[0] ^= 0x01u;
    assert_int_equal(dcu_698_file_write_block(1u, tampered_blk, 256u), 0);
    assert_int_not_equal(dcu_698_file_verify(), 0);
    assert_false(dcu_698_file_get_session()->verified);
    assert_int_equal(dcu_698_file_get_session()->cmd_result, DCU_FILE_RES_VERIFY_FAIL);
}

static void test_file_missing_block_and_unwritten_read_rejected(void **state) {
    (void)state;
    dcu_698_file_reset();
    fill_pattern_512();

    uint32_t expected_crc = dcu_698_file_crc32(s_payload_512, sizeof(s_payload_512));
    assert_int_equal(dcu_698_file_start("fw.bin", 512u, 256u, expected_crc), 0);
    assert_int_equal(dcu_698_file_write_block(0u, s_payload_512, 256u), 0);

    /* Block 1 not written yet: read must fail, verify must fail. */
    uint8_t rd_buf[256];
    size_t rd_len = 99u;
    assert_int_equal(dcu_698_file_read_block(1u, rd_buf, sizeof(rd_buf), &rd_len),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(rd_len, 0u);
    assert_int_equal(dcu_698_file_verify(), EDGE_SLAVE_SRC_TEMPORARY);
}

static void test_file_bounds_and_inactive_rejected(void **state) {
    (void)state;
    dcu_698_file_reset();

    uint8_t blk[256];
    memset(blk, 0xAA, sizeof(blk));
    size_t rd_len = 0u;

    /* Inactive session read/write/verify rejected. */
    assert_int_not_equal(dcu_698_file_write_block(0u, blk, 16u), 0);
    assert_int_not_equal(dcu_698_file_read_block(0u, blk, sizeof(blk), &rd_len), 0);
    assert_int_not_equal(dcu_698_file_verify(), 0);

    /* Invalid start parameters rejected. */
    assert_int_not_equal(dcu_698_file_start(NULL, 512u, 256u, 0u), 0);
    assert_int_not_equal(dcu_698_file_start("", 512u, 256u, 0u), 0);
    assert_int_not_equal(dcu_698_file_start("big.bin", 16385u, 256u, 0u), 0);
    assert_int_not_equal(dcu_698_file_start("zero.bin", 0u, 256u, 0u), 0);
    assert_int_not_equal(dcu_698_file_start("badblk.bin", 512u, 0u, 0u), 0);
    assert_int_not_equal(dcu_698_file_start("badblk.bin", 512u, 257u, 0u), 0);
    /* Exceeds 64 blocks (e.g. 1000 bytes with block_size=8 => 125 blocks). */
    assert_int_not_equal(dcu_698_file_start("many.bin", 1000u, 8u, 0u), 0);

    /* Valid start: 300 bytes with block_size=256 => 2 blocks (256 + 44). */
    assert_int_equal(dcu_698_file_start("partial.bin", 300u, 256u, 0u), 0);
    /* Out-of-bounds block index >= total_blocks (2). */
    assert_int_not_equal(dcu_698_file_write_block(2u, blk, 256u), 0);
    assert_int_not_equal(dcu_698_file_read_block(2u, blk, sizeof(blk), &rd_len), 0);
    /* Block 1 exceeds total_size (256 + 100 > 300). */
    assert_int_not_equal(dcu_698_file_write_block(1u, blk, 100u), 0);
    /* Exact tail block length (44) succeeds. */
    assert_int_equal(dcu_698_file_write_block(1u, blk, 44u), 0);
}

static void test_file_encode_axdr_a2_and_a3(void **state) {
    (void)state;
    dcu_698_file_reset();
    fill_pattern_512();

    uint32_t expected_crc = dcu_698_file_crc32(s_payload_512, sizeof(s_payload_512));
    assert_int_equal(dcu_698_file_start("ota.bin", 512u, 256u, expected_crc), 0);
    assert_int_equal(dcu_698_file_write_block(0u, s_payload_512, 256u), 0);

    uint8_t buf[128];
    size_t out_len = 0u;
    assert_int_equal(dcu_698_file_encode_attr(0xF001u, 2u, buf, sizeof(buf), &out_len), 0);
    assert_int_equal(buf[0], DLT698_TAG_STRUCTURE);
    assert_int_equal(buf[1], 6u);
    assert_int_equal(buf[2], DLT698_TAG_VISIBLE_STRING);
    assert_int_equal(buf[3], 7u);
    assert_memory_equal(buf + 4, "ota.bin", 7u);

    /* A3 bitmap: 2 blocks, block 0 written (bit 0 = 1), block 1 not written => 0x01. */
    assert_int_equal(dcu_698_file_encode_attr(0xF001u, 3u, buf, sizeof(buf), &out_len), 0);
    assert_int_equal(out_len, 3u);
    assert_int_equal(buf[0], 0x04u);
    assert_int_equal(buf[1], 2u);
    assert_int_equal(buf[2], 0x01u);

    /* Write block 1 => bitmap becomes 0x03. */
    assert_int_equal(dcu_698_file_write_block(1u, s_payload_512 + 256u, 256u), 0);
    assert_int_equal(dcu_698_file_encode_attr(0xF001u, 3u, buf, sizeof(buf), &out_len), 0);
    assert_int_equal(out_len, 3u);
    assert_int_equal(buf[2], 0x03u);

    /* Unknown OI and unknown attr return expected DARs. */
    assert_int_equal(dcu_698_file_encode_attr(0x4300u, 2u, buf, sizeof(buf), &out_len),
                     EDGE_SLAVE_SRC_NOT_FOUND);
    assert_int_equal(dcu_698_file_encode_attr(0xF001u, 99u, buf, sizeof(buf), &out_len),
                     EDGE_SLAVE_SRC_UNDEFINED);
}

static void test_file_dfu_ports_uninjected_return_dar5(void **state) {
    (void)state;
    dcu_698_file_reset();

    assert_int_equal(dcu_698_file_activate(NULL, NULL), EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_file_rollback(NULL, NULL), EDGE_SLAVE_SRC_CLASS_MISMATCH);

    dcu_file_dfu_ops_t empty_ops = {0};
    assert_int_equal(dcu_698_file_activate(&empty_ops, NULL), EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_file_rollback(&empty_ops, NULL), EDGE_SLAVE_SRC_CLASS_MISMATCH);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_file_start_write_read_verify_512b),
        cmocka_unit_test(test_file_idempotent_overwrite_and_tamper_fail),
        cmocka_unit_test(test_file_missing_block_and_unwritten_read_rejected),
        cmocka_unit_test(test_file_bounds_and_inactive_rejected),
        cmocka_unit_test(test_file_encode_axdr_a2_and_a3),
        cmocka_unit_test(test_file_dfu_ports_uninjected_return_dar5),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
