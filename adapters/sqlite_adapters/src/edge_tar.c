#include "edge_tar.h"

#include <string.h>
#include <time.h>

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
