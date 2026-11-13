/**
 * @file test_edge_eapp.c
 * @brief `.eapp` container bounds, signature and content validation.
 */

#include <cmocka.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "edgeos/edge_eapp.h"

static const char *kManifest =
    "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
    "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
    "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
    "\"publishPerSec\":1}}";

static const uint8_t kWasm[] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};

static const uint8_t kSeedA[32] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
};
static const uint8_t kSeedB[32] = {
    32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,
    16, 15, 14, 13, 12, 11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,
};

static void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
    p[2] = (uint8_t)((v >> 16) & 0xff);
    p[3] = (uint8_t)((v >> 24) & 0xff);
}

static void public_key_from_seed(const uint8_t seed[32], uint8_t pub[32]) {
    EVP_PKEY *key = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, seed, 32);
    size_t len = 32;
    assert_non_null(key);
    assert_int_equal(EVP_PKEY_get_raw_public_key(key, pub, &len), 1);
    assert_int_equal(len, 32);
    EVP_PKEY_free(key);
}

static void key_id_from_seed(const uint8_t seed[32], uint8_t key_id[32]) {
    uint8_t pub[32];
    unsigned int len = 0;
    public_key_from_seed(seed, pub);
    assert_int_equal(EVP_Digest(pub, sizeof(pub), key_id, &len, EVP_sha256(), NULL), 1);
    assert_int_equal(len, 32);
}

static int validate_with(const uint8_t *package, size_t len, const uint8_t pub[32],
                         edge_eapp_package_t *out, char *reason, size_t reason_size) {
    edge_eapp_key_t ring;
    memset(&ring, 0, sizeof(ring));
    snprintf(ring.publisher_id, sizeof(ring.publisher_id), "acme");
    memcpy(ring.public_key, pub, 32);
    ring.trust_level = 2;
    ring.verified = true;
    return edge_eapp_validate(package, len, &ring, 1, out, reason, reason_size);
}

static void sign_message(const uint8_t seed[32], const uint8_t *message, size_t message_len,
                         uint8_t signature[64]) {
    EVP_PKEY *key = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, seed, 32);
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    size_t sig_len = 64;
    assert_non_null(key);
    assert_non_null(ctx);
    assert_int_equal(EVP_DigestSignInit(ctx, NULL, NULL, NULL, key), 1);
    assert_int_equal(EVP_DigestSign(ctx, signature, &sig_len, message, message_len), 1);
    assert_int_equal(sig_len, 64);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(key);
}

static size_t build_package(const char *manifest, const uint8_t *wasm, size_t wasm_len,
                            const uint8_t seed[32], uint8_t *out) {
    size_t manifest_len = strlen(manifest);
    size_t pos = 0;
    memcpy(out, EDGE_EAPP_MAGIC, EDGE_EAPP_MAGIC_LEN);
    pos += EDGE_EAPP_MAGIC_LEN;
    key_id_from_seed(seed, out + pos);
    pos += EDGE_EAPP_KEY_ID_LEN;
    put_u32(out + pos, (uint32_t)manifest_len);
    pos += 4;
    memcpy(out + pos, manifest, manifest_len);
    pos += manifest_len;
    put_u32(out + pos, (uint32_t)wasm_len);
    pos += 4;
    memcpy(out + pos, wasm, wasm_len);
    pos += wasm_len;
    put_u32(out + pos, EDGE_EAPP_SIGNATURE_LEN);
    pos += 4;
    sign_message(seed, out, pos, out + pos);
    return pos + EDGE_EAPP_SIGNATURE_LEN;
}

static size_t put_name(uint8_t *buf, size_t pos, const char *text) {
    size_t length = strlen(text);
    buf[pos++] = (uint8_t)length;
    memcpy(buf + pos, text, length);
    return pos + length;
}

static size_t build_denied_import_wasm(uint8_t *buf) {
    const uint8_t header[8] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    memcpy(buf, header, sizeof(header));
    size_t pos = sizeof(header);
    buf[pos++] = 0x02; /* import section */
    size_t size_pos = pos++;
    buf[pos++] = 1; /* one import */
    pos = put_name(buf, pos, "env");
    pos = put_name(buf, pos, "system_execute");
    buf[pos++] = 0x00; /* kind: func */
    buf[pos++] = 0x00; /* type index */
    buf[size_pos] = (uint8_t)(pos - size_pos - 1);
    return pos;
}

static void test_valid_package(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[2048];
    public_key_from_seed(kSeedA, pub);
    size_t len = build_package(kManifest, kWasm, sizeof(kWasm), kSeedA, package);

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, len, pub, &out, reason, sizeof(reason)), 0);
    assert_string_equal(out.manifest_json, kManifest);
    assert_int_equal(out.manifest_len, strlen(kManifest));
    assert_int_equal(out.wasm_len, sizeof(kWasm));
    assert_memory_equal(out.wasm, kWasm, sizeof(kWasm));

    edge_eapp_package_free(&out);
    assert_null(out.manifest_json);
    assert_null(out.wasm);
}

static void test_tampered_wasm(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[2048];
    public_key_from_seed(kSeedA, pub);
    size_t len = build_package(kManifest, kWasm, sizeof(kWasm), kSeedA, package);
    package[len - EDGE_EAPP_SIGNATURE_LEN - 4 - 1] ^= 0xff; /* last wasm byte */

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, len, pub, &out, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "bad_signature");
}

static void test_wrong_key(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[2048];
    public_key_from_seed(kSeedB, pub);
    size_t len = build_package(kManifest, kWasm, sizeof(kWasm), kSeedA, package);

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, len, pub, &out, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "unknown_key");
}

