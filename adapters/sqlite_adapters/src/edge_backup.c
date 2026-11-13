/**
 * @file edge_backup.c
 * @brief SQLite snapshot + ustar bundle, manifest and restore.
 */

#include "edgeos/edge_backup.h"

#include <errno.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <cjson/cJSON.h>

#include "sqlite_service.h"
#include "edge_tar.h"
#include "edge_digest.h"

#define TAR_BLOCK 512u
#define MANIFEST_MAX (64u * 1024u)

static int fail(char *reason, size_t size, const char *text) {
    if (reason && size) snprintf(reason, size, "%s", text);
    return -1;
}

static int write_padding(FILE *out, size_t size) {
    size_t pad = (TAR_BLOCK - size % TAR_BLOCK) % TAR_BLOCK;
    if (pad) {
        unsigned char zeros[TAR_BLOCK];
        memset(zeros, 0, sizeof(zeros));
        if (fwrite(zeros, 1, pad, out) != pad) return -1;
    }
    return 0;
}

static int tar_write_file(FILE *out, const char *name, const char *src_path) {
    FILE *src = fopen(src_path, "rb");
    if (!src) return -1;
    if (fseeko(src, 0, SEEK_END) != 0) {
        fclose(src);
        return -1;
    }
    off_t size = ftello(src);
    if (size < 0 || fseeko(src, 0, SEEK_SET) != 0) {
        fclose(src);
        return -1;
    }
    char header[EDGE_TAR_BLOCK];
    edge_tar_header(header, name, (size_t)size);
    if (fwrite(header, 1, sizeof(header), out) != sizeof(header)) {
        fclose(src);
        return -1;
    }
    char buf[512];
    size_t got;
    while ((got = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, got, out) != got) {
            fclose(src);
            return -1;
        }
    }
    fclose(src);
    return write_padding(out, (size_t)size);
}

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
} tar_read_header_t;

typedef struct {
    FILE *f;
    char name[101];
    unsigned long long size;
    long data_offset;
} tar_entry_t;

static int tar_next(tar_entry_t *entry) {
    tar_read_header_t header;
    if (fread(&header, 1, sizeof(header), entry->f) != sizeof(header)) return 1;
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

    snprintf(entry->name, sizeof(entry->name), "%s", header.name);
    entry->size = strtoull(header.size, NULL, 8);
    entry->data_offset = ftell(entry->f);
    if (fseeko(entry->f, (off_t)(entry->size + ((TAR_BLOCK - entry->size % TAR_BLOCK) % TAR_BLOCK)),
               SEEK_CUR) != 0)
        return -1;
    return 0;
}

static int read_member(const char *path, const char *wanted, unsigned char **data, size_t *len,
                       char sha[65]) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    tar_entry_t entry = {.f = f};
    int found = 0;
    int rc;
    while ((rc = tar_next(&entry)) == 0) {
        if (strcmp(entry.name, wanted) != 0) continue;
        if (entry.size > MANIFEST_MAX && strcmp(wanted, "edge.db") != 0) break;
        unsigned char *buf = malloc(entry.size ? (size_t)entry.size : 1);
        if (!buf) break;
        if (fseek(f, entry.data_offset, SEEK_SET) != 0 ||
            fread(buf, 1, (size_t)entry.size, f) != (size_t)entry.size) {
            free(buf);
            break;
        }
        EVP_MD_CTX *ctx = EVP_MD_CTX_new();
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int digest_len = 0;
        int ok = ctx && EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) == 1 &&
                 EVP_DigestUpdate(ctx, buf, (size_t)entry.size) == 1 &&
                 EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1;
        EVP_MD_CTX_free(ctx);
        if (!ok || digest_len != 32) {
            free(buf);
            break;
        }
        for (unsigned int i = 0; i < digest_len; i++) snprintf(sha + i * 2, 3, "%02x", digest[i]);
        *data = buf;
        *len = (size_t)entry.size;
        found = 1;
        break;
    }
    fclose(f);
    return found ? 0 : -1;
}

static int hash_member(const char *path, const char *wanted, char sha[65],
                       unsigned long long *size_out) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    tar_entry_t entry = {.f = f};
    int rc;
    int ok = -1;
    while ((rc = tar_next(&entry)) == 0) {
        if (strcmp(entry.name, wanted) != 0) continue;
        if (fseek(f, entry.data_offset, SEEK_SET) != 0) break;
        EVP_MD_CTX *ctx = EVP_MD_CTX_new();
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int digest_len = 0;
        char buf[512];
        unsigned long long left = entry.size;
        int good = ctx && EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) == 1;
        while (good && left > 0) {
            size_t chunk = left > sizeof(buf) ? sizeof(buf) : (size_t)left;
            if (fread(buf, 1, chunk, f) != chunk || EVP_DigestUpdate(ctx, buf, chunk) != 1) {
                good = 0;
                break;
            }
            left -= chunk;
        }
        if (good) good = EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1 && digest_len == 32;
        EVP_MD_CTX_free(ctx);
        if (!good) break;
        for (unsigned int i = 0; i < digest_len; i++) snprintf(sha + i * 2, 3, "%02x", digest[i]);
        if (size_out) *size_out = entry.size;
        ok = 0;
        break;
    }
    fclose(f);
    return ok;
}

