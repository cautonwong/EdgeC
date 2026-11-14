/**
 * @file edge_eapp.h
 * @brief `.eapp` package container: bounds, Ed25519 signature, manifest schema
 *        and wasm import whitelist.
 *
 * Wire layout, little endian:
 *
 *   "EDGEAPP1\n"      9 bytes
 *   key_id            32 bytes, SHA-256 of the signer public key
 *   manifest_len      u32
 *   manifest JSON     manifest_len bytes, UTF-8, no NUL
 *   wasm_len          u32
 *   wasm module       wasm_len bytes
 *   signature_len     u32, must be 64
 *   signature         64 bytes, Ed25519 over every preceding byte
 *
 * The signature covers the header, both lengths and both payloads, so a
 * manifest or wasm swap always requires re-signing. Verification uses pure
 * Ed25519 (OpenSSL EVP_PKEY_ED25519); callers never pre-hash.
 */
#ifndef EDGE_EAPP_H
#define EDGE_EAPP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_EAPP_MAGIC "EDGEAPP1\n"
#define EDGE_EAPP_MAGIC_LEN 9
#define EDGE_EAPP_SIGNATURE_LEN 64
#define EDGE_EAPP_PUBLIC_KEY_LEN 32
#define EDGE_EAPP_KEY_ID_LEN 32
#define EDGE_EAPP_PUBLISHER_SIZE 64
#define EDGE_EAPP_KEYRING_MAX 8
#define EDGE_EAPP_MAX_MANIFEST (64u * 1024u)
#define EDGE_EAPP_MAX_WASM (16u * 1024u * 1024u)
#define EDGE_EAPP_MAX_PACKAGE (32u * 1024u * 1024u)
#define EDGE_EAPP_REASON_SIZE 200

/** One publisher key in the local keyring. */
typedef struct {
    char publisher_id[EDGE_EAPP_PUBLISHER_SIZE];
    uint8_t public_key[EDGE_EAPP_PUBLIC_KEY_LEN];
    uint8_t trust_level; /**< 0 untrusted .. 3 first-party */
    bool verified;
} edge_eapp_key_t;

/** Verified package contents. Owned by the caller; free with edge_eapp_package_free. */
typedef struct {
    char *manifest_json; /**< NUL-terminated copy of the manifest */
    size_t manifest_len;
    uint8_t *wasm; /**< copy of the wasm module */
    size_t wasm_len;
    char publisher_id[EDGE_EAPP_PUBLISHER_SIZE]; /**< matched keyring entry */
    uint8_t signer_key[EDGE_EAPP_PUBLIC_KEY_LEN];
    uint8_t trust_level;
    bool verified;
} edge_eapp_package_t;

/**
 * Verify and unpack a `.eapp` package.
 *
 * Checks, in order: magic, key id against the keyring, declared lengths, exact
 * package length, Ed25519 signature over the authenticated prefix, manifest
 * schema and wasm imports. Nothing is parsed before the signature passes.
 *
 * @param data        package bytes
 * @param len         package length
 * @param keys        publisher keyring
 * @param key_count   number of keyring entries, 1..EDGE_EAPP_KEYRING_MAX
 * @param out         receives the verified copies and the matched publisher
 * @param reason      failure reason: bad_args, package_too_large, truncated,
 *                    bad_magic, unknown_key, manifest_too_large, wasm_too_large,
 *                    bad_signature_len, trailing_data, bad_signature,
 *                    out_of_memory, "manifest: <detail>", "wasm: <detail>"
 * @param reason_size capacity of reason
 * @return 0 on success, -1 on failure with out zeroed
 */
int edge_eapp_validate(const void *data, size_t len, const edge_eapp_key_t *keys, size_t key_count,
                       edge_eapp_package_t *out, char *reason, size_t reason_size);

/** Release the copies held by a validated package. Safe on a zeroed struct. */
void edge_eapp_package_free(edge_eapp_package_t *pkg);

/**
 * Build and sign a `.eapp` package.
 *
 * The manifest schema and wasm imports are validated before anything is
 * signed, so an invalid package is never emitted. The signature covers every
 * byte up to the signature itself, including the signature-length field.
 *
 * @param manifest_json NUL-terminated manifest JSON
 * @param wasm          wasm module bytes, may be NULL only when wasm_len is 0
 * @param private_key   32-byte Ed25519 seed
 * @param out           receives a malloc'd package; caller frees with free()
 * @param out_len       receives the package length
 * @param reason        failure reason, as in edge_eapp_validate plus sign_failed
 * @return 0 on success, -1 on failure with *out NULL and *out_len 0
 */
int edge_eapp_build(const char *manifest_json, const void *wasm, size_t wasm_len,
                    const uint8_t private_key[EDGE_EAPP_PUBLIC_KEY_LEN], void **out,
                    size_t *out_len, char *reason, size_t reason_size);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_EAPP_H */