static void test_swapped_key_id_is_a_bad_signature(void **state) {
    (void)state;
    uint8_t pub_a[32];
    uint8_t package[2048];
    public_key_from_seed(kSeedA, pub_a);
    size_t len = build_package(kManifest, kWasm, sizeof(kWasm), kSeedA, package);
    key_id_from_seed(kSeedB, package + EDGE_EAPP_MAGIC_LEN);

    edge_eapp_key_t ring[2];
    memset(ring, 0, sizeof(ring));
    snprintf(ring[0].publisher_id, sizeof(ring[0].publisher_id), "a");
    public_key_from_seed(kSeedA, ring[0].public_key);
    snprintf(ring[1].publisher_id, sizeof(ring[1].publisher_id), "b");
    public_key_from_seed(kSeedB, ring[1].public_key);

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(edge_eapp_validate(package, len, ring, 2, &out, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "bad_signature");
}

static void test_bad_magic(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[2048];
    public_key_from_seed(kSeedA, pub);
    size_t len = build_package(kManifest, kWasm, sizeof(kWasm), kSeedA, package);
    package[0] = 'X';

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, len, pub, &out, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "bad_magic");
}

static void test_truncated_signature(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[2048];
    public_key_from_seed(kSeedA, pub);
    size_t len = build_package(kManifest, kWasm, sizeof(kWasm), kSeedA, package);

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, len - 1, pub, &out, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "truncated");
}

static void test_bad_signature_len(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[2048];
    public_key_from_seed(kSeedA, pub);
    size_t len = build_package(kManifest, kWasm, sizeof(kWasm), kSeedA, package);
    put_u32(package + len - EDGE_EAPP_SIGNATURE_LEN - 4, 63);

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, len, pub, &out, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "bad_signature_len");
}

static void test_trailing_data(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[2048];
    public_key_from_seed(kSeedA, pub);
    size_t len = build_package(kManifest, kWasm, sizeof(kWasm), kSeedA, package);
    package[len++] = 0x00;

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, len, pub, &out, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "trailing_data");
}

static void test_manifest_too_large(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[64];
    public_key_from_seed(kSeedA, pub);
    memcpy(package, EDGE_EAPP_MAGIC, EDGE_EAPP_MAGIC_LEN);
    key_id_from_seed(kSeedA, package + EDGE_EAPP_MAGIC_LEN);
    put_u32(package + EDGE_EAPP_MAGIC_LEN + EDGE_EAPP_KEY_ID_LEN, EDGE_EAPP_MAX_MANIFEST + 1);

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, sizeof(package), pub, &out, reason, sizeof(reason)),
                     -1);
    assert_string_equal(reason, "manifest_too_large");
}

static void test_wasm_too_large(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[64];
    public_key_from_seed(kSeedA, pub);
    memcpy(package, EDGE_EAPP_MAGIC, EDGE_EAPP_MAGIC_LEN);
    key_id_from_seed(kSeedA, package + EDGE_EAPP_MAGIC_LEN);
    size_t pos = EDGE_EAPP_MAGIC_LEN + EDGE_EAPP_KEY_ID_LEN;
    put_u32(package + pos, 2);
    pos += 4;
    memcpy(package + pos, "{}", 2);
    pos += 2;
    put_u32(package + pos, EDGE_EAPP_MAX_WASM + 1);

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, sizeof(package), pub, &out, reason, sizeof(reason)),
                     -1);
    assert_string_equal(reason, "wasm_too_large");
}

static void test_package_too_large(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t byte = 0;
    public_key_from_seed(kSeedA, pub);

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(&byte, EDGE_EAPP_MAX_PACKAGE + 1, pub, &out, reason, sizeof(reason)),
                     -1);
    assert_string_equal(reason, "package_too_large");
}

static void test_invalid_manifest_signed(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[2048];
    public_key_from_seed(kSeedA, pub);
    size_t len = build_package("{}", kWasm, sizeof(kWasm), kSeedA, package);

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, len, pub, &out, reason, sizeof(reason)), -1);
    assert_int_equal(strncmp(reason, "manifest: ", 10), 0);
}

static void test_denied_import_signed(void **state) {
    (void)state;
    uint8_t pub[32];
    uint8_t package[2048];
    uint8_t wasm[64];
    size_t wasm_len = build_denied_import_wasm(wasm);
    public_key_from_seed(kSeedA, pub);
    size_t len = build_package(kManifest, wasm, wasm_len, kSeedA, package);

    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(package, len, pub, &out, reason, sizeof(reason)), -1);
    assert_int_equal(strncmp(reason, "wasm: denied_import: env.system_execute",
                             strlen("wasm: denied_import: env.system_execute")),
                     0);
}

static void test_bad_args(void **state) {
    (void)state;
    uint8_t pub[32];
    edge_eapp_package_t out;
    char reason[EDGE_EAPP_REASON_SIZE];
    assert_int_equal(validate_with(NULL, 0, pub, &out, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "bad_args");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_valid_package),
        cmocka_unit_test(test_tampered_wasm),
        cmocka_unit_test(test_wrong_key),
        cmocka_unit_test(test_swapped_key_id_is_a_bad_signature),
        cmocka_unit_test(test_bad_magic),
        cmocka_unit_test(test_truncated_signature),
        cmocka_unit_test(test_bad_signature_len),
        cmocka_unit_test(test_trailing_data),
        cmocka_unit_test(test_manifest_too_large),
        cmocka_unit_test(test_wasm_too_large),
        cmocka_unit_test(test_package_too_large),
        cmocka_unit_test(test_invalid_manifest_signed),
        cmocka_unit_test(test_denied_import_signed),
        cmocka_unit_test(test_bad_args),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
