/**
 * test_wasm_signature.c — 验证 WASM 签名链的独立测试
 * 编译: gcc -o test_wasm_signature test_wasm_signature.c -lcrypto
 * 运行: ./test_wasm_signature ../algo/wasm/demo/demo.wasm ../keys/edgec_signing_public.pem
 */

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file_to_buffer(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(*size);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, *size, f) != *size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    return buf;
}

/**
 * 模拟 wasm_mgr.c 的 verify_wasm_signature 逻辑
 * 返回: 1 = valid, 0 = invalid, -1 = error
 */
static int verify_wasm_sig(const char *wasm_path, const char *pubkey_path, const uint8_t *wasm_buf,
                           size_t wasm_size) {
    /* 构建签名文件路径 */
    char sig_path[1024];
    int ret = snprintf(sig_path, sizeof(sig_path), "%s.sig", wasm_path);
    if (ret < 0 || (size_t)ret >= sizeof(sig_path)) return -1;

    /* 读取签名 */
    size_t sig_size;
    uint8_t *sig_buf = (uint8_t *)read_file_to_buffer(sig_path, &sig_size);
    if (!sig_buf) {
        fprintf(stderr, "FAIL: cannot read signature file %s\n", sig_path);
        return -1;
    }

    /* 读取公钥 */
    BIO *bio = BIO_new_file(pubkey_path, "rb");
    if (!bio) {
        free(sig_buf);
        fprintf(stderr, "FAIL: cannot read public key %s\n", pubkey_path);
        return -1;
    }
    EVP_PKEY *pkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (!pkey) {
        free(sig_buf);
        fprintf(stderr, "FAIL: cannot parse public key\n");
        return -1;
    }

    /* EVP_DigestVerify: PKCS1v15 SHA256 */
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    int ok = EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, pkey);
    if (ok == 1) ok = EVP_DigestVerifyUpdate(ctx, wasm_buf, wasm_size);
    if (ok == 1) ok = EVP_DigestVerifyFinal(ctx, sig_buf, sig_size);

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    free(sig_buf);
    return ok;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <wasm_path> <pubkey_path>\n", argv[0]);
        return 1;
    }

    size_t wasm_size;
    uint8_t *wasm_buf = (uint8_t *)read_file_to_buffer(argv[1], &wasm_size);
    if (!wasm_buf) {
        fprintf(stderr, "FAIL: cannot read wasm file %s\n", argv[1]);
        return 1;
    }

    int result = verify_wasm_sig(argv[1], argv[2], wasm_buf, wasm_size);
    free(wasm_buf);

    if (result == 1) {
        printf("PASS: Signature VALID for %s\n", argv[1]);
        return 0;
    } else if (result == 0) {
        printf("FAIL: Signature INVALID for %s\n", argv[1]);
        return 1;
    } else {
        printf("FAIL: Verification error (%d) for %s\n", result, argv[1]);
        return 1;
    }
}
