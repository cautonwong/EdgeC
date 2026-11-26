#include "edge_tar.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "edge_digest.h"

typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
} tar_header_t;

_Static_assert(sizeof(tar_header_t) == EDGE_TAR_BLOCK, "ustar header is one block");

typedef struct {
    FILE *f;
    char name[EDGE_TAR_NAME_SIZE];
    unsigned long long size;
    long data_offset;
} tar_entry_t;

static void octal(char *field, size_t size, unsigned long long value) {
    snprintf(field, size, "%0*llo", (int)(size - 1), value);
}

void edge_tar_header(void *block, const char *name, unsigned long long size) {
    tar_header_t *h = (tar_header_t *)block;
    memset(h, 0, sizeof(*h));
    snprintf(h->name, sizeof(h->name), "%s", name);
    octal(h->mode, sizeof(h->mode), 0644);
    octal(h->uid, sizeof(h->uid), 0);
    octal(h->gid, sizeof(h->gid), 0);
    octal(h->size, sizeof(h->size), size);
    octal(h->mtime, sizeof(h->mtime), (unsigned long long)time(NULL));
    h->typeflag = '0';
    memcpy(h->magic, "ustar", 6);
    memcpy(h->version, "00", 2);
    memcpy(h->uname, "root", 5);
    memcpy(h->gname, "root", 5);
    memset(h->checksum, ' ', sizeof(h->checksum));
    unsigned int sum = 0;
    const unsigned char *raw = (const unsigned char *)h;
    for (size_t i = 0; i < sizeof(*h); i++) sum += raw[i];
    snprintf(h->checksum, sizeof(h->checksum), "%06o", sum);
}

int edge_tar_write_entry(FILE *out, const char *name, const void *data, size_t size) {
    char header[EDGE_TAR_BLOCK];
    edge_tar_header(header, name, size);
    if (fwrite(header, 1, sizeof(header), out) != sizeof(header)) return -1;
    if (size && fwrite(data, 1, size, out) != size) return -1;
    size_t pad = (EDGE_TAR_BLOCK - size % EDGE_TAR_BLOCK) % EDGE_TAR_BLOCK;
    if (pad) {
        char zeros[EDGE_TAR_BLOCK];
        memset(zeros, 0, sizeof(zeros));
        if (fwrite(zeros, 1, pad, out) != pad) return -1;
    }
    return 0;
}

int edge_tar_finish(FILE *out) {
    char zeros[EDGE_TAR_BLOCK];
    memset(zeros, 0, sizeof(zeros));
    if (fwrite(zeros, 1, sizeof(zeros), out) != sizeof(zeros)) return -1;
    return fwrite(zeros, 1, sizeof(zeros), out) == sizeof(zeros) ? 0 : -1;
}

FILE *edge_tar_fopen_private(const char *path, const char *mode) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return NULL;
    if (fchmod(fd, 0600) != 0) {
        close(fd);
        return NULL;
    }
    FILE *f = fdopen(fd, mode);
    if (!f) close(fd);
    return f;
}

/* 0 on an entry, 1 on the first zero block (already consumed), -1 on a
 * malformed or truncated header. */
