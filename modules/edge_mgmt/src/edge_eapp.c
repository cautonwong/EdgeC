/**
 * @file edge_eapp.c
 * @brief `.eapp` container parser, bounds checks and Ed25519 verification.
 */

#include "edgeos/edge_eapp.h"

#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_wasm_imports.h"
#include "edgeos/app_manifest.h"

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int key_id_from_public(const uint8_t public_key[EDGE_EAPP_PUBLIC_KEY_LEN],
                              uint8_t key_id[EDGE_EAPP_KEY_ID_LEN]) {
    unsigned int len = 0;
    return EVP_Digest(public_key, EDGE_EAPP_PUBLIC_KEY_LEN, key_id, &len, EVP_sha256(), NULL) == 1 &&
                   len == EDGE_EAPP_KEY_ID_LEN
               ? 0
               : -1;
}

static int fail(char *reason, size_t reason_size, const char *text) {
    if (reason && reason_size) snprintf(reason, reason_size, "%s", text);
    return -1;
}

static int verify_ed25519(const uint8_t public_key[EDGE_EAPP_PUBLIC_KEY_LEN], const uint8_t *message,
                          size_t message_len, const uint8_t signature[EDGE_EAPP_SIGNATURE_LEN]) {
    EVP_PKEY *key =
        EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, public_key, EDGE_EAPP_PUBLIC_KEY_LEN);
    if (!key) return -1;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    int ok = 0;
    if (ctx && EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, key) == 1 &&
        EVP_DigestVerify(ctx, signature, EDGE_EAPP_SIGNATURE_LEN, message, message_len) == 1) {
        ok = 1;
    }
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(key);
    return ok ? 0 : -1;
}

int edge_eapp_validate(const void *data, size_t len, const edge_eapp_key_t *keys,
                       size_t key_count, edge_eapp_package_t *out, char *reason,
                       size_t reason_size) {
    if (!data || !keys || key_count == 0 || key_count > EDGE_EAPP_KEYRING_MAX || !out) {
        return fail(reason, reason_size, "bad_args");
    }
    memset(out, 0, sizeof(*out));

    const uint8_t *bytes = (const uint8_t *)data;
    if (len > EDGE_EAPP_MAX_PACKAGE) return fail(reason, reason_size, "package_too_large");
    if (len < EDGE_EAPP_MAGIC_LEN + EDGE_EAPP_KEY_ID_LEN + 4) {
        return fail(reason, reason_size, "truncated");
    }
    if (memcmp(bytes, EDGE_EAPP_MAGIC, EDGE_EAPP_MAGIC_LEN) != 0) {
        return fail(reason, reason_size, "bad_magic");
    }

    const uint8_t *key_id = bytes + EDGE_EAPP_MAGIC_LEN;
    const edge_eapp_key_t *signer = NULL;
    uint8_t candidate[EDGE_EAPP_KEY_ID_LEN];
    for (size_t i = 0; i < key_count; i++) {
        if (key_id_from_public(keys[i].public_key, candidate) == 0 &&
            memcmp(candidate, key_id, EDGE_EAPP_KEY_ID_LEN) == 0) {
            signer = &keys[i];
            break;
        }
    }
    if (!signer) return fail(reason, reason_size, "unknown_key");

    size_t pos = EDGE_EAPP_MAGIC_LEN + EDGE_EAPP_KEY_ID_LEN;
    uint32_t manifest_len = read_u32(bytes + pos);
    pos += 4;
    if (manifest_len > EDGE_EAPP_MAX_MANIFEST) {
        return fail(reason, reason_size, "manifest_too_large");
    }
    if (len - pos < (size_t)manifest_len + 4) return fail(reason, reason_size, "truncated");
    const uint8_t *manifest = bytes + pos;
    pos += manifest_len;

    uint32_t wasm_len = read_u32(bytes + pos);
    pos += 4;
    if (wasm_len > EDGE_EAPP_MAX_WASM) return fail(reason, reason_size, "wasm_too_large");
    if (len - pos < (size_t)wasm_len + 4) return fail(reason, reason_size, "truncated");
    const uint8_t *wasm = bytes + pos;
    pos += wasm_len;

    uint32_t signature_len = read_u32(bytes + pos);
    pos += 4;
    if (signature_len != EDGE_EAPP_SIGNATURE_LEN) {
        return fail(reason, reason_size, "bad_signature_len");
    }
    if (len - pos < signature_len) return fail(reason, reason_size, "truncated");
    if (len - pos != signature_len) return fail(reason, reason_size, "trailing_data");

    if (verify_ed25519(signer->public_key, bytes, pos, bytes + pos) != 0) {
        return fail(reason, reason_size, "bad_signature");
    }

    out->manifest_json = malloc((size_t)manifest_len + 1);
    if (!out->manifest_json) return fail(reason, reason_size, "out_of_memory");
    memcpy(out->manifest_json, manifest, manifest_len);
    out->manifest_json[manifest_len] = '\0';
    out->manifest_len = manifest_len;

    char detail[EDGE_EAPP_REASON_SIZE];
    if (edge_app_manifest_validate_json(out->manifest_json, detail, sizeof(detail)) != 0) {
        if (reason && reason_size) snprintf(reason, reason_size, "manifest: %s", detail);
        edge_eapp_package_free(out);
        return -1;
    }

    if (edge_wasm_imports_validate(wasm, wasm_len, detail, sizeof(detail)) != 0) {
        if (reason && reason_size) snprintf(reason, reason_size, "wasm: %s", detail);
        edge_eapp_package_free(out);
        return -1;
    }

    out->wasm = malloc(wasm_len ? wasm_len : 1);
    if (!out->wasm) {
        edge_eapp_package_free(out);
        return fail(reason, reason_size, "out_of_memory");
    }
    if (wasm_len) memcpy(out->wasm, wasm, wasm_len);
    out->wasm_len = wasm_len;
    snprintf(out->publisher_id, sizeof(out->publisher_id), "%s", signer->publisher_id);
    memcpy(out->signer_key, signer->public_key, EDGE_EAPP_PUBLIC_KEY_LEN);
    out->trust_level = signer->trust_level;
    out->verified = signer->verified;
    return 0;
}

