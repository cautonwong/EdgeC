/**
 * @file test_npu_enclave.c
 * @brief Unit Tests for NPU Model Encryption, Secure Enclave & Hardware Signature Verification
 * (AEP-749 / Issue #279)
 */

#include <cmocka.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "datahub.h"
#include "npu_enclave.h"
#include "npu_hal.h"

static void test_npu_enclave_package_and_decrypt_success(void **state) {
    (void)state;

    npu_enclave_storage_t storage;
    memset(&storage, 0x55, sizeof(storage));

    npu_enclave_config_t cfg = {.enforce_signature_check = true,
                                .enforce_integrity_sha256 = true,
                                .zeroize_on_unload = true};

    npu_enclave_t *enclave = npu_enclave_create_in(&storage, &cfg);
    assert_non_null(enclave);

    /* 1. Setup Keys */
    uint8_t root_key[32] = {0x10, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB,
                            0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                            0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    uint8_t model_key[32] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33, 0x44, 0x55,
                             0x66, 0x77, 0x88, 0x99, 0x00, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6,
                             0x17, 0x28, 0x39, 0x4A, 0x5B, 0x6C, 0x7D, 0x8E, 0x9F, 0x0A};

    assert_int_equal(
        npu_enclave_set_root_pubkey(enclave, root_key, sizeof(root_key), NPU_SIG_ED25519), 0);
    assert_int_equal(npu_enclave_register_model_key(enclave, 42, model_key, sizeof(model_key)), 0);

    /* 2. Create Dummy Plaintext Model Weights */
    const char plaintext_model[] =
        "TENSOR_FLOW_LITE_MODEL_WEIGHTS_CONV2D_RESNET50_INDUSTRIAL_FAULT_DETECTION_MATRIX";
    size_t plaintext_len = strlen(plaintext_model) + 1;

    uint8_t package_buf[512];
    size_t package_size = 0;

    int rc = npu_enclave_package_model(42, NPU_CIPHER_AES_256_GCM, NPU_SIG_ED25519, model_key,
                                       root_key, plaintext_model, plaintext_len, package_buf,
                                       sizeof(package_buf), &package_size);
    assert_int_equal(rc, 0);
    assert_true(package_size > plaintext_len);

    /* Verify that ciphertext does NOT contain plaintext string */
    assert_null(strstr((char *)package_buf + sizeof(npu_enc_model_header_t), "TENSOR_FLOW_LITE"));

    /* 3. Decrypt in Enclave */
    uint8_t decrypted_buf[256];
    size_t decrypted_size = 0;
    rc = npu_enclave_decrypt_model(enclave, package_buf, package_size, decrypted_buf,
                                   sizeof(decrypted_buf), &decrypted_size);
    assert_int_equal(rc, 0);
    assert_int_equal(decrypted_size, plaintext_len);
    assert_string_equal((char *)decrypted_buf, plaintext_model);

    /* 4. Check Enclave Diagnostics */
    npu_enclave_auth_status_t status;
    float decrypt_time_us = 0.0f;
    uint32_t tamper_count = 0;
    npu_enclave_get_status(enclave, &status, &decrypt_time_us, &tamper_count);
    assert_int_equal(status, NPU_ENCLAVE_AUTH_PASSED);
    assert_int_equal(tamper_count, 0);
}

static void test_npu_enclave_tamper_detection_and_security(void **state) {
    (void)state;

    npu_enclave_storage_t storage;
    npu_enclave_config_t cfg = {.enforce_signature_check = true,
                                .enforce_integrity_sha256 = true,
                                .zeroize_on_unload = true};
    npu_enclave_t *enclave = npu_enclave_create_in(&storage, &cfg);

    uint8_t root_key[32] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                            17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    uint8_t model_key[32] = {32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,
                             16, 15, 14, 13, 12, 11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1};

    npu_enclave_set_root_pubkey(enclave, root_key, sizeof(root_key), NPU_SIG_ED25519);
    npu_enclave_register_model_key(enclave, 99, model_key, sizeof(model_key));

    const char raw_model[] = "CRITICAL_GRID_ISLANDING_PROTECTION_NEURAL_NETWORK";
    size_t raw_len = strlen(raw_model) + 1;

    uint8_t package_buf[512];
    size_t package_size = 0;
    npu_enclave_package_model(99, NPU_CIPHER_AES_256_GCM, NPU_SIG_ED25519, model_key, root_key,
                              raw_model, raw_len, package_buf, sizeof(package_buf), &package_size);

    /* Tamper Test 1: Bit flip in ciphertext */
    uint8_t tampered_package[512];
    memcpy(tampered_package, package_buf, package_size);
    tampered_package[sizeof(npu_enc_model_header_t) + 4] ^= 0xFF; /* Corrupt byte */

    uint8_t dec_buf[256];
    size_t dec_size = 0;
    int rc = npu_enclave_decrypt_model(enclave, tampered_package, package_size, dec_buf,
                                       sizeof(dec_buf), &dec_size);
    assert_true(rc < 0); /* Must reject corrupted payload */

    npu_enclave_auth_status_t status;
    uint32_t tamper_count = 0;
    npu_enclave_get_status(enclave, &status, NULL, &tamper_count);
    assert_true(status == NPU_ENCLAVE_AUTH_SIG_INVALID || status == NPU_ENCLAVE_AUTH_TAMPER_DETECT);
    assert_int_equal(tamper_count, 1);

    /* DataHub Sync */
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    shm_unlink("/test_npu_enclave_hub");
    assert_int_equal(
        datahub_init(&hub, &loop, "/test_npu_enclave_hub", 32, 64, NULL, 0, NULL, NULL, NULL), 0);
    assert_int_equal(npu_enclave_sync_datahub(enclave, &hub), 0);
    datahub_destroy(&hub);
    uv_loop_close(&loop);
    shm_unlink("/test_npu_enclave_hub");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_npu_enclave_package_and_decrypt_success),
        cmocka_unit_test(test_npu_enclave_tamper_detection_and_security),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
