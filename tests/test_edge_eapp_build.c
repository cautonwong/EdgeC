/**
 * @file test_edge_eapp_build.c
 * @brief Build-then-verify round trips for `.eapp` packages.
 */

#include <cmocka.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "edgeos/edge_eapp.h"

static const char *kManifest =
    "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"2.1.0\",\"language\":\"c\","
    "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
    "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
    "\"publishPerSec\":1}}";

static const uint8_t kWasm[] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};

static const uint8_t kSeed[32] = {
    9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
};

static void derive_public(const uint8_t seed[32], uint8_t pub[32]) {
    EVP_PKEY *key = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, seed, 32);
    size_t len = 32;
    assert_non_null(key);
    assert_int_equal(EVP_PKEY_get_raw_public_key(key, pub, &len), 1);
    assert_int_equal(len, 32);
    EVP_PKEY_free(key);
}

static void test_build_round_trip(void **state) {
    (void)state;
    void *package = NULL;
    size_t package_len = 0;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(edge_eapp_build(kManifest, kWasm, sizeof(kWasm), kSeed, &package, &package_len,
                                     reason, sizeof(reason)),
                     0);
    assert_non_null(package);
    assert_true(package_len > sizeof(kWasm));

    uint8_t pub[32];
    derive_public(kSeed, pub);
    edge_eapp_key_t ring;
    memset(&ring, 0, sizeof(ring));
    snprintf(ring.publisher_id, sizeof(ring.publisher_id), "test");
    memcpy(ring.public_key, pub, sizeof(pub));
    edge_eapp_package_t verified;
    assert_int_equal(
        edge_eapp_validate(package, package_len, &ring, 1, &verified, reason, sizeof(reason)), 0);
    assert_string_equal(verified.manifest_json, kManifest);
    assert_int_equal(verified.wasm_len, sizeof(kWasm));
    assert_memory_equal(verified.wasm, kWasm, sizeof(kWasm));
    edge_eapp_package_free(&verified);
    free(package);
}

static void test_build_is_deterministic(void **state) {
    (void)state;
    void *first = NULL;
    void *second = NULL;
    size_t first_len = 0;
    size_t second_len = 0;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(
        edge_eapp_build(kManifest, kWasm, sizeof(kWasm), kSeed, &first, &first_len, reason,
                        sizeof(reason)),
        0);
    assert_int_equal(
        edge_eapp_build(kManifest, kWasm, sizeof(kWasm), kSeed, &second, &second_len, reason,
                        sizeof(reason)),
        0);
    assert_int_equal(first_len, second_len);
    assert_memory_equal(first, second, first_len);
    free(first);
    free(second);
}

static void test_build_rejects_invalid_manifest(void **state) {
    (void)state;
    void *package = (void *)0x1;
    size_t package_len = 7;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(edge_eapp_build("{}", kWasm, sizeof(kWasm), kSeed, &package, &package_len,
                                     reason, sizeof(reason)),
                     -1);
    assert_null(package);
    assert_int_equal(package_len, 0);
    assert_int_equal(strncmp(reason, "manifest: ", 10), 0);
}

static void test_build_rejects_denied_import(void **state) {
    (void)state;
    uint8_t wasm[64];
    const uint8_t header[8] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    memcpy(wasm, header, sizeof(header));
    size_t pos = sizeof(header);
    wasm[pos++] = 0x02; /* import section */
    size_t size_pos = pos++;
    wasm[pos++] = 1;
    const char *module = "env";
    wasm[pos++] = (uint8_t)strlen(module);
    memcpy(wasm + pos, module, strlen(module));
    pos += strlen(module);
    const char *name = "system_execute";
    wasm[pos++] = (uint8_t)strlen(name);
    memcpy(wasm + pos, name, strlen(name));
    pos += strlen(name);
    wasm[pos++] = 0x00;
    wasm[pos++] = 0x00;
    wasm[size_pos] = (uint8_t)(pos - size_pos - 1);

    void *package = NULL;
    size_t package_len = 0;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(edge_eapp_build(kManifest, wasm, pos, kSeed, &package, &package_len, reason,
                                     sizeof(reason)),
                     -1);
    assert_null(package);
    assert_int_equal(strncmp(reason, "wasm: denied_import: env.system_execute",
                             strlen("wasm: denied_import: env.system_execute")),
                     0);
}

static void test_build_rejects_oversize_manifest(void **state) {
    (void)state;
    size_t len = EDGE_EAPP_MAX_MANIFEST + 1;
    char *manifest = malloc(len + 1);
    assert_non_null(manifest);
    memset(manifest, 'a', len);
    manifest[len] = '\0';

    void *package = NULL;
    size_t package_len = 0;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(edge_eapp_build(manifest, kWasm, sizeof(kWasm), kSeed, &package, &package_len,
                                     reason, sizeof(reason)),
                     -1);
    assert_string_equal(reason, "manifest_too_large");
    free(manifest);
}

static void test_build_bad_args(void **state) {
    (void)state;
    void *package = NULL;
    size_t package_len = 0;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(edge_eapp_build(NULL, kWasm, sizeof(kWasm), kSeed, &package, &package_len,
                                     reason, sizeof(reason)),
                     -1);
    assert_string_equal(reason, "bad_args");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_build_round_trip),
        cmocka_unit_test(test_build_is_deterministic),
        cmocka_unit_test(test_build_rejects_invalid_manifest),
        cmocka_unit_test(test_build_rejects_denied_import),
        cmocka_unit_test(test_build_rejects_oversize_manifest),
        cmocka_unit_test(test_build_bad_args),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
