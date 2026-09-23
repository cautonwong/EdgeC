/**
 * @file test_dcu_phase1_delivery.c
 * @brief Production Unit Tests for Phase 1 Delivery: DLT698.45 Management (DFU/Tariffs), AppDaemon
 * & ESAM Security
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcu_app_daemon.h"
#include "dcu_dlt698_mgmt_engine.h"
#include "edge_esam_driver.h"

static void test_dcu_dlt698_dfu_and_tariffs(void** state) {
    (void)state;

    dcu_dlt698_mgmt_t mgmt;
    dcu_mgmt_init(&mgmt);

    assert_int_equal(mgmt.dfu.state, DFU_STATE_IDLE);
    assert_int_equal(mgmt.dfu.active_bank, 0);  // Bank A

    // 1. Initiate DFU transfer of a 1024-byte firmware image (2 blocks of 512)
    uint8_t image[1024];
    for (int i = 0; i < 1024; i++) image[i] = (uint8_t)(i & 0xFF);

    // Compute expected CRC32
    uint32_t expected_crc = 0xFFFFFFFF;
    for (int i = 0; i < 1024; i++) {
        expected_crc ^= image[i];
        for (int j = 0; j < 8; j++) {
            expected_crc = (expected_crc >> 1) ^ (0xEDB88320 & (-(expected_crc & 1)));
        }
    }
    expected_crc = ~expected_crc;

    int rc = dcu_mgmt_dfu_start(&mgmt, "v2.5.0_SGCC", 1024, expected_crc);
    assert_int_equal(rc, 0);
    assert_int_equal(mgmt.dfu.expected_blocks, 2);

    // 2. Write block 0 and block 1
    rc = dcu_mgmt_dfu_write_block(&mgmt, 0, &image[0], 512);
    assert_int_equal(rc, 0);
    assert_int_equal(mgmt.dfu.received_blocks, 1);

    rc = dcu_mgmt_dfu_write_block(&mgmt, 1, &image[512], 512);
    assert_int_equal(rc, 0);
    assert_int_equal(mgmt.dfu.received_blocks, 2);
    assert_int_equal(mgmt.dfu.state, DFU_STATE_VERIFYING);

    // 3. Verify and activate Bank B
    rc = dcu_mgmt_dfu_verify_and_activate(&mgmt);
    assert_int_equal(rc, 0);
    assert_int_equal(mgmt.dfu.active_bank, 1);  // Swapped to Bank B
    assert_int_equal(mgmt.dfu.state, DFU_STATE_ACTIVATED);

    // 4. Test TOU Tariff Slot Update
    dcu_tou_slot_t new_slots[2] = {
        {.slot_id = 1, .start_hour = 0, .start_minute = 0, .tariff_rate = 4},  // Valley
        {.slot_id = 2, .start_hour = 12, .start_minute = 0, .tariff_rate = 1}  // Peak
    };
    rc = dcu_mgmt_set_tariff_table(&mgmt, new_slots, 2);
    assert_int_equal(rc, 0);
    assert_int_equal(mgmt.tariff_table.slot_count, 2);
    assert_int_equal(mgmt.tariff_table.version, 2);

    // 5. Test Security Session Authentication
    uint64_t now_ms = 1750000000000ULL;
    rc = dcu_mgmt_authenticate_session(&mgmt, 2, 3600, now_ms);
    assert_int_equal(rc, 0);
    assert_true(mgmt.sec_session.is_authenticated);
    assert_int_equal(mgmt.sec_session.security_level, 2);
}

static void test_dcu_app_daemon_lifecycle_and_quota(void** state) {
    (void)state;

    dcu_app_daemon_t daemon;
    uint64_t t0 = 1750000000000ULL;
    dcu_app_daemon_init(&daemon, t0);

    // 1. Install 2 container apps
    dcu_app_quota_t quota1 = {
        .max_cpu_pct = 20, .max_mem_bytes = 64 * 1024 * 1024, .max_flash_bytes = 32 * 1024 * 1024};
    int rc = dcu_app_daemon_install(&daemon, "LineLoss_Analytics_App", "1.0.0", 101, &quota1);
    assert_int_equal(rc, 0);

    dcu_app_quota_t quota2 = {
        .max_cpu_pct = 15, .max_mem_bytes = 32 * 1024 * 1024, .max_flash_bytes = 16 * 1024 * 1024};
    rc = dcu_app_daemon_install(&daemon, "VVC_Optimizer_App", "1.1.0", 102, &quota2);
    assert_int_equal(rc, 0);
    assert_int_equal(daemon.app_count, 2);

    // 2. Start Apps
    assert_int_equal(dcu_app_daemon_start(&daemon, 101, t0), 0);
    assert_int_equal(dcu_app_daemon_start(&daemon, 102, t0), 0);
    assert_int_equal(daemon.apps[0].state, APP_STATE_RUNNING);

    // 3. Normal Heartbeat within quota (CPU 12% <= 20%, Mem 16MB <= 64MB)
    rc = dcu_app_daemon_heartbeat(&daemon, 101, 12.5, 16 * 1024 * 1024, t0 + 1000);
    assert_int_equal(rc, 0);
    assert_false(daemon.apps[0].is_quota_exceeded);

    // 4. Over-quota Heartbeat (CPU 28% > 20%)
    rc = dcu_app_daemon_heartbeat(&daemon, 101, 28.0, 16 * 1024 * 1024, t0 + 2000);
    assert_int_equal(rc, 0);
    assert_true(daemon.apps[0].is_quota_exceeded);

    // 5. Watchdog poll: detect exceeded quota -> auto-recover / restart
    int recovered = dcu_app_daemon_watchdog_poll(&daemon, 5000, t0 + 2500);
    assert_int_equal(recovered, 1);
    assert_int_equal(daemon.apps[0].restart_count, 1);
    assert_false(daemon.apps[0].is_quota_exceeded);

    // 6. Stop App
    assert_int_equal(dcu_app_daemon_stop(&daemon, 101), 0);
    assert_int_equal(daemon.apps[0].state, APP_STATE_STOPPED);
}

static int mock_spi_transmit(const uint8_t* tx, uint8_t* rx, uint16_t len, void* user_data) {
    (void)user_data;
    if (!tx || !rx || len < 4) return -1;
    uint8_t ins = tx[1];
    uint8_t le = (len > 4) ? tx[len - 1] : 8;

    rx[0] = le;         // Length
    if (ins == 0x84) {  // Random
        for (uint8_t i = 1; i <= le; i++) rx[i] = (uint8_t)(0xA5 ^ i);
    } else if (ins == 0x3C) {  // SM3 hash
        rx[0] = 32;
        for (uint8_t i = 1; i <= 32; i++) rx[i] = (uint8_t)(0x3C ^ i);
    } else if (ins == 0x3A) {  // SM4 CBC
        uint8_t lc = tx[4];
        rx[0] = lc;
        for (uint8_t i = 1; i <= lc; i++) rx[i] = (uint8_t)(tx[4 + i] ^ 0x4B);
    } else {
        rx[0] = 0;
    }
    return 0;
}

static void test_edge_esam_driver_apdu(void** state) {
    (void)state;

    esam_driver_t esam;
    int rc = esam_driver_init(&esam, mock_spi_transmit, NULL);  // Bound to physical SPI interface
    assert_int_equal(rc, 0);
    assert_true(esam.is_initialized);

    // 1. Get Random 8 bytes (0x0084)
    uint8_t rand_buf[8] = {0};
    rc = esam_get_random(&esam, rand_buf, 8);
    assert_int_equal(rc, 0);
    assert_true(rand_buf[0] != 0 || rand_buf[1] != 0);

    // 2. SM3 Hash (0x803C)
    uint8_t msg[16] = "EdgeOS_DCU_2026";
    uint8_t hash_out[32];
    rc = esam_compute_sm3_hash(&esam, msg, 15, hash_out);
    assert_int_equal(rc, 0);

    // 3. SM4 CBC Encryption (0x803A)
    uint8_t plaintext[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                             0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    uint8_t iv[16] = {0};
    uint8_t ciphertext[16];
    rc = esam_sm4_cbc_encrypt(&esam, 1, iv, plaintext, 16, ciphertext);
    assert_int_equal(rc, 0);
    assert_true(memcmp(plaintext, ciphertext, 16) != 0);

    // 4. SM2 Verify Signature (0x802A)
    uint8_t sig_r[32] = {0xAA};
    uint8_t sig_s[32] = {0xBB};
    rc = esam_sm2_verify_signature(&esam, hash_out, sig_r, sig_s);
    assert_int_equal(rc, 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_dlt698_dfu_and_tariffs),
        cmocka_unit_test(test_dcu_app_daemon_lifecycle_and_quota),
        cmocka_unit_test(test_edge_esam_driver_apdu),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
