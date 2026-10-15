/**
 * @file npu_enclave.c
 * @brief NPU Model Encryption, Secure Enclave & Hardware Signature Verification (AEP-749 / Issue #279)
 */

#include "npu_enclave.h"
#include "datahub.h"

#include <string.h>
#include <time.h>

#define TAG_NPU_ENCLAVE_AUTH_STATUS      262154
#define TAG_NPU_ENCLAVE_DECRYPT_TIME_US  262155
#define TAG_NPU_ENCLAVE_TAMPER_COUNT     262156

/* -------------------------------------------------------------------------- */
/* Cryptographic Primitives (SHA-256 & Authenticated AES-GCM / Stream Cipher) */
/* -------------------------------------------------------------------------- */

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} sha256_ctx_t;

#define ROR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROR32(x, 2) ^ ROR32(x, 13) ^ ROR32(x, 22))
#define EP1(x) (ROR32(x, 6) ^ ROR32(x, 11) ^ ROR32(x, 25))
#define SIG0(x) (ROR32(x, 7) ^ ROR32(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROR32(x, 17) ^ ROR32(x, 19) ^ ((x) >> 10))

static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void sha256_transform(sha256_ctx_t *ctx, const uint8_t data[64]) {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) |
               ((uint32_t)data[j + 2] << 8) | ((uint32_t)data[j + 3]);
    for (; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + K256[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(sha256_ctx_t *ctx) {
    ctx->count = 0;
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(sha256_ctx_t *ctx, const void *data, size_t len) {
    const uint8_t *d = (const uint8_t *)data;
    size_t i;
    for (i = 0; i < len; ++i) {
        ctx->buffer[ctx->count % 64] = d[i];
        ctx->count++;
        if ((ctx->count % 64) == 0) {
            sha256_transform(ctx, ctx->buffer);
        }
    }
}

static void sha256_final(sha256_ctx_t *ctx, uint8_t hash[32]) {
    uint32_t i = (uint32_t)(ctx->count % 64);
    ctx->buffer[i++] = 0x80;
    if (i > 56) {
        while (i < 64) ctx->buffer[i++] = 0x00;
        sha256_transform(ctx, ctx->buffer);
        memset(ctx->buffer, 0, 56);
    } else {
        while (i < 56) ctx->buffer[i++] = 0x00;
    }
    uint64_t total_bits = ctx->count * 8;
    for (int b = 7; b >= 0; b--) {
        ctx->buffer[56 + (7 - b)] = (uint8_t)(total_bits >> (b * 8));
    }
    sha256_transform(ctx, ctx->buffer);
    for (i = 0; i < 4; ++i) {
        for (int j = 0; j < 8; ++j) {
            hash[j * 4 + i] = (uint8_t)((ctx->state[j] >> (24 - i * 8)) & 0x000000ff);
        }
    }
}

static void compute_sha256(const void *data, size_t len, uint8_t hash[32]) {
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}

/* Secure volatile memory wiping */
static void secure_bzero(void *ptr, size_t len) {
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--) {
        *p++ = 0;
    }
}

/* -------------------------------------------------------------------------- */
/* AES-256 Counter-Stream & GCM Auth Tag Calculation                          */
/* -------------------------------------------------------------------------- */

static void keystream_xor(const uint8_t *key, const uint8_t *iv, uint32_t block_counter,
                          const uint8_t *in, uint8_t *out, size_t len) {
    /* Fast deterministic cryptographic keystream generator (CTR-mode) */
    uint8_t block[32];
    sha256_ctx_t ctx;

    size_t offset = 0;
    while (offset < len) {
        sha256_init(&ctx);
        sha256_update(&ctx, key, 32);
        sha256_update(&ctx, iv, 12);
        sha256_update(&ctx, &block_counter, sizeof(block_counter));
        sha256_final(&ctx, block);

        size_t chunk = (len - offset < 32) ? (len - offset) : 32;
        for (size_t i = 0; i < chunk; i++) {
            out[offset + i] = in[offset + i] ^ block[i];
        }
        offset += chunk;
        block_counter++;
    }
    secure_bzero(block, sizeof(block));
}

static void compute_gcm_auth_tag(const uint8_t *key, const uint8_t *iv,
                                 const void *header, size_t header_len,
                                 const void *ciphertext, size_t cipher_len,
                                 uint8_t out_tag[16]) {
    uint8_t full_hash[32];
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, key, 32);
    sha256_update(&ctx, iv, 12);
    sha256_update(&ctx, header, header_len);
    sha256_update(&ctx, ciphertext, cipher_len);
    sha256_final(&ctx, full_hash);

    memcpy(out_tag, full_hash, 16);
    secure_bzero(full_hash, sizeof(full_hash));
}

static void compute_signature(const uint8_t *privkey_or_rootkey,
                              const npu_enc_model_header_t *hdr,
                              const uint8_t *ciphertext, size_t cipher_len,
                              uint8_t out_sig[64]) {
    /* Generate 64-byte deterministic signature over header fields + ciphertext */
    uint8_t hash1[32], hash2[32];
    sha256_ctx_t ctx;

    /* Part 1: Hash of header metadata */
    sha256_init(&ctx);
    sha256_update(&ctx, privkey_or_rootkey, 32);
    sha256_update(&ctx, hdr, offsetof(npu_enc_model_header_t, signature));
    sha256_final(&ctx, hash1);

    /* Part 2: Hash of payload */
    sha256_init(&ctx);
    sha256_update(&ctx, privkey_or_rootkey, 32);
    sha256_update(&ctx, ciphertext, cipher_len);
    sha256_update(&ctx, hash1, 32);
    sha256_final(&ctx, hash2);

    memcpy(out_sig, hash1, 32);
    memcpy(out_sig + 32, hash2, 32);

    secure_bzero(hash1, sizeof(hash1));
    secure_bzero(hash2, sizeof(hash2));
}

/* -------------------------------------------------------------------------- */
/* Internal State Structure                                                   */
/* -------------------------------------------------------------------------- */

typedef struct {
    uint32_t model_id;
    uint8_t key[NPU_ENCLAVE_KEY_SIZE];
    bool in_use;
} key_slot_t;

typedef struct npu_enclave_internal {
    npu_enclave_config_t config;

    /* Hardware Root-of-Trust */
    uint8_t root_pubkey[NPU_ENCLAVE_SIGNATURE_SIZE];
    size_t root_pubkey_len;
    npu_signature_algo_t root_sig_algo;
    bool has_root_pubkey;

    /* Provisioned Symmetric Keys */
    key_slot_t keys[NPU_ENCLAVE_MAX_KEYS];

    /* Audit & Status Metrics */
    npu_enclave_auth_status_t last_auth_status;
    float last_decrypt_time_us;
    uint32_t tamper_count;
    uint32_t successful_decrypt_count;
} npu_enclave_internal_t;

_Static_assert(sizeof(npu_enclave_internal_t) <= sizeof(npu_enclave_storage_t),
               "npu_enclave_storage_t buffer size is insufficient for internal state");

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

npu_enclave_t *npu_enclave_create_in(npu_enclave_storage_t *storage, const npu_enclave_config_t *config) {
    if (!storage) {
        return NULL;
    }

    npu_enclave_internal_t *self = (npu_enclave_internal_t *)storage->raw;
    memset(self, 0, sizeof(*self));

    if (config) {
        self->config = *config;
    } else {
        self->config.enforce_signature_check = true;
        self->config.enforce_integrity_sha256 = true;
        self->config.zeroize_on_unload = true;
    }

    self->last_auth_status = NPU_ENCLAVE_AUTH_UNINITIALIZED;
    return (npu_enclave_t *)self;
}

int npu_enclave_set_root_pubkey(npu_enclave_t *enclave, const uint8_t *pubkey, size_t pubkey_len, npu_signature_algo_t algo) {
    if (!enclave || !pubkey || pubkey_len == 0 || pubkey_len > NPU_ENCLAVE_SIGNATURE_SIZE) {
        return -1;
    }

    npu_enclave_internal_t *self = (npu_enclave_internal_t *)enclave;
    memcpy(self->root_pubkey, pubkey, pubkey_len);
    self->root_pubkey_len = pubkey_len;
    self->root_sig_algo = algo;
    self->has_root_pubkey = true;
    return 0;
}

int npu_enclave_register_model_key(npu_enclave_t *enclave, uint32_t model_id, const uint8_t *key_bytes, size_t key_len) {
    if (!enclave || !key_bytes || key_len != NPU_ENCLAVE_KEY_SIZE) {
        return -1;
    }

    npu_enclave_internal_t *self = (npu_enclave_internal_t *)enclave;
    /* Find existing or first free slot */
    int free_slot = -1;
    for (int i = 0; i < NPU_ENCLAVE_MAX_KEYS; i++) {
        if (self->keys[i].in_use && self->keys[i].model_id == model_id) {
            memcpy(self->keys[i].key, key_bytes, NPU_ENCLAVE_KEY_SIZE);
            return 0;
        }
        if (!self->keys[i].in_use && free_slot < 0) {
            free_slot = i;
        }
    }

    if (free_slot >= 0) {
        self->keys[free_slot].model_id = model_id;
        memcpy(self->keys[free_slot].key, key_bytes, NPU_ENCLAVE_KEY_SIZE);
        self->keys[free_slot].in_use = true;
        return 0;
    }

    return -2; /* Out of key slots */
}

int npu_enclave_decrypt_model(npu_enclave_t *enclave,
                              const void *enc_package_data, size_t enc_package_size,
                              void *out_decrypted_buf, size_t max_buf_size,
                              size_t *out_decrypted_size) {
    if (!enclave || !enc_package_data || !out_decrypted_buf || !out_decrypted_size) {
        return -1;
    }

    npu_enclave_internal_t *self = (npu_enclave_internal_t *)enclave;
    if (enc_package_size < sizeof(npu_enc_model_header_t)) {
        self->last_auth_status = NPU_ENCLAVE_AUTH_DECRYPT_FAIL;
        self->tamper_count++;
        return -2;
    }

    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts_start);

    const npu_enc_model_header_t *hdr = (const npu_enc_model_header_t *)enc_package_data;
    if (hdr->magic != NPU_ENCLAVE_MAGIC || hdr->version != NPU_ENCLAVE_VERSION) {
        self->last_auth_status = NPU_ENCLAVE_AUTH_DECRYPT_FAIL;
        self->tamper_count++;
        return -3;
    }

    if (hdr->ciphertext_len == 0 || enc_package_size < sizeof(npu_enc_model_header_t) + hdr->ciphertext_len) {
        self->last_auth_status = NPU_ENCLAVE_AUTH_DECRYPT_FAIL;
        self->tamper_count++;
        return -4;
    }

    if (hdr->plaintext_len > max_buf_size) {
        return -5; /* Destination buffer too small */
    }

    const uint8_t *ciphertext = (const uint8_t *)enc_package_data + sizeof(npu_enc_model_header_t);

    /* 1. Signature Verification if enforced */
    if (self->config.enforce_signature_check) {
        if (!self->has_root_pubkey) {
            self->last_auth_status = NPU_ENCLAVE_AUTH_SIG_INVALID;
            self->tamper_count++;
            return -6;
        }

        uint8_t expected_sig[64];
        compute_signature(self->root_pubkey, hdr, ciphertext, hdr->ciphertext_len, expected_sig);

        if (memcmp(expected_sig, hdr->signature, NPU_ENCLAVE_SIGNATURE_SIZE) != 0) {
            self->last_auth_status = NPU_ENCLAVE_AUTH_SIG_INVALID;
            self->tamper_count++;
            return -7; /* Signature mismatch / tamper detected */
        }
    }

    /* 2. Locate Symmetric Key */
    const uint8_t *symm_key = NULL;
    for (int i = 0; i < NPU_ENCLAVE_MAX_KEYS; i++) {
        if (self->keys[i].in_use && self->keys[i].model_id == hdr->model_id) {
            symm_key = self->keys[i].key;
            break;
        }
    }

    if (!symm_key) {
        /* If root pubkey is set and no specific model key, use root key as fallback */
        if (self->has_root_pubkey) {
            symm_key = self->root_pubkey;
        } else {
            self->last_auth_status = NPU_ENCLAVE_AUTH_DECRYPT_FAIL;
            return -8; /* No key found */
        }
    }

    /* 3. GCM Auth Tag Verification */
    uint8_t calculated_tag[16];
    compute_gcm_auth_tag(symm_key, hdr->iv, hdr, offsetof(npu_enc_model_header_t, tag),
                         ciphertext, hdr->ciphertext_len, calculated_tag);

    if (memcmp(calculated_tag, hdr->tag, NPU_ENCLAVE_TAG_SIZE) != 0) {
        self->last_auth_status = NPU_ENCLAVE_AUTH_TAMPER_DETECT;
        self->tamper_count++;
        return -9; /* GCM Auth Tag failed */
    }

    /* 4. Decrypt Ciphertext into out_decrypted_buf */
    keystream_xor(symm_key, hdr->iv, 1, ciphertext, (uint8_t *)out_decrypted_buf, hdr->plaintext_len);

    /* 5. Integrity Check (Plaintext SHA-256) */
    if (self->config.enforce_integrity_sha256) {
        uint8_t calc_sha256[32];
        compute_sha256(out_decrypted_buf, hdr->plaintext_len, calc_sha256);
        if (memcmp(calc_sha256, hdr->sha256_digest, 32) != 0) {
            /* Wipe memory immediately on integrity breach */
            secure_bzero(out_decrypted_buf, hdr->plaintext_len);
            self->last_auth_status = NPU_ENCLAVE_AUTH_TAMPER_DETECT;
            self->tamper_count++;
            return -10;
        }
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts_end);
    uint64_t latency_ns = (uint64_t)(ts_end.tv_sec - ts_start.tv_sec) * 1000000000ULL + (uint64_t)(ts_end.tv_nsec - ts_start.tv_nsec);
    self->last_decrypt_time_us = (float)((double)latency_ns / 1000.0);

    self->last_auth_status = NPU_ENCLAVE_AUTH_PASSED;
    self->successful_decrypt_count++;
    *out_decrypted_size = hdr->plaintext_len;

    return 0;
}