static int tar_next(tar_entry_t *entry) {
    tar_header_t header;
    if (fread(&header, 1, sizeof(header), entry->f) != sizeof(header)) return -1;
    int all_zero = 1;
    const unsigned char *raw = (const unsigned char *)&header;
    for (size_t i = 0; i < sizeof(header); i++) {
        if (raw[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    if (all_zero) return 1;

    char recorded[9];
    memcpy(recorded, header.checksum, 8);
    recorded[8] = '\0';
    memset(header.checksum, ' ', sizeof(header.checksum));
    unsigned int sum = 0;
    raw = (const unsigned char *)&header;
    for (size_t i = 0; i < sizeof(header); i++) sum += raw[i];
    if (strtoul(recorded, NULL, 8) != sum) return -1;

    /* Reject ustar typeflag/prefix forms this writer never emits. */
    if (header.typeflag != '0' && header.typeflag != '\0') return -1;
    if (header.prefix[0] != '\0' || memchr(header.name, '\0', sizeof(header.name)) == NULL) {
        return -1;
    }

    snprintf(entry->name, sizeof(entry->name), "%s", header.name);
    entry->size = strtoull(header.size, NULL, 8);
    long data_offset = ftell(entry->f);
    if (data_offset < 0) return -1;
    entry->data_offset = data_offset;
    unsigned long long pad = (EDGE_TAR_BLOCK - entry->size % EDGE_TAR_BLOCK) % EDGE_TAR_BLOCK;
    if (fseeko(entry->f, (off_t)(entry->size + pad), SEEK_CUR) != 0) return -1;
    return 0;
}

int edge_tar_list(const char *path, char names[][EDGE_TAR_NAME_SIZE], size_t cap, size_t *count) {
    if (!path || !names || !count) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    tar_entry_t entry = {.f = f};
    size_t found = 0;
    int rc;
    while ((rc = tar_next(&entry)) == 0) {
        if (found >= cap) {
            fclose(f);
            return -1;
        }
        snprintf(names[found], EDGE_TAR_NAME_SIZE, "%s", entry.name);
        found++;
    }
    if (rc != 1) {
        fclose(f);
        return -1;
    }
    char block[EDGE_TAR_BLOCK];
    if (fread(block, 1, sizeof(block), f) != sizeof(block)) {
        fclose(f);
        return -1;
    }
    for (size_t i = 0; i < sizeof(block); i++) {
        if (block[i] != 0) {
            fclose(f);
            return -1;
        }
    }
    if (fgetc(f) != EOF) {
        fclose(f);
        return -1;
    }
    if (fclose(f) != 0) return -1;
    *count = found;
    return 0;
}

int edge_tar_read_member(const char *path, const char *wanted, size_t max_size,
                         unsigned char **data, size_t *len, char sha[65]) {
    if (!path || !wanted || !data || !len) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    tar_entry_t entry = {.f = f};
    int rc;
    int ok = -1;
    while ((rc = tar_next(&entry)) == 0) {
        if (strcmp(entry.name, wanted) != 0) continue;
        if (entry.size > max_size) break;
        unsigned char *buf = malloc((size_t)entry.size + 1);
        if (!buf) break;
        if (fseek(f, entry.data_offset, SEEK_SET) != 0 ||
            fread(buf, 1, (size_t)entry.size, f) != (size_t)entry.size) {
            free(buf);
            break;
        }
        buf[entry.size] = '\0';
        edge_sha256_t *hasher = edge_sha256_start();
        int hashed = hasher != NULL && edge_sha256_update(hasher, buf, (size_t)entry.size) == 0 &&
                     edge_sha256_finish_hex(hasher, sha) == 0;
        edge_sha256_free(hasher);
        if (!hashed) {
            free(buf);
            break;
        }
        *data = buf;
        *len = (size_t)entry.size;
        ok = 0;
        break;
    }
    fclose(f);
    return ok;
}

int edge_tar_hash_member(const char *path, const char *wanted, char sha[65],
                         unsigned long long *size_out) {
    if (!path || !wanted) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    tar_entry_t entry = {.f = f};
    int rc;
    int ok = -1;
    while ((rc = tar_next(&entry)) == 0) {
        if (strcmp(entry.name, wanted) != 0) continue;
        if (fseek(f, entry.data_offset, SEEK_SET) != 0) break;
        edge_sha256_t *hasher = edge_sha256_start();
        char buf[512];
        unsigned long long left = entry.size;
        int good = hasher != NULL;
        while (good && left > 0) {
            size_t chunk = left > sizeof(buf) ? sizeof(buf) : (size_t)left;
            if (fread(buf, 1, chunk, f) != chunk || edge_sha256_update(hasher, buf, chunk) != 0) {
                good = 0;
                break;
            }
            left -= chunk;
        }
        if (good) good = edge_sha256_finish_hex(hasher, sha) == 0;
        edge_sha256_free(hasher);
        if (!good) break;
        if (size_out) *size_out = entry.size;
        ok = 0;
        break;
    }
    fclose(f);
    return ok;
}

int edge_tar_copy_member(const char *path, const char *wanted, const char *dest) {
    if (!path || !wanted || !dest) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    tar_entry_t entry = {.f = f};
    int rc;
    int done = -1;
    while ((rc = tar_next(&entry)) == 0) {
        if (strcmp(entry.name, wanted) != 0) continue;
        int fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd < 0) break;
        FILE *out = fdopen(fd, "wb");
        if (!out) {
            close(fd);
            break;
        }
        if (fseek(f, entry.data_offset, SEEK_SET) != 0) {
            fclose(out);
            break;
        }
        char buf[512];
        unsigned long long left = entry.size;
        while (left > 0) {
            size_t chunk = left > sizeof(buf) ? sizeof(buf) : (size_t)left;
            if (fread(buf, 1, chunk, f) != chunk || fwrite(buf, 1, chunk, out) != chunk) break;
            left -= chunk;
        }
        int ok = left == 0 && fflush(out) == 0 && fsync(fileno(out)) == 0;
        if (fclose(out) != 0) ok = 0;
        if (ok) done = 0;
        break;
    }
    fclose(f);
    return done;
}
