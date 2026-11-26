/**
 * @file history_bundle.c
 * @brief `edgeos-history/1` ustar bundle with a SHA-256-pinned manifest.
 */

#include "history_bundle.h"

#include <cjson/cJSON.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "edge_digest.h"

#define MANIFEST_MAX (64u * 1024u)

static edge_history_result_t fail(edge_history_result_t rc, char *reason, size_t reason_size,
                                  const char *text) {
    if (reason && reason_size) snprintf(reason, reason_size, "%s", text);
    return rc;
}

static edge_history_result_t io_result(void) {
    if (errno == ENOSPC || errno == EDQUOT) return EDGE_HISTORY_NO_SPACE;
    return EDGE_HISTORY_IO;
}

static int is_sha256_hex(const char *text) {
    if (!text || strlen(text) != 64) return 0;
    for (size_t i = 0; i < 64; i++) {
        if (!((text[i] >= '0' && text[i] <= '9') || (text[i] >= 'a' && text[i] <= 'f'))) return 0;
    }
    return 1;
}

static void fsync_parent(const char *path) {
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
    } else {
        snprintf(dir, sizeof(dir), ".");
    }
    int fd = open(dir, O_RDONLY | O_DIRECTORY);
    if (fd >= 0) {
        (void)fsync(fd);
        close(fd);
    }
}

static edge_history_result_t write_member(FILE *out, const edge_history_member_t *member,
                                          unsigned long long *payload_written,
                                          unsigned long long fail_after, char *reason,
                                          size_t reason_size) {
    FILE *src = fopen(member->path, "rb");
    if (!src) return fail(io_result(), reason, reason_size, "member source is unreadable");
    if (fseeko(src, 0, SEEK_END) != 0) {
        fclose(src);
        return fail(io_result(), reason, reason_size, "member seek failed");
    }
    off_t end = ftello(src);
    if (end < 0 || fseeko(src, 0, SEEK_SET) != 0) {
        fclose(src);
        return fail(io_result(), reason, reason_size, "member size is unreadable");
    }
    unsigned long long size = (unsigned long long)end;
    char header[EDGE_TAR_BLOCK];
    edge_tar_header(header, member->name, size);
    if (fwrite(header, 1, sizeof(header), out) != sizeof(header)) {
        fclose(src);
        return fail(io_result(), reason, reason_size, "bundle header write failed");
    }
    char buf[512];
    size_t got;
    while ((got = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, got, out) != got) {
            fclose(src);
            return fail(io_result(), reason, reason_size, "bundle data write failed");
        }
        *payload_written += got;
        if (fail_after && *payload_written > fail_after) {
            fclose(src);
            return fail(EDGE_HISTORY_NO_SPACE, reason, reason_size, "simulated disk full");
        }
    }
    if (ferror(src)) {
        fclose(src);
        return fail(io_result(), reason, reason_size, "member read failed");
    }
    fclose(src);
    size_t pad = (size_t)((EDGE_TAR_BLOCK - size % EDGE_TAR_BLOCK) % EDGE_TAR_BLOCK);
    if (pad) {
        char zeros[EDGE_TAR_BLOCK];
        memset(zeros, 0, sizeof(zeros));
        if (fwrite(zeros, 1, pad, out) != pad) {
            return fail(io_result(), reason, reason_size, "bundle padding write failed");
        }
    }
    return EDGE_HISTORY_OK;
}

static int duplicate_member(const edge_history_bundle_desc_t *desc, size_t upto) {
    for (size_t i = 0; i < upto; i++) {
        if (strcmp(desc->members[i].name, desc->members[upto].name) == 0) return 1;
    }
    return 0;
}

