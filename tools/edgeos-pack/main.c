/**
 * @file main.c
 * @brief edgeos-pack: keygen, build and verify signed `.eapp` packages.
 *
 * Host-side tool. The device never packs; it only verifies and installs.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "edgeos/app_manifest.h"
#include "edgeos/edge_eapp.h"

#define PACK_MAX_INPUT (64u * 1024u * 1024u)

static int usage(void) {
    fprintf(stderr,
            "usage:\n"
            "  edgeos-pack keygen --out-dir DIR\n"
            "  edgeos-pack build --manifest FILE --wasm FILE --seed FILE --out FILE\n"
            "  edgeos-pack verify --package FILE --pubkey FILE\n");
    return 1;
}

static int read_file(const char *path, uint8_t **data, size_t *len) {
    *data = NULL;
    *len = 0;
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "edgeos-pack: cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long size = ftell(f);
    if (size < 0 || (unsigned long)size > PACK_MAX_INPUT) {
        fprintf(stderr, "edgeos-pack: %s is too large\n", path);
        fclose(f);
        return -1;
    }
    rewind(f);
    uint8_t *buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(f);
        return -1;
    }
    size_t got = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    if (got != (size_t)size) {
        fprintf(stderr, "edgeos-pack: short read on %s\n", path);
        free(buffer);
        return -1;
    }
    buffer[size] = '\0';
    *data = buffer;
    *len = (size_t)size;
    return 0;
}

static int write_atomic(const char *path, const void *data, size_t len, mode_t mode) {
    char tmp[PATH_MAX];
    int n = snprintf(tmp, sizeof(tmp), "%s.tmp.%ld", path, (long)getpid());
    if (n < 0 || (size_t)n >= sizeof(tmp)) return -1;

    int fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, mode);
    if (fd < 0) {
        fprintf(stderr, "edgeos-pack: cannot create %s: %s\n", tmp, strerror(errno));
        return -1;
    }
    const uint8_t *bytes = (const uint8_t *)data;
    size_t written = 0;
    while (written < len) {
        ssize_t chunk = write(fd, bytes + written, len - written);
        if (chunk < 0) {
            if (errno == EINTR) continue;
            fprintf(stderr, "edgeos-pack: write %s failed: %s\n", tmp, strerror(errno));
            close(fd);
            unlink(tmp);
            return -1;
        }
        written += (size_t)chunk;
    }
    if (fsync(fd) != 0) {
        close(fd);
        unlink(tmp);
        return -1;
    }
    close(fd);
    if (rename(tmp, path) != 0) {
        fprintf(stderr, "edgeos-pack: rename to %s failed: %s\n", path, strerror(errno));
        unlink(tmp);
        return -1;
    }
    return 0;
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Key files hold 32 raw bytes or 64 hex chars with optional trailing space. */
static int load_key(const char *path, uint8_t out[32]) {
    uint8_t *data = NULL;
    size_t len = 0;
    if (read_file(path, &data, &len) != 0) return -1;

    if (len == 32) {
        memcpy(out, data, 32);
        free(data);
        return 0;
    }
    while (len > 0 && (data[len - 1] == '\n' || data[len - 1] == '\r' || data[len - 1] == ' ')) {
        len--;
    }
    if (len != 64) {
        fprintf(stderr, "edgeos-pack: %s is not a 32-byte or 64-hex-char key\n", path);
        free(data);
        return -1;
    }
    for (size_t i = 0; i < 32; i++) {
        int hi = hex_value((char)data[i * 2]);
        int lo = hex_value((char)data[i * 2 + 1]);
        if (hi < 0 || lo < 0) {
            fprintf(stderr, "edgeos-pack: %s has a non-hex character\n", path);
            free(data);
            return -1;
        }
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    free(data);
    return 0;
}

static int derive_public(const uint8_t seed[32], uint8_t pub[32]) {
    EVP_PKEY *key = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, seed, 32);
    if (!key) return -1;
    size_t len = 32;
    int ok = EVP_PKEY_get_raw_public_key(key, pub, &len) == 1 && len == 32;
    EVP_PKEY_free(key);
    return ok ? 0 : -1;
}

static void hex_line(const uint8_t key[32], char out[65]) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < 32; i++) {
        out[i * 2] = digits[key[i] >> 4];
        out[i * 2 + 1] = digits[key[i] & 0x0f];
    }
    out[64] = '\n';
}

static int parse_args(int argc, char **argv, const char *const *names, const char **values,
                      int count) {
    for (int i = 2; i < argc; i += 2) {
        if (i + 1 >= argc) return -1;
        int found = -1;
        for (int n = 0; n < count; n++) {
            if (strcmp(argv[i], names[n]) == 0) found = n;
        }
        if (found < 0 || values[found]) return -1;
        values[found] = argv[i + 1];
    }
    for (int n = 0; n < count; n++) {
        if (!values[n]) return -1;
    }
    return 0;
}