static int copy_member(const char *path, const char *wanted, const char *dest) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    tar_entry_t entry = {.f = f};
    int rc;
    int done = -1;
    while ((rc = tar_next(&entry)) == 0) {
        if (strcmp(entry.name, wanted) != 0) continue;
        FILE *out = fopen(dest, "wb");
        if (!out) break;
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
        fclose(out);
        if (ok) done = 0;
        break;
    }
    fclose(f);
    return done;
}

static int parse_schema_version(const char *db_path) {
    sqlite3 *db = NULL;
    if (sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return -1;
    }
    sqlite3_stmt *stmt = NULL;
    int version = -1;
    if (sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, NULL) == SQLITE_OK &&
        sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return version;
}

static int sqlite_snapshot(const char *src_path, const char *dst_path) {
    sqlite3 *src = NULL;
    sqlite3 *dst = NULL;
    int rc = -1;
    if (sqlite3_open_v2(src_path, &src, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) goto out;
    if (sqlite3_open_v2(dst_path, &dst, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) !=
        SQLITE_OK)
        goto out;
    sqlite3_backup *backup = sqlite3_backup_init(dst, "main", src, "main");
    if (!backup) goto out;
    sqlite3_backup_step(backup, -1);
    if (sqlite3_backup_finish(backup) != SQLITE_OK) goto out;
    rc = 0;
out:
    if (dst) sqlite3_close(dst);
    if (src) sqlite3_close(src);
    return rc;
}

int edge_backup_create(const char *db_path, const char *out_path, char *reason,
                       size_t reason_size) {
    if (!db_path || !out_path) return fail(reason, reason_size, "missing path");
    int schema_version = parse_schema_version(db_path);
    if (schema_version < 0) return fail(reason, reason_size, "cannot read database schema");

    char snapshot[512];
    char bundle[512];
    if (snprintf(snapshot, sizeof(snapshot), "%s.dbtmp", out_path) >= (int)sizeof(snapshot) ||
        snprintf(bundle, sizeof(bundle), "%s.tmp", out_path) >= (int)sizeof(bundle)) {
        return fail(reason, reason_size, "path too long");
    }
    if (sqlite_snapshot(db_path, snapshot) != 0) {
        unlink(snapshot);
        return fail(reason, reason_size, "database snapshot failed");
    }

    (void)chmod(snapshot, 0600);
    char digest[65];
    unsigned long long db_size = 0;
    if (edge_sha256_file(snapshot, digest, &db_size) != 0) {
        unlink(snapshot);
        return fail(reason, reason_size, "database digest failed");
    }

    cJSON *manifest = cJSON_CreateObject();
    cJSON *files = cJSON_CreateArray();
    if (!manifest || !files) {
        cJSON_Delete(manifest);
        cJSON_Delete(files);
        unlink(snapshot);
        return fail(reason, reason_size, "out of memory");
    }
    cJSON_AddStringToObject(manifest, "format", EDGE_BACKUP_FORMAT);
    cJSON_AddNumberToObject(manifest, "created_at", (double)time(NULL));
    cJSON_AddNumberToObject(manifest, "schema_version", schema_version);
    cJSON *entry = cJSON_CreateObject();
    cJSON_AddStringToObject(entry, "name", "edge.db");
    cJSON_AddNumberToObject(entry, "size", (double)db_size);
    cJSON_AddStringToObject(entry, "sha256", digest);
    cJSON_AddItemToArray(files, entry);
    cJSON_AddItemToObject(manifest, "files", files);
    char *manifest_text = cJSON_PrintUnformatted(manifest);
    cJSON_Delete(manifest);
    if (!manifest_text) {
        unlink(snapshot);
        return fail(reason, reason_size, "out of memory");
    }

    char manifest_path[512];
    snprintf(manifest_path, sizeof(manifest_path), "%s.manifest.tmp", out_path);
    FILE *mf = fopen(manifest_path, "wb");
    size_t manifest_len = strlen(manifest_text);
    if (!mf || fwrite(manifest_text, 1, manifest_len, mf) != manifest_len) {
        if (mf) fclose(mf);
        free(manifest_text);
        unlink(snapshot);
        unlink(manifest_path);
        return fail(reason, reason_size, "manifest write failed");
    }
    fclose(mf);
    free(manifest_text);

    FILE *out = fopen(bundle, "wb");
    if (!out) {
        unlink(snapshot);
        unlink(manifest_path);
        return fail(reason, reason_size, "cannot create bundle");
    }
    int rc = 0;
    if (tar_write_file(out, "manifest.json", manifest_path) != 0 ||
        tar_write_file(out, "edge.db", snapshot) != 0) {
        rc = -1;
    }
    if (rc == 0 && edge_tar_finish(out) != 0) rc = -1;
    if (rc == 0 && (fflush(out) != 0 || fsync(fileno(out)) != 0)) rc = -1;
    fclose(out);
    unlink(snapshot);
    unlink(manifest_path);
    if (rc != 0) {
        unlink(bundle);
        return fail(reason, reason_size, "bundle write failed");
    }
    if (rename(bundle, out_path) != 0) {
        unlink(bundle);
        return fail(reason, reason_size, "bundle rename failed");
    }
    return 0;
}

int edge_backup_verify(const char *bundle_path, char *reason, size_t reason_size) {
    if (!bundle_path) return fail(reason, reason_size, "missing bundle");
    unsigned char *manifest_data = NULL;
    size_t manifest_len = 0;
    char manifest_sha[65] = {0};
    if (read_member(bundle_path, "manifest.json", &manifest_data, &manifest_len, manifest_sha) != 0) {
        return fail(reason, reason_size, "manifest missing or unreadable");
    }
    char db_sha[65] = {0};
    int db_ok = hash_member(bundle_path, "edge.db", db_sha, NULL) == 0;

    cJSON *manifest = cJSON_ParseWithLength((const char *)manifest_data, manifest_len);
    free(manifest_data);
    if (!manifest) return fail(reason, reason_size, "manifest is not json");
    cJSON *format = cJSON_GetObjectItem(manifest, "format");
    cJSON *schema = cJSON_GetObjectItem(manifest, "schema_version");
    cJSON *files = cJSON_GetObjectItem(manifest, "files");
    if (!cJSON_IsString(format) || strcmp(format->valuestring, EDGE_BACKUP_FORMAT) != 0 ||
        !cJSON_IsNumber(schema) || !cJSON_IsArray(files)) {
        cJSON_Delete(manifest);
        return fail(reason, reason_size, "manifest shape is invalid");
    }
    if (schema->valueint > sqlite_service_schema_version()) {
        cJSON_Delete(manifest);
        return fail(reason, reason_size, "backup schema is newer than this platform");
    }
    if (!db_ok) {
        cJSON_Delete(manifest);
        return fail(reason, reason_size, "database member missing");
    }
    int matched = 0;
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, files) {
        cJSON *name = cJSON_GetObjectItem(entry, "name");
        cJSON *sha = cJSON_GetObjectItem(entry, "sha256");
        if (!cJSON_IsString(name) || !cJSON_IsString(sha)) {
            cJSON_Delete(manifest);
            return fail(reason, reason_size, "manifest file entry is invalid");
        }
        if (strcmp(name->valuestring, "edge.db") == 0) {
            if (strcmp(sha->valuestring, db_sha) != 0) {
                cJSON_Delete(manifest);
                return fail(reason, reason_size, "database digest mismatch");
            }
            matched++;
        } else {
            cJSON_Delete(manifest);
            return fail(reason, reason_size, "unexpected member name");
        }
    }
    cJSON_Delete(manifest);
    if (matched != 1) return fail(reason, reason_size, "manifest does not cover the database");
    return 0;
}