static char *build_manifest(const edge_history_bundle_desc_t *desc, char *reason,
                            size_t reason_size, edge_history_result_t *rc) {
    cJSON *manifest = cJSON_CreateObject();
    cJSON *files = cJSON_CreateArray();
    if (!manifest || !files) {
        cJSON_Delete(manifest);
        cJSON_Delete(files);
        *rc = fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
        return NULL;
    }
    cJSON_AddStringToObject(manifest, "format", EDGE_HISTORY_FORMAT);
    cJSON_AddStringToObject(manifest, "store", desc->store);
    cJSON_AddNumberToObject(manifest, "schema_version", (double)desc->schema_version);
    cJSON_AddNumberToObject(manifest, "from_ms", (double)desc->from_ms);
    cJSON_AddNumberToObject(manifest, "to_ms", (double)desc->to_ms);
    cJSON_AddNumberToObject(manifest, "created_ms", (double)desc->created_ms);
    cJSON_AddStringToObject(manifest, "time_unit", "epoch_ms");
    cJSON_AddBoolToObject(manifest, "includes_active", desc->includes_active != 0);
    cJSON_AddNumberToObject(manifest, "watermark_ms", (double)desc->watermark_ms);
    cJSON_AddStringToObject(manifest, "source_fingerprint", desc->source_fingerprint);
    cJSON_AddNumberToObject(manifest, "row_count", (double)desc->row_count);
    if (desc->source_db || desc->source_table || desc->source_schema) {
        cJSON *source = cJSON_CreateObject();
        if (!source) {
            cJSON_Delete(manifest);
            cJSON_Delete(files);
            *rc = fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
            return NULL;
        }
        if (desc->source_db) cJSON_AddStringToObject(source, "db", desc->source_db);
        if (desc->source_table) cJSON_AddStringToObject(source, "table", desc->source_table);
        if (desc->source_schema) cJSON_AddStringToObject(source, "schema", desc->source_schema);
        cJSON_AddItemToObject(manifest, "source", source);
    }

    unsigned long long rows_sum = 0;
    for (size_t i = 0; i < desc->member_count; i++) {
        const edge_history_member_t *member = &desc->members[i];
        char sha[EDGE_HISTORY_SHA256_SIZE];
        unsigned long long size = 0;
        if (edge_sha256_file(member->path, sha, &size) != 0) {
            cJSON_Delete(manifest);
            cJSON_Delete(files);
            *rc = fail(EDGE_HISTORY_IO, reason, reason_size, "member is unreadable");
            return NULL;
        }
        cJSON *entry = cJSON_CreateObject();
        if (!entry) {
            cJSON_Delete(manifest);
            cJSON_Delete(files);
            *rc = fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
            return NULL;
        }
        cJSON_AddStringToObject(entry, "name", member->name);
        cJSON_AddNumberToObject(entry, "size", (double)size);
        cJSON_AddStringToObject(entry, "sha256", sha);
        if (member->has_rows) {
            cJSON_AddNumberToObject(entry, "rows", (double)member->rows);
            rows_sum += member->rows;
        }
        cJSON_AddItemToArray(files, entry);
    }
    if (rows_sum != desc->row_count) {
        cJSON_Delete(manifest);
        cJSON_Delete(files);
        *rc = fail(EDGE_HISTORY_INVALID, reason, reason_size, "row count does not match members");
        return NULL;
    }
    cJSON_AddItemToObject(manifest, "files", files);
    char *text = cJSON_PrintUnformatted(manifest);
    cJSON_Delete(manifest);
    if (!text) {
        *rc = fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
        return NULL;
    }
    if (strlen(text) > MANIFEST_MAX) {
        free(text);
        *rc = fail(EDGE_HISTORY_INVALID, reason, reason_size, "manifest is too large");
        return NULL;
    }
    *rc = EDGE_HISTORY_OK;
    return text;
}