static int cmd_keygen(int argc, char **argv) {
    const char *names[] = {"--out-dir"};
    const char *values[1] = {0};
    if (parse_args(argc, argv, names, values, 1) != 0) return usage();

    if (mkdir(values[0], 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "edgeos-pack: cannot create %s: %s\n", values[0], strerror(errno));
        return 1;
    }

    char seed_path[PATH_MAX];
    char pub_path[PATH_MAX];
    int n = snprintf(seed_path, sizeof(seed_path), "%s/edgec-eapp.seed", values[0]);
    int m = snprintf(pub_path, sizeof(pub_path), "%s/edgec-eapp.pub", values[0]);
    if (n < 0 || m < 0 || (size_t)n >= sizeof(seed_path) || (size_t)m >= sizeof(pub_path)) {
        return usage();
    }
    if (access(seed_path, F_OK) == 0 || access(pub_path, F_OK) == 0) {
        fprintf(stderr, "edgeos-pack: %s already exists, refusing to overwrite\n", values[0]);
        return 1;
    }

    uint8_t seed[32];
    uint8_t pub[32];
    if (RAND_bytes(seed, sizeof(seed)) != 1 || derive_public(seed, pub) != 0) {
        fprintf(stderr, "edgeos-pack: key generation failed\n");
        OPENSSL_cleanse(seed, sizeof(seed));
        return 1;
    }
    char seed_line[65];
    char pub_line[65];
    hex_line(seed, seed_line);
    hex_line(pub, pub_line);
    OPENSSL_cleanse(seed, sizeof(seed));

    if (write_atomic(seed_path, seed_line, sizeof(seed_line), 0600) != 0) return 1;
    if (write_atomic(pub_path, pub_line, sizeof(pub_line), 0644) != 0) {
        unlink(seed_path);
        return 1;
    }
    printf("seed %s\npublic %s\n", seed_path, pub_path);
    return 0;
}

static int cmd_build(int argc, char **argv) {
    const char *names[] = {"--manifest", "--wasm", "--seed", "--out"};
    const char *values[4] = {0};
    if (parse_args(argc, argv, names, values, 4) != 0) return usage();

    uint8_t *manifest = NULL;
    size_t manifest_len = 0;
    uint8_t *wasm = NULL;
    size_t wasm_len = 0;
    uint8_t seed[32];
    if (read_file(values[0], &manifest, &manifest_len) != 0) return 1;
    if (read_file(values[1], &wasm, &wasm_len) != 0) {
        free(manifest);
        return 1;
    }
    if (load_key(values[2], seed) != 0) {
        free(manifest);
        free(wasm);
        return 1;
    }

    void *package = NULL;
    size_t package_len = 0;
    char reason[EDGE_EAPP_REASON_SIZE];
    int rc = edge_eapp_build((const char *)manifest, wasm, wasm_len, seed, &package, &package_len,
                             reason, sizeof(reason));
    OPENSSL_cleanse(seed, sizeof(seed));
    free(manifest);
    free(wasm);
    if (rc != 0) {
        fprintf(stderr, "edgeos-pack: rejected: %s\n", reason);
        return 2;
    }

    if (write_atomic(values[3], package, package_len, 0644) != 0) {
        free(package);
        return 1;
    }
    printf("wrote %s (%zu bytes)\n", values[3], package_len);
    free(package);
    return 0;
}

static int cmd_verify(int argc, char **argv) {
    const char *names[] = {"--package", "--pubkey"};
    const char *values[2] = {0};
    if (parse_args(argc, argv, names, values, 2) != 0) return usage();

    uint8_t *package = NULL;
    size_t package_len = 0;
    uint8_t public_key[32];
    if (read_file(values[0], &package, &package_len) != 0) return 1;
    if (load_key(values[1], public_key) != 0) {
        free(package);
        return 1;
    }

    edge_eapp_key_t ring;
    memset(&ring, 0, sizeof(ring));
    snprintf(ring.publisher_id, sizeof(ring.publisher_id), "verify");
    memcpy(ring.public_key, public_key, EDGE_EAPP_PUBLIC_KEY_LEN);
    edge_eapp_package_t verified;
    char reason[EDGE_EAPP_REASON_SIZE];
    int rc = edge_eapp_validate(package, package_len, &ring, 1, &verified, reason, sizeof(reason));
    free(package);
    if (rc != 0) {
        fprintf(stderr, "edgeos-pack: invalid: %s\n", reason);
        return 2;
    }

    app_spec_t spec;
    if (edge_app_manifest_parse(verified.manifest_json, &spec, reason, sizeof(reason)) == 0) {
        printf("ok id=%s version=%s wasm=%zu bytes\n", spec.id, spec.version, verified.wasm_len);
    } else {
        printf("ok wasm=%zu bytes\n", verified.wasm_len);
    }
    edge_eapp_package_free(&verified);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) return usage();
    if (strcmp(argv[1], "keygen") == 0) return cmd_keygen(argc, argv);
    if (strcmp(argv[1], "build") == 0) return cmd_build(argc, argv);
    if (strcmp(argv[1], "verify") == 0) return cmd_verify(argc, argv);
    return usage();
}
