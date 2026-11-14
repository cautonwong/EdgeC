#include "edge_digest.h"

#include <openssl/evp.h>
#include <stdio.h>
#include <string.h>

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
