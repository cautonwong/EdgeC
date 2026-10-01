/**
 * @file npu_enclave.h
 * @brief NPU Dynamic Model Encryption, Secure Enclave & Hardware Signature Verification (AEP-749 / Issue #279)
 *
 * Provides cryptographic model package decryption (AES-256-GCM / SM4-GCM),
 * hardware Root-of-Trust digital signature verification (Ed25519 / ECDSA / SM2),
 * anti-tampering integrity audit, memory zeroization on eviction, and DataHub telemetry sync.
 */

#ifndef EDGE_NPU_ENCLAVE_H
#define EDGE_NPU_ENCLAVE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#include "npu_hal.h"
#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants & Enums                                                          */
/* -------------------------------------------------------------------------- */

#define NPU_ENCLAVE_MAGIC           0x454E434D  /* 'ENCM' */
#define NPU_ENCLAVE_VERSION         1
#define NPU_ENCLAVE_MAX_KEYS        16
#define NPU_ENCLAVE_KEY_SIZE        32          /* 256 bits */
#define NPU_ENCLAVE_IV_SIZE         12          /* 96 bits GCM IV */
#define NPU_ENCLAVE_TAG_SIZE        16          /* 128 bits GCM Auth Tag */
#define NPU_ENCLAVE_SIGNATURE_SIZE  64          /* 512 bits Digital Signature */
#define NPU_ENCLAVE_SHA256_SIZE     32

typedef enum {
    NPU_CIPHER_AES_256_GCM        = 1,
    NPU_CIPHER_CHACHA20_POLY1305  = 2,
    NPU_CIPHER_SM4_GCM            = 3
} npu_cipher_suite_t;

typedef enum {
    NPU_SIG_ED25519               = 1,
    NPU_SIG_ECDSA_P256            = 2,
    NPU_SIG_SM2                   = 3
} npu_signature_algo_t;

typedef enum {
    NPU_ENCLAVE_AUTH_UNINITIALIZED = 0,
    NPU_ENCLAVE_AUTH_PASSED        = 1,
    NPU_ENCLAVE_AUTH_SIG_INVALID   = 2,
    NPU_ENCLAVE_AUTH_DECRYPT_FAIL  = 3,
    NPU_ENCLAVE_AUTH_TAMPER_DETECT = 4
} npu_enclave_auth_status_t;

/**
 * @brief Encrypted Model Header Package
 */
typedef struct {
    uint32_t magic;                                  /**< Must match NPU_ENCLAVE_MAGIC */
    uint16_t version;                                /**< Header version (1) */
    uint8_t cipher_suite;                            /**< npu_cipher_suite_t */
    uint8_t sig_algorithm;                           /**< npu_signature_algo_t */
    uint32_t model_id;                               /**< Unique Model ID */
    uint32_t plaintext_len;                          /**< Expected plaintext byte size */
    uint32_t ciphertext_len;                         /**< Ciphertext byte size */
    uint8_t iv[NPU_ENCLAVE_IV_SIZE];                 /**< GCM Nonce */
    uint8_t tag[NPU_ENCLAVE_TAG_SIZE];               /**< GCM Auth Tag */
    uint8_t sha256_digest[NPU_ENCLAVE_SHA256_SIZE];  /**< Plaintext SHA-256 */
    uint8_t signature[NPU_ENCLAVE_SIGNATURE_SIZE];   /**< Digital signature over header + ciphertext */
} __attribute__((packed)) npu_enc_model_header_t;

/**
 * @brief Enclave Configuration Descriptor
 */
typedef struct {
    bool enforce_signature_check;                    /**< Require valid hardware signature to decrypt */
    bool enforce_integrity_sha256;                   /**< Require SHA-256 hash match */
    bool zeroize_on_unload;                          /**< Perform volatile memory zeroing */
} npu_enclave_config_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define NPU_ENCLAVE_STORAGE_SIZE 8192

typedef struct {
    alignas(max_align_t) uint8_t raw[NPU_ENCLAVE_STORAGE_SIZE];
} npu_enclave_storage_t;

typedef struct npu_enclave npu_enclave_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief In-place initialize NPU Enclave security manager
 */
npu_enclave_t *npu_enclave_create_in(npu_enclave_storage_t *storage, const npu_enclave_config_t *config);

/**
 * @brief Provision hardware Root-of-Trust public key
 */
int npu_enclave_set_root_pubkey(npu_enclave_t *enclave, const uint8_t *pubkey, size_t pubkey_len, npu_signature_algo_t algo);

/**
 * @brief Provision model decryption symmetric key
 */
int npu_enclave_register_model_key(npu_enclave_t *enclave, uint32_t model_id, const uint8_t *key_bytes, size_t key_len);

/**
 * @brief Decrypt and verify encrypted model package in memory
 *
 * @param enclave Enclave context
 * @param enc_package_data Complete encrypted model package (header + ciphertext)
 * @param enc_package_size Total size of package
 * @param out_decrypted_buf Caller-allocated buffer for decrypted model
 * @param max_buf_size Size of out_decrypted_buf
 * @param out_decrypted_size Pointer to receive exact decrypted size
 * @return 0 on success, negative error code on failure
 */
int npu_enclave_decrypt_model(npu_enclave_t *enclave,
                              const void *enc_package_data, size_t enc_package_size,
                              void *out_decrypted_buf, size_t max_buf_size,
                              size_t *out_decrypted_size);

/**
 * @brief Decrypt, verify and directly load encrypted model into NPU HAL
 */
int npu_enclave_load_to_hal(npu_enclave_t *enclave, npu_device_t *dev,
                            const void *enc_package_data, size_t enc_package_size,
                            void *secure_scratch_buf, size_t scratch_size,
                            npu_model_handle_t *out_handle);

/**
 * @brief Securely unload model from NPU and wipe scratch memory
 */
int npu_enclave_secure_unload(npu_enclave_t *enclave, npu_device_t *dev,
                              npu_model_handle_t handle,
                              void *secure_scratch_buf, size_t scratch_size);

/**
 * @brief Get Enclave security diagnostics & audit metrics
 */
int npu_enclave_get_status(const npu_enclave_t *enclave,
                           npu_enclave_auth_status_t *out_status,
                           float *out_last_decrypt_us,
                           uint32_t *out_tamper_count);

/**
 * @brief Synchronize Enclave security state to DataHub
 */
int npu_enclave_sync_datahub(const npu_enclave_t *enclave, datahub_t *hub);

/**
 * @brief Helper utility: Encrypt and sign plaintext model package
 */
int npu_enclave_package_model(uint32_t model_id,
                              npu_cipher_suite_t cipher,
                              npu_signature_algo_t sig_algo,
                              const uint8_t *symmetric_key,
                              const uint8_t *private_key_sign,
                              const void *plaintext_model, size_t plaintext_len,
                              void *out_package_buf, size_t max_buf_size,
                              size_t *out_package_size);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_NPU_ENCLAVE_H */