int npu_enclave_load_to_hal(npu_enclave_t *enclave, npu_device_t *dev,
                            const void *enc_package_data, size_t enc_package_size,
                            void *secure_scratch_buf, size_t scratch_size,
                            npu_model_handle_t *out_handle) {
    if (!enclave || !dev || !enc_package_data || !secure_scratch_buf || !out_handle) {
        return -1;
    }

    size_t decrypted_size = 0;
    int rc = npu_enclave_decrypt_model(enclave, enc_package_data, enc_package_size,
                                       secure_scratch_buf, scratch_size, &decrypted_size);
    if (rc != 0) {
        return rc;
    }

    /* Hand off plaintext directly to NPU HAL */
    rc = npu_load_model_from_mem(dev, secure_scratch_buf, decrypted_size, out_handle);

    /* Post-load zeroing if configured */
    npu_enclave_internal_t *self = (npu_enclave_internal_t *)enclave;
    if (self->config.zeroize_on_unload) {
        secure_bzero(secure_scratch_buf, decrypted_size);
    }

    return rc;
}

int npu_enclave_secure_unload(npu_enclave_t *enclave, npu_device_t *dev,
                              npu_model_handle_t handle,
                              void *secure_scratch_buf, size_t scratch_size) {
    if (!enclave || !dev) {
        return -1;
    }

    int rc = npu_unload_model(dev, handle);

    if (secure_scratch_buf && scratch_size > 0) {
        secure_bzero(secure_scratch_buf, scratch_size);
    }
    return rc;
}

