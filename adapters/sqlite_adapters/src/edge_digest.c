#include "edge_digest.h"

#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct edge_sha256_s {
    EVP_MD_CTX *ctx;
};

int edge_sha256_file(const char *path, char hex[65], unsigned long long *size_out) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    unsigned long long total = 0;
    char buf[512];
    size_t got;
    int ok = ctx != NULL && EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) == 1;
    while (ok && (got = fread(buf, 1, sizeof(buf), f)) > 0) {
        total += got;
        if (EVP_DigestUpdate(ctx, buf, got) != 1) ok = 0;
    }
    if (ok) ok = EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1;
    EVP_MD_CTX_free(ctx);
    fclose(f);
    if (!ok || digest_len != 32) return -1;
    for (unsigned int i = 0; i < digest_len; i++) snprintf(hex + i * 2, 3, "%02x", digest[i]);
    if (size_out) *size_out = total;
    return 0;
}

edge_sha256_t *edge_sha256_start(void) {
    edge_sha256_t *hash = malloc(sizeof(*hash));
    if (!hash) return NULL;
    hash->ctx = EVP_MD_CTX_new();
    if (!hash->ctx || EVP_DigestInit_ex(hash->ctx, EVP_sha256(), NULL) != 1) {
        EVP_MD_CTX_free(hash->ctx);
        free(hash);
        return NULL;
    }
    return hash;
}

int edge_sha256_update(edge_sha256_t *ctx, const void *data, size_t len) {
    if (!ctx || (!data && len > 0)) return -1;
    return EVP_DigestUpdate(ctx->ctx, data, len) == 1 ? 0 : -1;
}

int edge_sha256_finish_hex(edge_sha256_t *ctx, char hex[65]) {
    if (!ctx || !hex) return -1;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    if (EVP_DigestFinal_ex(ctx->ctx, digest, &digest_len) != 1 || digest_len != 32) return -1;
    for (unsigned int i = 0; i < digest_len; i++) snprintf(hex + i * 2, 3, "%02x", digest[i]);
    return 0;
}

void edge_sha256_free(edge_sha256_t *ctx) {
    if (!ctx) return;
    EVP_MD_CTX_free(ctx->ctx);
    free(ctx);
}