edge_history_result_t edge_history_bundle_write(const char *out_path,
                                                const edge_history_bundle_desc_t *desc,
                                                unsigned long long fail_after_bytes, char *reason,
                                                size_t reason_size) {
    if (reason && reason_size) reason[0] = '\0';
    if (!out_path || !desc || !desc->format || strcmp(desc->format, EDGE_HISTORY_FORMAT) != 0 ||
        !desc->store || desc->store[0] == '\0' || desc->schema_version < 1 ||
        desc->from_ms > desc->to_ms || !desc->members || desc->member_count == 0 ||
        desc->member_count >= EDGE_TAR_MAX_MEMBERS || !is_sha256_hex(desc->source_fingerprint)) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "bundle description is invalid");
    }
    for (size_t i = 0; i < desc->member_count; i++) {
        const char *name = desc->members[i].name;
        if (!name || name[0] == '\0' || strlen(name) >= 100 || strcmp(name, "manifest.json") == 0 ||
            !desc->members[i].path || duplicate_member(desc, i)) {
            return fail(EDGE_HISTORY_INVALID, reason, reason_size, "member name is invalid");
        }
    }

    edge_history_result_t rc = EDGE_HISTORY_OK;
    char *manifest = build_manifest(desc, reason, reason_size, &rc);
    if (!manifest) return rc;

    char tmp[512];
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", out_path) >= (int)sizeof(tmp)) {
        free(manifest);
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "path too long");
    }
    unlink(tmp);
    FILE *out = edge_tar_fopen_private(tmp, "wb");
    if (!out) {
        free(manifest);
        return fail(io_result(), reason, reason_size, "cannot create bundle");
    }

    unsigned long long payload_written = 0;
    rc = EDGE_HISTORY_OK;
    for (size_t i = 0; i < desc->member_count && rc == EDGE_HISTORY_OK; i++) {
        rc = write_member(out, &desc->members[i], &payload_written, fail_after_bytes, reason,
                          reason_size);
    }
    if (rc == EDGE_HISTORY_OK &&
        edge_tar_write_entry(out, "manifest.json", manifest, strlen(manifest)) != 0) {
        rc = fail(io_result(), reason, reason_size, "manifest write failed");
    }
    if (rc == EDGE_HISTORY_OK && edge_tar_finish(out) != 0) {
        rc = fail(io_result(), reason, reason_size, "bundle terminator write failed");
    }
    if (rc == EDGE_HISTORY_OK && (fflush(out) != 0 || fsync(fileno(out)) != 0)) {
        rc = fail(io_result(), reason, reason_size, "bundle sync failed");
    }
    if (fclose(out) != 0 && rc == EDGE_HISTORY_OK) {
        rc = fail(io_result(), reason, reason_size, "bundle close failed");
    }
    free(manifest);
    if (rc != EDGE_HISTORY_OK) {
        unlink(tmp);
        return rc;
    }

    edge_history_bundle_info_t info;
    rc = edge_history_bundle_verify(tmp, desc->store, desc->schema_version, &info, reason,
                                    reason_size);
    if (rc != EDGE_HISTORY_OK) {
        unlink(tmp);
        return rc;
    }
    if (rename(tmp, out_path) != 0) {
        unlink(tmp);
        return fail(io_result(), reason, reason_size, "bundle rename failed");
    }
    fsync_parent(out_path);
    return EDGE_HISTORY_OK;
}

static const char *json_string(const cJSON *root, const char *key) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

static int json_i64(const cJSON *root, const char *key, int64_t *out) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsNumber(item)) return -1;
    double value = item->valuedouble;
    if (value < -9.0e18 || value > 9.0e18) return -1;
    *out = (int64_t)value;
    return 0;
}

static int json_u64(const cJSON *root, const char *key, unsigned long long *out) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsNumber(item)) return -1;
    double value = item->valuedouble;
    if (value < 0.0 || value > 9.0e18) return -1;
    *out = (unsigned long long)value;
    return 0;
}

static int listed_count(const cJSON *files, const char *name) {
    int matches = 0;
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, files) {
        const char *listed = json_string(entry, "name");
        if (listed && strcmp(listed, name) == 0) matches++;
    }
    return matches;
}