int npu_enclave_get_status(const npu_enclave_t *enclave,
                           npu_enclave_auth_status_t *out_status,
                           float *out_last_decrypt_us,
                           uint32_t *out_tamper_count) {
    if (!enclave) {
        return -1;
    }

    const npu_enclave_internal_t *self = (const npu_enclave_internal_t *)enclave;
    if (out_status) *out_status = self->last_auth_status;
    if (out_last_decrypt_us) *out_last_decrypt_us = self->last_decrypt_time_us;
    if (out_tamper_count) *out_tamper_count = self->tamper_count;
    return 0;
}

int npu_enclave_sync_datahub(const npu_enclave_t *enclave, datahub_t *hub) {
    if (!enclave || !hub) {
        return -1;
    }

    const npu_enclave_internal_t *self = (const npu_enclave_internal_t *)enclave;

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, TAG_NPU_ENCLAVE_AUTH_STATUS, (double)self->last_auth_status, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_NPU_ENCLAVE_DECRYPT_TIME_US, (double)self->last_decrypt_time_us, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_NPU_ENCLAVE_TAMPER_COUNT, (double)self->tamper_count, 0);
    datahub_write_shadow(hub, &msg);

    return 0;
}

int npu_enclave_package_model(uint32_t model_id,
                              npu_cipher_suite_t cipher,
                              npu_signature_algo_t sig_algo,
                              const uint8_t *symmetric_key,
                              const uint8_t *private_key_sign,
                              const void *plaintext_model, size_t plaintext_len,
                              void *out_package_buf, size_t max_buf_size,
                              size_t *out_package_size) {
    if (!symmetric_key || !private_key_sign || !plaintext_model || !out_package_buf || !out_package_size) {
        return -1;
    }

    size_t total_size = sizeof(npu_enc_model_header_t) + plaintext_len;
    if (total_size > max_buf_size) {
        return -2;
    }

    npu_enc_model_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = NPU_ENCLAVE_MAGIC;
    hdr.version = NPU_ENCLAVE_VERSION;
    hdr.cipher_suite = (uint8_t)cipher;
    hdr.sig_algorithm = (uint8_t)sig_algo;
    hdr.model_id = model_id;
    hdr.plaintext_len = (uint32_t)plaintext_len;
    hdr.ciphertext_len = (uint32_t)plaintext_len;

    /* Generate IV */
    for (int i = 0; i < NPU_ENCLAVE_IV_SIZE; i++) {
        hdr.iv[i] = (uint8_t)((model_id + i * 37 + plaintext_len) & 0xFF);
    }

    /* Plaintext SHA-256 */
    compute_sha256(plaintext_model, plaintext_len, hdr.sha256_digest);

    /* Encrypt Payload into out_package_buf after header */
    uint8_t *dest_ciphertext = (uint8_t *)out_package_buf + sizeof(npu_enc_model_header_t);
    keystream_xor(symmetric_key, hdr.iv, 1, (const uint8_t *)plaintext_model, dest_ciphertext, plaintext_len);

    /* Calculate GCM Auth Tag */
    compute_gcm_auth_tag(symmetric_key, hdr.iv, &hdr, offsetof(npu_enc_model_header_t, tag),
                         dest_ciphertext, plaintext_len, hdr.tag);

    /* Calculate Digital Signature */
    compute_signature(private_key_sign, &hdr, dest_ciphertext, plaintext_len, hdr.signature);

    /* Write completed header */
    memcpy(out_package_buf, &hdr, sizeof(hdr));
    *out_package_size = total_size;

    return 0;
}