int edge_backup_restore(const char *bundle_path, const char *db_path, char *reason,
                        size_t reason_size) {
    if (!bundle_path || !db_path) return fail(reason, reason_size, "missing path");
    if (edge_backup_verify(bundle_path, reason, reason_size) != 0) return -1;

    char tmp[512];
    char previous[512];
    if (snprintf(tmp, sizeof(tmp), "%s.restore.tmp", db_path) >= (int)sizeof(tmp) ||
        snprintf(previous, sizeof(previous), "%s.before-restore", db_path) >=
            (int)sizeof(previous)) {
        return fail(reason, reason_size, "path too long");
    }
    if (copy_member(bundle_path, "edge.db", tmp) != 0) {
        unlink(tmp);
        return fail(reason, reason_size, "database extract failed");
    }
    if (access(db_path, F_OK) == 0) {
        unlink(previous);
        if (rename(db_path, previous) != 0) {
            unlink(tmp);
            return fail(reason, reason_size, "cannot move the current database aside");
        }
    }
    if (rename(tmp, db_path) != 0) {
        unlink(tmp);
        return fail(reason, reason_size, "cannot install the restored database");
    }
    char side[512];
    if (snprintf(side, sizeof(side), "%s-wal", db_path) < (int)sizeof(side)) unlink(side);
    if (snprintf(side, sizeof(side), "%s-shm", db_path) < (int)sizeof(side)) unlink(side);
    return 0;
}