void edge_eapp_package_free(edge_eapp_package_t *pkg) {
    if (!pkg) return;
    free(pkg->manifest_json);
    free(pkg->wasm);
    memset(pkg, 0, sizeof(*pkg));
}

static void write_u32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8) & 0xffu);
    p[2] = (uint8_t)((value >> 16) & 0xffu);
    p[3] = (uint8_t)((value >> 24) & 0xffu);
}

static int sign_ed25519(const uint8_t private_key[EDGE_EAPP_PUBLIC_KEY_LEN], const uint8_t *message,
                        size_t message_len, uint8_t signature[EDGE_EAPP_SIGNATURE_LEN]) {
    EVP_PKEY *key =
        EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, private_key, EDGE_EAPP_PUBLIC_KEY_LEN);
    if (!key) return -1;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    size_t signature_len = EDGE_EAPP_SIGNATURE_LEN;
    int ok = 0;
    if (ctx && EVP_DigestSignInit(ctx, NULL, NULL, NULL, key) == 1 &&
        EVP_DigestSign(ctx, signature, &signature_len, message, message_len) == 1 &&
        signature_len == EDGE_EAPP_SIGNATURE_LEN) {
        ok = 1;
    }
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(key);
    return ok ? 0 : -1;
}

int edge_eapp_build(const char *manifest_json, const void *wasm, size_t wasm_len,
                    const uint8_t private_key[EDGE_EAPP_PUBLIC_KEY_LEN], void **out,
                    size_t *out_len, char *reason, size_t reason_size) {
    if (!manifest_json || !private_key || !out || !out_len || (!wasm && wasm_len)) {
        return fail(reason, reason_size, "bad_args");
    }
    *out = NULL;
    *out_len = 0;

    size_t manifest_len = strlen(manifest_json);
    if (manifest_len > EDGE_EAPP_MAX_MANIFEST) {
        return fail(reason, reason_size, "manifest_too_large");
    }
    if (wasm_len > EDGE_EAPP_MAX_WASM) return fail(reason, reason_size, "wasm_too_large");

    size_t total = EDGE_EAPP_MAGIC_LEN + EDGE_EAPP_KEY_ID_LEN + 4 + manifest_len + 4 + wasm_len +
                   4 + EDGE_EAPP_SIGNATURE_LEN;
    if (total > EDGE_EAPP_MAX_PACKAGE) return fail(reason, reason_size, "package_too_large");

    char detail[EDGE_EAPP_REASON_SIZE];
    if (edge_app_manifest_validate_json(manifest_json, detail, sizeof(detail)) != 0) {
        if (reason && reason_size) snprintf(reason, reason_size, "manifest: %s", detail);
        return -1;
    }
    if (edge_wasm_imports_validate(wasm, wasm_len, detail, sizeof(detail)) != 0) {
        if (reason && reason_size) snprintf(reason, reason_size, "wasm: %s", detail);
        return -1;
    }

    uint8_t *package = malloc(total);
    if (!package) return fail(reason, reason_size, "out_of_memory");

    EVP_PKEY *key = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, private_key,
                                                 EDGE_EAPP_PUBLIC_KEY_LEN);
    uint8_t public_key[EDGE_EAPP_PUBLIC_KEY_LEN];
    size_t public_len = sizeof(public_key);
    int derived = key && EVP_PKEY_get_raw_public_key(key, public_key, &public_len) == 1 &&
                  public_len == sizeof(public_key);
    EVP_PKEY_free(key);
    uint8_t key_id[EDGE_EAPP_KEY_ID_LEN];
    if (!derived || key_id_from_public(public_key, key_id) != 0) {
        free(package);
        return fail(reason, reason_size, "bad_args");
    }

    size_t pos = 0;
    memcpy(package, EDGE_EAPP_MAGIC, EDGE_EAPP_MAGIC_LEN);
    pos += EDGE_EAPP_MAGIC_LEN;
    memcpy(package + pos, key_id, EDGE_EAPP_KEY_ID_LEN);
    pos += EDGE_EAPP_KEY_ID_LEN;
    write_u32(package + pos, (uint32_t)manifest_len);
    pos += 4;
    memcpy(package + pos, manifest_json, manifest_len);
    pos += manifest_len;
    write_u32(package + pos, (uint32_t)wasm_len);
    pos += 4;
    if (wasm_len) memcpy(package + pos, wasm, wasm_len);
    pos += wasm_len;
    write_u32(package + pos, EDGE_EAPP_SIGNATURE_LEN);
    pos += 4;

    if (sign_ed25519(private_key, package, pos, package + pos) != 0) {
        free(package);
        return fail(reason, reason_size, "sign_failed");
    }

    *out = package;
    *out_len = total;
    return 0;
}