edge_history_result_t edge_history_bundle_verify(const char *path, const char *expect_store,
                                                 int max_schema_version,
                                                 edge_history_bundle_info_t *info, char *reason,
                                                 size_t reason_size) {
    if (reason && reason_size) reason[0] = '\0';
    if (!path || !info) return fail(EDGE_HISTORY_INVALID, reason, reason_size, "missing path");
    char(*names)[EDGE_TAR_NAME_SIZE] = malloc(EDGE_TAR_MAX_MEMBERS * sizeof(*names));
    if (!names) return fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
    size_t name_count = 0;
    if (edge_tar_list(path, names, EDGE_TAR_MAX_MEMBERS, &name_count) != 0) {
        free(names);
        return fail(EDGE_HISTORY_VERIFY, reason, reason_size, "bundle is malformed");
    }
    size_t manifest_count = 0;
    for (size_t i = 0; i < name_count; i++) {
        if (strcmp(names[i], "manifest.json") == 0) manifest_count++;
    }
    if (manifest_count != 1) {
        free(names);
        return fail(EDGE_HISTORY_VERIFY, reason, reason_size, "manifest missing or duplicated");
    }

    unsigned char *text = NULL;
    size_t text_len = 0;
    char manifest_sha[EDGE_HISTORY_SHA256_SIZE];
    if (edge_tar_read_member(path, "manifest.json", MANIFEST_MAX, &text, &text_len, manifest_sha) !=
        0) {
        free(names);
        return fail(EDGE_HISTORY_VERIFY, reason, reason_size, "manifest is unreadable");
    }
    cJSON *root = cJSON_ParseWithLength((const char *)text, text_len);
    free(text);
    if (!root) {
        free(names);
        return fail(EDGE_HISTORY_VERIFY, reason, reason_size, "manifest is not json");
    }

    edge_history_result_t rc = EDGE_HISTORY_VERIFY;
    size_t listed = 0;
    unsigned long long rows_sum = 0;
    const cJSON *entry = NULL;
    const char *format = json_string(root, "format");
    const char *store = json_string(root, "store");
    const char *time_unit = json_string(root, "time_unit");
    const char *fingerprint = json_string(root, "source_fingerprint");
    const cJSON *files = cJSON_GetObjectItemCaseSensitive(root, "files");
    const cJSON *includes_active = cJSON_GetObjectItemCaseSensitive(root, "includes_active");
    int64_t from_ms = 0, to_ms = 0, created_ms = 0, watermark_ms = 0;
    unsigned long long row_count = 0;
    int schema_version = 0;
    const cJSON *schema = cJSON_GetObjectItemCaseSensitive(root, "schema_version");
    if (!format || strcmp(format, EDGE_HISTORY_FORMAT) != 0 || !store || store[0] == '\0' ||
        !time_unit || strcmp(time_unit, "epoch_ms") != 0 || !is_sha256_hex(fingerprint) ||
        !cJSON_IsArray(files) || cJSON_GetArraySize(files) == 0 || !cJSON_IsBool(includes_active) ||
        !cJSON_IsNumber(schema) || schema->valuedouble < 1.0 ||
        schema->valuedouble > 2147483647.0 || json_i64(root, "from_ms", &from_ms) != 0 ||
        json_i64(root, "to_ms", &to_ms) != 0 || json_i64(root, "created_ms", &created_ms) != 0 ||
        json_i64(root, "watermark_ms", &watermark_ms) != 0 ||
        json_u64(root, "row_count", &row_count) != 0 || from_ms < 0 || to_ms < from_ms ||
        created_ms < 0 || watermark_ms < 0) {
        goto out;
    }
    schema_version = (int)schema->valuedouble;
    if (schema_version > max_schema_version) {
        rc = EDGE_HISTORY_INVALID;
        fail(rc, reason, reason_size, "bundle schema is newer than this platform");
        goto out;
    }
    if (expect_store && strcmp(store, expect_store) != 0) goto out;

    listed = (size_t)cJSON_GetArraySize(files);
    if (name_count != listed + 1) goto out;
    if (strcmp(names[name_count - 1], "manifest.json") != 0) goto out;
    cJSON_ArrayForEach(entry, files) {
        const char *name = json_string(entry, "name");
        const char *sha = json_string(entry, "sha256");
        unsigned long long size = 0;
        if (!name || !sha || !is_sha256_hex(sha) || strcmp(name, "manifest.json") == 0 ||
            json_u64(entry, "size", &size) != 0) {
            goto out;
        }
        int in_archive = 0;
        for (size_t i = 0; i < name_count; i++) {
            if (strcmp(names[i], name) == 0) in_archive = 1;
        }
        if (!in_archive || listed_count(files, name) != 1) goto out;
        char member_sha[EDGE_HISTORY_SHA256_SIZE];
        unsigned long long member_size = 0;
        if (edge_tar_hash_member(path, name, member_sha, &member_size) != 0 ||
            member_size != size || strcmp(member_sha, sha) != 0) {
            goto out;
        }
        const cJSON *rows = cJSON_GetObjectItemCaseSensitive(entry, "rows");
        if (rows) {
            unsigned long long row_value = 0;
            if (json_u64(entry, "rows", &row_value) != 0) goto out;
            rows_sum += row_value;
        }
    }
    if (rows_sum != row_count) goto out;

    snprintf(info->store, sizeof(info->store), "%s", store);
    info->schema_version = schema_version;
    info->from_ms = from_ms;
    info->to_ms = to_ms;
    info->created_ms = created_ms;
    info->watermark_ms = watermark_ms;
    info->includes_active = cJSON_IsTrue(includes_active) ? 1 : 0;
    info->row_count = row_count;
    snprintf(info->source_fingerprint, sizeof(info->source_fingerprint), "%s", fingerprint);
    info->member_count = (uint64_t)listed;
    rc = EDGE_HISTORY_OK;

out:
    cJSON_Delete(root);
    free(names);
    if (rc != EDGE_HISTORY_OK && reason && reason_size && reason[0] == '\0') {
        snprintf(reason, reason_size, "%s", "bundle verification failed");
    }
    return rc;
}
