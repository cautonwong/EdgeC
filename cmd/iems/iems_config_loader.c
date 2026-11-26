/**
 * @file iems_config_loader.c
 * @brief P6-04b: converted device/port config families -> inventory/poll_tasks.
 *
 * The loader validates the whole document set before it writes anything. The
 * SQLite apply step replaces both tables in one transaction, so a rejected
 * config never leaves a half-loaded registry. The poll scheduler is untouched;
 * it observes the rows through iems_poll_tasks_load/iems_poll_task_route.
 */

#include "iems_config_loader.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cjson/cJSON.h>

#include "edge_log.h"
#include "iems_config_loader_priv.h"
#include "iems_driver_adapter.h"
#include "iems_point_model.h"

#define IEMS_CONFIG_VERSION "1.0.0"
#define IEMS_CONFIG_MAX_FILE_BYTES (1024u * 1024u)
#define IEMS_CONFIG_MAX_LC_ENTRIES 32u

#define IEMS_CONFIG_DEVICE_FILE "device_config.json"
#define IEMS_CONFIG_DEF_FILE "def_device_config.json"
#define IEMS_CONFIG_LC_FILE "lc/device_config.json"
#define IEMS_CONFIG_PORT_FILE "port_config.json"

/* 配置族 schema 无口令字段。这些键出现在两族里只能是错误，值为明文时绝不回显。 */
static const char *const k_sensitive_keys[] = {
    "password", "passwd", "pass", "secret", "token",
    "api_key", "private_key", "access_key", "secret_key",
};

static const char *const k_device_root_keys[] = {"schema_version", "dev_class"};
static const char *const k_class_keys[] = {"type", "devs"};
static const char *const k_dev_keys[] = {"no",   "name", "type",     "channel", "template",
                                         "protocol", "user_def", "addr", "period"};
static const char *const k_port_root_keys[] = {"schema_version", "dev_cfg", "ver"};
static const char *const k_dev_cfg_keys[] = {"uart", "lan", "wan", "can", "di", "do"};
static const char *const k_uart_keys[] = {"name", "baud", "data_bit", "stop_bit", "parity"};
static const char *const k_can_keys[] = {"name", "baud", "filter"};
static const char *const k_lan_keys[] = {"name", "type", "ip", "mask"};
static const char *const k_wan_keys[] = {"name", "type", "ip", "gateway", "mask", "metric"};
static const char *const k_di_keys[] = {"name", "disp_name", "ncno"};
static const char *const k_do_keys[] = {"name", "disp_name"};
static const char *const k_filter_keys[] = {"id", "mask"};

/* 只有已开通的 SOUTHBOUND 总线才能被设备路由。新增总线必须先加
 * iems_driver_adapter.c 的 connectivity 条目，否则控制写会落到未初始化的 bus。 */
static const struct {
    const char *name;
    iems_config_port_kind_t kind;
    int bus_id;
} k_channel_bus[] = {
    {"RS485_1", IEMS_CONFIG_PORT_UART, 0},
    {"CAN0", IEMS_CONFIG_PORT_CAN, 1},
};

/* ------------------------------------------------------------------ */
/* Errors                                                             */
/* ------------------------------------------------------------------ */

int iems_config_fail(char *err, size_t err_size, const char *kind, const char *path,
                 const char *detail) {
    if (err && err_size) {
        if (detail) {
            snprintf(err, err_size, "%s %s: %s", kind, path ? path : "$", detail);
        } else {
            snprintf(err, err_size, "%s %s", kind, path ? path : "$");
        }
    }
    return -1;
}

void iems_config_path_key(char *buf, size_t cap, const char *base, const char *key) {
    if (!buf || cap == 0) return;
    size_t used = 0;
    while (base && base[used] && used + 1 < cap) {
        buf[used] = base[used];
        used++;
    }
    if (used + 1 < cap) buf[used++] = '.';
    for (size_t i = 0; key && key[i] && used + 1 < cap; i++) buf[used++] = key[i];
    buf[used] = '\0';
}

void iems_config_path_index(char *buf, size_t cap, const char *base, size_t index) {
    if (!buf || cap == 0) return;
    size_t used = 0;
    while (base && base[used] && used + 1 < cap) {
        buf[used] = base[used];
        used++;
    }
    if (used + 1 < cap) buf[used++] = '[';
    char digits[24];
    int n = snprintf(digits, sizeof(digits), "%zu", index);
    for (int i = 0; i < n && used + 1 < cap; i++) buf[used++] = digits[i];
    if (used + 1 < cap) buf[used++] = ']';
    buf[used] = '\0';
}

/* ------------------------------------------------------------------ */
/* Source: filesystem adapter                                         */
/* ------------------------------------------------------------------ */

static iems_config_file_source_t *SELF(void *ctx) {
    return (iems_config_file_source_t *)ctx;
}

static int _safe_relative(const char *path) {
    if (!path || !path[0] || path[0] == '/') return 0;
    if (strstr(path, "..")) return 0;
    return 1;
}

static iems_config_source_rc_t _source_fail(char *err, size_t err_size, const char *kind,
                                            const char *path, const char *detail) {
    iems_config_fail(err, err_size, kind, path, detail);
    return IEMS_CONFIG_SOURCE_ERROR;
}

static int _within_root(const char *real_root, const char *target) {
    size_t n = strlen(real_root);
    return strncmp(target, real_root, n) == 0 && (target[n] == '/' || target[n] == '\0');
}

static iems_config_source_rc_t _fs_read(void *ctx, const char *relative_path, char **data,
                                       size_t *len, char *err, size_t err_size) {
    if (data) *data = NULL;
    if (len) *len = 0;
    if (!ctx || !data || !_safe_relative(relative_path)) {
        return _source_fail(err, err_size, "READ_FAILED", relative_path ? relative_path : "$",
                            "invalid relative path");
    }
    iems_config_file_source_t *fs = SELF(ctx);
    if (fs->real_root[0] == '\0') {
        return _source_fail(err, err_size, "SOURCE_ERROR", fs->root, "config root unavailable");
    }

    char full[1024];  // stack-ok: root[512] + relative path (<=128) + NUL
    int n = snprintf(full, sizeof(full), "%s/%s", fs->root, relative_path);
    if (n <= 0 || (size_t)n >= sizeof(full)) {
        return _source_fail(err, err_size, "READ_FAILED", relative_path, "path too long");
    }
    char resolved[PATH_MAX];  // stack-ok: realpath requires a PATH_MAX buffer
    if (!realpath(full, resolved)) {
        /* Only ENOENT is a missing file. ENOTDIR means a parent component is
         * not a directory (for example "lc" is a regular file): treating that
         * as absent silently drops the whole LC tree. It is a source error. */
        if (errno == ENOENT) return IEMS_CONFIG_SOURCE_ABSENT;
        return _source_fail(err, err_size, "SOURCE_ERROR", relative_path, strerror(errno));
    }
    if (!_within_root(fs->real_root, resolved)) {
        return _source_fail(err, err_size, "UNSAFE_PATH", relative_path,
                            "resolves outside config root");
    }
    int fd = open(resolved, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return _source_fail(err, err_size, "READ_FAILED", relative_path, strerror(errno));
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
        int saved = errno;
        close(fd);
        return _source_fail(err, err_size, "SOURCE_ERROR", relative_path, strerror(saved));
    }
    if (S_ISDIR(st.st_mode)) {
        close(fd);
        return _source_fail(err, err_size, "READ_FAILED", relative_path, "is a directory");
    }
    if (!S_ISREG(st.st_mode) || st.st_size < 0) {
        close(fd);
        return _source_fail(err, err_size, "READ_FAILED", relative_path, "size unavailable");
    }
    if ((unsigned long)st.st_size > IEMS_CONFIG_MAX_FILE_BYTES) {
        close(fd);
        return _source_fail(err, err_size, "READ_FAILED", relative_path, "file exceeds 1 MiB");
    }
    FILE *f = fdopen(fd, "rb");
    if (!f) {
        int saved = errno;
        close(fd);
        return _source_fail(err, err_size, "READ_FAILED", relative_path, strerror(saved));
    }
    size_t size = (size_t)st.st_size;
    char *buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return _source_fail(err, err_size, "READ_FAILED", relative_path, "out of memory");
    }
    size_t got = fread(buf, 1, size, f);
    fclose(f);
    if (got != size) {
        free(buf);
        return _source_fail(err, err_size, "READ_FAILED", relative_path, "short read");
    }
    buf[size] = '\0';
    *data = buf;
    if (len) *len = size;
    return IEMS_CONFIG_SOURCE_OK;
}

static iems_config_source_rc_t _fs_list(void *ctx, const char *relative_dir,
                                       int (*on_entry)(void *cb_ctx, const char *name),
                                       void *cb_ctx, char *err, size_t err_size) {
    if (!ctx || !on_entry || !_safe_relative(relative_dir)) {
        return _source_fail(err, err_size, "SOURCE_ERROR", relative_dir ? relative_dir : "$",
                            "invalid relative path");
    }
    iems_config_file_source_t *fs = SELF(ctx);
    if (fs->real_root[0] == '\0') {
        return _source_fail(err, err_size, "SOURCE_ERROR", fs->root, "config root unavailable");
    }

    char full[1024];  // stack-ok: root[512] + relative dir (<=64) + NUL
    int n = snprintf(full, sizeof(full), "%s/%s", fs->root, relative_dir);
    if (n <= 0 || (size_t)n >= sizeof(full)) {
        return _source_fail(err, err_size, "SOURCE_ERROR", relative_dir, "path too long");
    }
    char resolved[PATH_MAX];  // stack-ok: realpath requires a PATH_MAX buffer
    if (!realpath(full, resolved)) {
        if (errno == ENOENT) return IEMS_CONFIG_SOURCE_ABSENT;
        return _source_fail(err, err_size, "SOURCE_ERROR", relative_dir, strerror(errno));
    }
    if (!_within_root(fs->real_root, resolved)) {
        return _source_fail(err, err_size, "UNSAFE_PATH", relative_dir,
                            "resolves outside config root");
    }
    struct stat st;
    if (stat(resolved, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return _source_fail(err, err_size, "SOURCE_ERROR", relative_dir, "not a directory");
    }

    DIR *dir = opendir(resolved);
    if (!dir) {
        if (errno == ENOENT) return IEMS_CONFIG_SOURCE_ABSENT;
        return _source_fail(err, err_size, "SOURCE_ERROR", relative_dir, strerror(errno));
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (on_entry(cb_ctx, entry->d_name) != 0) {
            closedir(dir);
            return _source_fail(err, err_size, "SOURCE_ERROR", relative_dir,
                                "entry callback aborted");
        }
    }
    closedir(dir);
    return IEMS_CONFIG_SOURCE_OK;
}

static const iems_config_source_t k_file_source = {
    .read_file = _fs_read,
    .list_dir = _fs_list,
};

void iems_config_file_source_init(iems_config_file_source_t *fs, const char *root) {
    if (!fs) return;
    memset(fs, 0, sizeof(*fs));
    if (!root) return;
    snprintf(fs->root, sizeof(fs->root), "%s", root);
    char *resolved = realpath(root, NULL);
    if (resolved) {
        snprintf(fs->real_root, sizeof(fs->real_root), "%s", resolved);
        free(resolved);
    }
}

iems_config_source_t iems_config_file_source_port(iems_config_file_source_t *fs) {
    iems_config_source_t source = k_file_source;
    source.ctx = fs;
    return source;
}

/* ------------------------------------------------------------------ */
/* JSON helpers                                                       */
/* ------------------------------------------------------------------ */

static int _is_secret_ref(const char *value) {
    if (!value || strncmp(value, "$secret:", 8) != 0 || value[8] == '\0') return 0;
    for (const char *p = value + 8; *p; p++) {
        if (!(isalnum((unsigned char)*p) || *p == '_' || *p == '.' || *p == '-')) return 0;
    }
    return 1;
}

static int _is_env_ref(const char *value) {
    if (!value || value[0] != '$' || value[1] != '{') return 0;
    const char *p = value + 2;
    if (!(isalpha((unsigned char)*p) || *p == '_')) return 0;
    while (isalnum((unsigned char)*p) || *p == '_') p++;
    return p[0] == '}' && p[1] == '\0';
}

int iems_config_sensitive_scan(cJSON *object, const char *path, char *err, size_t err_size) {
    cJSON *item;
    cJSON_ArrayForEach(item, object) {
        for (size_t i = 0; i < sizeof(k_sensitive_keys) / sizeof(k_sensitive_keys[0]); i++) {
            if (strcmp(item->string, k_sensitive_keys[i]) != 0) continue;
            char field[192];
            iems_config_path_key(field, sizeof(field), path, item->string);
            if (!cJSON_IsString(item)) {
                return iems_config_fail(err, err_size, "INVALID_SECRET_FIELD", field,
                             "secret fields are not accepted in device/port config");
            }
            if (_is_secret_ref(item->valuestring) || _is_env_ref(item->valuestring)) {
                return iems_config_fail(err, err_size, "SECRET_REF_NOT_ALLOWED", field,
                             "device/port schema has no secret field");
            }
            return iems_config_fail(err, err_size, "PLAINTEXT_SECRET", field,
                         "plaintext value withheld; device/port schema has no secret field");
        }
    }
    return 0;
}

int iems_config_check_keys(cJSON *object, const char *path, const char *const *allowed,
                       size_t allowed_count, char *err, size_t err_size) {
    cJSON *item;
    cJSON_ArrayForEach(item, object) {
        int ok = 0;
        for (size_t i = 0; i < allowed_count; i++) {
            if (strcmp(item->string, allowed[i]) == 0) {
                ok = 1;
                break;
            }
        }
        if (!ok) {
            char field[192];
            iems_config_path_key(field, sizeof(field), path, item->string);
            return iems_config_fail(err, err_size, "UNKNOWN_KEY", field, NULL);
        }
    }
    return 0;
}

static int _get_string(cJSON *object, const char *key, int required, int nonempty, const char *path,
                       char *err, size_t err_size, const char **out) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    char field[192];
    iems_config_path_key(field, sizeof(field), path, key);
    if (!item) {
        if (required) return iems_config_fail(err, err_size, "MISSING_REQUIRED", field, NULL);
        if (out) *out = NULL;
        return 0;
    }
    if (!cJSON_IsString(item)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected string");
    }
    if (nonempty && item->valuestring[0] == '\0') {
        return iems_config_fail(err, err_size, "INVALID_VALUE", field, "must be non-empty");
    }
    if (out) *out = item->valuestring;
    return 0;
}

int iems_config_get_int(cJSON *object, const char *key, int required, int minimum, const char *path,
                    char *err, size_t err_size, int *out) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    char field[192];
    iems_config_path_key(field, sizeof(field), path, key);
    if (!item) {
        if (required) return iems_config_fail(err, err_size, "MISSING_REQUIRED", field, NULL);
        return 0;
    }
    if (!cJSON_IsNumber(item)) return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected integer");
    double value = item->valuedouble;
    if (value > (double)INT_MAX || value < (double)INT_MIN) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected integer");
    }
    if (value != (double)(long long)value) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected integer");
    }
    if (value < (double)minimum) return iems_config_fail(err, err_size, "INVALID_VALUE", field, "below minimum");
    if (out) *out = (int)value;
    return 0;
}

static int _get_array(cJSON *object, const char *key, int required, const char *path, char *err,
                      size_t err_size, cJSON **out) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    char field[192];
    iems_config_path_key(field, sizeof(field), path, key);
    if (!item) {
        if (required) return iems_config_fail(err, err_size, "MISSING_REQUIRED", field, NULL);
        if (out) *out = NULL;
        return 0;
    }
    if (!cJSON_IsArray(item)) return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected array");
    if (out) *out = item;
    return 0;
}

static int _get_object(cJSON *object, const char *key, int required, const char *path, char *err,
                       size_t err_size, cJSON **out) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    char field[192];
    iems_config_path_key(field, sizeof(field), path, key);
    if (!item) {
        if (required) return iems_config_fail(err, err_size, "MISSING_REQUIRED", field, NULL);
        if (out) *out = NULL;
        return 0;
    }
    if (!cJSON_IsObject(item)) return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected object");
    if (out) *out = item;
    return 0;
}

static int _copy_field(char *dst, size_t cap, const char *src, const char *path, char *err,
                       size_t err_size) {
    if (strlen(src) >= cap) return iems_config_fail(err, err_size, "VALUE_TOO_LONG", path, NULL);
    memcpy(dst, src, strlen(src) + 1);
    return 0;
}

/* 对象根 + 版本是两族共同的信任入口。数组根与 {"config": [...]} 信封都拒绝。 */
int iems_config_check_root(cJSON *root, const char *rel, char *err, size_t err_size) {
    if (cJSON_IsArray(root)) {
        return iems_config_fail(err, err_size, "ENVELOPE_NOT_ALLOWED", rel,
                     "array root belongs to array config families");
    }
    if (!cJSON_IsObject(root)) return iems_config_fail(err, err_size, "INVALID_ROOT", rel, "expected object");
    if (cJSON_GetObjectItemCaseSensitive(root, "config")) {
        return iems_config_fail(err, err_size, "ENVELOPE_NOT_ALLOWED", rel,
                     "config envelope is not accepted for device/port");
    }
    cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "schema_version");
    char path[192];
    iems_config_path_key(path, sizeof(path), rel, "schema_version");
    if (!version) return iems_config_fail(err, err_size, "MISSING_SCHEMA_VERSION", path, NULL);
    if (!cJSON_IsString(version)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", path, "expected string");
    }
    if (strcmp(version->valuestring, IEMS_CONFIG_VERSION) != 0) {
        return iems_config_fail(err, err_size, "UNSUPPORTED_SCHEMA_VERSION", path, version->valuestring);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Strict JSON precheck                                               */
/* ------------------------------------------------------------------ */

/* cJSON 1.7.19 accepts a superset of JSON (BOM, trailing data, raw control
 * bytes, duplicate keys, NUL escapes). Reject that superset before the
 * schema-shaped validation so the loader's view matches config_schema.py. */
#define IEMS_CONFIG_MAX_JSON_DEPTH 32u

static int _utf8_valid(const unsigned char *s, size_t len) {
    size_t i = 0;
    while (i < len) {
        unsigned char c = s[i];
        if (c < 0x80) {
            i++;
            continue;
        }
        int extra;
        uint32_t cp;
        if ((c & 0xE0) == 0xC0) {
            extra = 1;
            cp = c & 0x1F;
            if (cp < 2) return 0; /* overlong */
        } else if ((c & 0xF0) == 0xE0) {
            extra = 2;
            cp = c & 0x0F;
        } else if ((c & 0xF8) == 0xF0) {
            extra = 3;
            cp = c & 0x07;
        } else {
            return 0;
        }
        if (i + (size_t)extra >= len) return 0;
        for (int j = 1; j <= extra; j++) {
            if ((s[i + (size_t)j] & 0xC0) != 0x80) return 0;
            cp = (cp << 6) | (s[i + (size_t)j] & 0x3F);
        }
        if ((extra == 2 && cp < 0x800) || (extra == 3 && cp < 0x10000) ||
            (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
            return 0;
        }
        i += (size_t)extra + 1;
    }
    return 1;
}

/* Skip escaped backslashes so the literal text \\u0000 is not mistaken for a
 * NUL escape. */
static int _has_nul_escape(const char *data, size_t len) {
    for (size_t i = 0; i + 1 < len; i++) {
        if (data[i] != '\\') continue;
        if (data[i + 1] == '\\') {
            i++;
            continue;
        }
        if (data[i + 1] != 'u' || i + 5 >= len) continue;
        if (data[i + 2] == '0' && data[i + 3] == '0' && data[i + 4] == '0' &&
            data[i + 5] == '0') {
            return 1;
        }
        i++; /* skip the escaped character */
    }
    return 0;
}

static int _check_text(const char *text, const char *path, char *err, size_t err_size) {
    size_t len = strlen(text);
    for (size_t i = 0; i < len; i++) {
        if ((unsigned char)text[i] < 0x20) {
            return iems_config_fail(err, err_size, "CONTROL_CHARACTER", path, NULL);
        }
    }
    if (!_utf8_valid((const unsigned char *)text, len)) {
        return iems_config_fail(err, err_size, "INVALID_JSON", path, "invalid UTF-8 after decode");
    }
    return 0;
}

static int _check_structure(cJSON *node, const char *path, unsigned depth, char *err,
                            size_t err_size) {
    if (depth > IEMS_CONFIG_MAX_JSON_DEPTH) {
        return iems_config_fail(err, err_size, "JSON_TOO_DEEP", path, NULL);
    }
    if (cJSON_IsObject(node)) {
        cJSON *a;
        cJSON_ArrayForEach(a, node) {
            cJSON *b;
            for (b = node->child; b && b != a; b = b->next) {
                if (strcmp(a->string, b->string) == 0) {
                    char field[192];
                    iems_config_path_key(field, sizeof(field), path, a->string);
                    return iems_config_fail(err, err_size, "DUPLICATE_KEY", field, NULL);
                }
            }
        }
        cJSON_ArrayForEach(a, node) {
            char child_path[192];
            iems_config_path_key(child_path, sizeof(child_path), path, a->string);
            if (_check_text(a->string, child_path, err, err_size) != 0) return -1;
            if (_check_structure(a, child_path, depth + 1, err, err_size) != 0) return -1;
        }
        return 0;
    }
    if (cJSON_IsArray(node)) {
        size_t index = 0;
        cJSON *item;
        cJSON_ArrayForEach(item, node) {
            char child_path[192];
            iems_config_path_index(child_path, sizeof(child_path), path, index++);
            if (_check_structure(item, child_path, depth + 1, err, err_size) != 0) return -1;
        }
        return 0;
    }
    if (cJSON_IsString(node)) return _check_text(node->valuestring, path, err, err_size);
    return 0;
}

int iems_config_strict_parse(const char *data, size_t len, const char *rel, cJSON **out, char *err,
                         size_t err_size) {
    *out = NULL;
    const unsigned char *raw = (const unsigned char *)data;
    if (len >= 3 && raw[0] == 0xEF && raw[1] == 0xBB && raw[2] == 0xBF) {
        return iems_config_fail(err, err_size, "INVALID_JSON", rel, "UTF-8 BOM is not accepted");
    }
    for (size_t i = 0; i < len; i++) {
        if (raw[i] == '\0') {
            return iems_config_fail(err, err_size, "INVALID_JSON", rel, "raw NUL byte is not accepted");
        }
    }
    if (!_utf8_valid(raw, len)) {
        return iems_config_fail(err, err_size, "INVALID_JSON", rel, "invalid UTF-8");
    }
    if (_has_nul_escape(data, len)) {
        return iems_config_fail(err, err_size, "INVALID_JSON", rel,
                     "\\u0000 NUL escape is not accepted");
    }

    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithOpts(data, &parse_end, 1);
    if (!root) {
        return iems_config_fail(err, err_size, "INVALID_JSON", rel,
                     "parse failed or trailing data after the document");
    }
    if (_check_structure(root, rel, 0, err, err_size) != 0) {
        cJSON_Delete(root);
        return -1;
    }
    *out = root;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Ports                                                              */
/* ------------------------------------------------------------------ */

static int _hex_token(const char *value) {
    if (!value || value[0] != '0' || (value[1] != 'x' && value[1] != 'X') || value[2] == '\0') {
        return 0;
    }
    for (const char *p = value + 2; *p; p++) {
        if (!isxdigit((unsigned char)*p)) return 0;
    }
    return 1;
}

static int _add_port(iems_config_model_t *model, const iems_config_port_t *port, const char *path,
                     char *err, size_t err_size) {
    for (size_t i = 0; i < model->port_count; i++) {
        if (port->name[0] && strcmp(model->ports[i].name, port->name) == 0) {
            return iems_config_fail(err, err_size, "DUPLICATE_PORT", path, port->name);
        }
    }
    if (model->port_count >= IEMS_CONFIG_MAX_PORTS) {
        return iems_config_fail(err, err_size, "TOO_MANY_PORTS", path, NULL);
    }
    model->ports[model->port_count++] = *port;
    return 0;
}

static int _parse_filter(cJSON *filter, const char *path, uint16_t *count, char *err,
                         size_t err_size) {
    if (!cJSON_IsArray(filter)) return iems_config_fail(err, err_size, "INVALID_TYPE", path, "expected array");
    int size = cJSON_GetArraySize(filter);
    if (size < 0 || size > UINT16_MAX) {
        return iems_config_fail(err, err_size, "INVALID_VALUE", path, "filter list too large");
    }
    size_t index = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, filter) {
        char item_path[192];
        iems_config_path_index(item_path, sizeof(item_path), path, index++);
        if (!cJSON_IsObject(item)) {
            return iems_config_fail(err, err_size, "INVALID_TYPE", item_path, "expected object");
        }
        if (iems_config_sensitive_scan(item, item_path, err, err_size) != 0 ||
            iems_config_check_keys(item, item_path, k_filter_keys,
                        sizeof(k_filter_keys) / sizeof(k_filter_keys[0]), err, err_size) != 0) {
            return -1;
        }
        const char *id = NULL;
        const char *mask = NULL;
        if (_get_string(item, "id", 1, 1, item_path, err, err_size, &id) != 0 ||
            _get_string(item, "mask", 1, 1, item_path, err, err_size, &mask) != 0) {
            return -1;
        }
        if (!_hex_token(id) || !_hex_token(mask)) {
            return iems_config_fail(err, err_size, "INVALID_VALUE", item_path, "id/mask must be 0x hex");
        }
    }
    *count = (uint16_t)size;
    return 0;
}

static int _parse_uart(cJSON *attr, const char *path, iems_config_port_t *port, char *err,
                       size_t err_size) {
    port->kind = IEMS_CONFIG_PORT_UART;
    const char *name = NULL, *parity = NULL;
    int baud = 0, data_bit = 0, stop_bit = 0;
    if (_get_string(attr, "name", 1, 1, path, err, err_size, &name) != 0 ||
        iems_config_get_int(attr, "baud", 1, 1, path, err, err_size, &baud) != 0 ||
        iems_config_get_int(attr, "data_bit", 1, 1, path, err, err_size, &data_bit) != 0 ||
        iems_config_get_int(attr, "stop_bit", 1, 1, path, err, err_size, &stop_bit) != 0 ||
        _get_string(attr, "parity", 1, 1, path, err, err_size, &parity) != 0) {
        return -1;
    }
    if (!isalpha((unsigned char)parity[0]) || parity[1] != '\0') {
        return iems_config_fail(err, err_size, "INVALID_VALUE", path, "parity must be one letter");
    }
    if (_copy_field(port->name, sizeof(port->name), name, path, err, err_size) != 0) return -1;
    port->baud = (uint32_t)baud;
    port->data_bit = (uint16_t)data_bit;
    port->stop_bit = (uint16_t)stop_bit;
    port->parity = parity[0];
    return 0;
}

static int _parse_can(cJSON *attr, const char *path, iems_config_port_t *port, char *err,
                      size_t err_size) {
    port->kind = IEMS_CONFIG_PORT_CAN;
    const char *name = NULL;
    int baud = 0;
    if (_get_string(attr, "name", 1, 1, path, err, err_size, &name) != 0 ||
        iems_config_get_int(attr, "baud", 1, 1, path, err, err_size, &baud) != 0) {
        return -1;
    }
    cJSON *filter = NULL;
    if (_get_array(attr, "filter", 1, path, err, err_size, &filter) != 0) return -1;
    char filter_path[192];
    iems_config_path_key(filter_path, sizeof(filter_path), path, "filter");
    if (_parse_filter(filter, filter_path, &port->filter_count, err, err_size) != 0) return -1;
    if (_copy_field(port->name, sizeof(port->name), name, path, err, err_size) != 0) return -1;
    port->baud = (uint32_t)baud;
    return 0;
}

static int _parse_lan(cJSON *attr, const char *path, iems_config_port_t *port, char *err,
                      size_t err_size) {
    port->kind = IEMS_CONFIG_PORT_LAN;
    const char *name = NULL, *type = NULL, *ip = NULL, *mask = NULL;
    if (_get_string(attr, "name", 0, 0, path, err, err_size, &name) != 0 ||
        _get_string(attr, "type", 0, 0, path, err, err_size, &type) != 0 ||
        _get_string(attr, "ip", 1, 1, path, err, err_size, &ip) != 0 ||
        _get_string(attr, "mask", 0, 0, path, err, err_size, &mask) != 0) {
        return -1;
    }
    if ((name && _copy_field(port->name, sizeof(port->name), name, path, err, err_size) != 0) ||
        (mask && _copy_field(port->mask, sizeof(port->mask), mask, path, err, err_size) != 0) ||
        (type && _copy_field(port->type, sizeof(port->type), type, path, err, err_size) != 0) ||
        _copy_field(port->ip, sizeof(port->ip), ip, path, err, err_size) != 0) {
        return -1;
    }
    return 0;
}

static int _parse_wan(cJSON *attr, const char *path, iems_config_port_t *port, char *err,
                      size_t err_size) {
    port->kind = IEMS_CONFIG_PORT_WAN;
    const char *name = NULL, *type = NULL, *ip = NULL, *gateway = NULL, *mask = NULL, *metric = NULL;
    if (_get_string(attr, "name", 0, 0, path, err, err_size, &name) != 0 ||
        _get_string(attr, "type", 0, 0, path, err, err_size, &type) != 0 ||
        _get_string(attr, "ip", 0, 0, path, err, err_size, &ip) != 0 ||
        _get_string(attr, "gateway", 0, 0, path, err, err_size, &gateway) != 0 ||
        _get_string(attr, "mask", 0, 0, path, err, err_size, &mask) != 0 ||
        _get_string(attr, "metric", 0, 0, path, err, err_size, &metric) != 0) {
        return -1;
    }
    if (type && strcmp(type, "static") != 0 && strcmp(type, "dhcp") != 0) {
        return iems_config_fail(err, err_size, "INVALID_VALUE", path, "wan type must be static or dhcp");
    }
    struct {
        char *dst;
        size_t cap;
        const char *src;
    } fields[] = {
        {port->name, sizeof(port->name), name},       {port->type, sizeof(port->type), type},
        {port->ip, sizeof(port->ip), ip},             {port->gateway, sizeof(port->gateway), gateway},
        {port->mask, sizeof(port->mask), mask},       {port->metric, sizeof(port->metric), metric},
    };
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        if (fields[i].src &&
            _copy_field(fields[i].dst, fields[i].cap, fields[i].src, path, err, err_size) != 0) {
            return -1;
        }
    }
    return 0;
}

static int _parse_di(cJSON *attr, const char *path, iems_config_port_t *port, char *err,
                     size_t err_size) {
    port->kind = IEMS_CONFIG_PORT_DI;
    const char *name = NULL, *disp_name = NULL;
    int ncno = 0;
    if (_get_string(attr, "name", 1, 1, path, err, err_size, &name) != 0 ||
        _get_string(attr, "disp_name", 1, 0, path, err, err_size, &disp_name) != 0 ||
        iems_config_get_int(attr, "ncno", 0, INT_MIN, path, err, err_size, &ncno) != 0) {
        return -1;
    }
    if (_copy_field(port->name, sizeof(port->name), name, path, err, err_size) != 0 ||
        _copy_field(port->disp_name, sizeof(port->disp_name), disp_name, path, err, err_size) != 0) {
        return -1;
    }
    port->ncno = ncno;
    return 0;
}

static int _parse_do(cJSON *attr, const char *path, iems_config_port_t *port, char *err,
                     size_t err_size) {
    port->kind = IEMS_CONFIG_PORT_DO;
    const char *name = NULL, *disp_name = NULL;
    if (_get_string(attr, "name", 1, 1, path, err, err_size, &name) != 0 ||
        _get_string(attr, "disp_name", 1, 0, path, err, err_size, &disp_name) != 0) {
        return -1;
    }
    if (_copy_field(port->name, sizeof(port->name), name, path, err, err_size) != 0 ||
        _copy_field(port->disp_name, sizeof(port->disp_name), disp_name, path, err, err_size) != 0) {
        return -1;
    }
    return 0;
}

typedef int (*port_parser_fn_t)(cJSON *attr, const char *path, iems_config_port_t *port, char *err,
                                size_t err_size);

typedef struct {
    const char *section;
    const char *const *keys;
    size_t key_count;
    port_parser_fn_t parse;
} port_section_t;

static int _parse_port_section(cJSON *dev_cfg, const port_section_t *def, const char *root_path,
                               iems_config_model_t *model, char *err, size_t err_size) {
    cJSON *section = cJSON_GetObjectItemCaseSensitive(dev_cfg, def->section);
    if (!section) return 0;

    char section_path[192];
    iems_config_path_key(section_path, sizeof(section_path), root_path, def->section);
    if (!cJSON_IsObject(section)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", section_path, "expected object");
    }
    if (iems_config_sensitive_scan(section, section_path, err, err_size) != 0 ||
        iems_config_check_keys(section, section_path, (const char *const[]){"attr"}, 1, err, err_size) != 0) {
        return -1;
    }
    cJSON *attrs = NULL;
    if (_get_array(section, "attr", 1, section_path, err, err_size, &attrs) != 0) return -1;

    char attr_path[192];
    iems_config_path_key(attr_path, sizeof(attr_path), section_path, "attr");
    size_t index = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, attrs) {
        char item_path[192];
        iems_config_path_index(item_path, sizeof(item_path), attr_path, index++);
        if (!cJSON_IsObject(item)) {
            return iems_config_fail(err, err_size, "INVALID_TYPE", item_path, "expected object");
        }
        if (iems_config_sensitive_scan(item, item_path, err, err_size) != 0 ||
            iems_config_check_keys(item, item_path, def->keys, def->key_count, err, err_size) != 0) {
            return -1;
        }
        iems_config_port_t port;
        memset(&port, 0, sizeof(port));
        if (def->parse(item, item_path, &port, err, err_size) != 0) return -1;
        if (_add_port(model, &port, item_path, err, err_size) != 0) return -1;
    }
    return 0;
}

static int _parse_port_document(cJSON *root, const char *rel, iems_config_model_t *model, char *err,
                                size_t err_size) {
    if (iems_config_check_root(root, rel, err, err_size) != 0) return -1;
    if (iems_config_sensitive_scan(root, rel, err, err_size) != 0) return -1;
    if (iems_config_check_keys(root, rel, k_port_root_keys,
                    sizeof(k_port_root_keys) / sizeof(k_port_root_keys[0]), err, err_size) != 0) {
        return -1;
    }
    const char *ver = NULL;
    if (_get_string(root, "ver", 0, 0, rel, err, err_size, &ver) != 0) return -1;

    cJSON *dev_cfg = NULL;
    char dev_cfg_path[192];
    iems_config_path_key(dev_cfg_path, sizeof(dev_cfg_path), rel, "dev_cfg");
    if (_get_object(root, "dev_cfg", 1, rel, err, err_size, &dev_cfg) != 0 ||
        iems_config_sensitive_scan(dev_cfg, dev_cfg_path, err, err_size) != 0 ||
        iems_config_check_keys(dev_cfg, dev_cfg_path, k_dev_cfg_keys,
                    sizeof(k_dev_cfg_keys) / sizeof(k_dev_cfg_keys[0]), err, err_size) != 0) {
        return -1;
    }
    if (!cJSON_GetObjectItemCaseSensitive(dev_cfg, "lan")) {
        char lan_path[192];
        iems_config_path_key(lan_path, sizeof(lan_path), dev_cfg_path, "lan");
        return iems_config_fail(err, err_size, "MISSING_REQUIRED", lan_path, NULL);
    }

    static const port_section_t sections[] = {
        {"uart", k_uart_keys, sizeof(k_uart_keys) / sizeof(k_uart_keys[0]), _parse_uart},
        {"can", k_can_keys, sizeof(k_can_keys) / sizeof(k_can_keys[0]), _parse_can},
        {"lan", k_lan_keys, sizeof(k_lan_keys) / sizeof(k_lan_keys[0]), _parse_lan},
        {"wan", k_wan_keys, sizeof(k_wan_keys) / sizeof(k_wan_keys[0]), _parse_wan},
        {"di", k_di_keys, sizeof(k_di_keys) / sizeof(k_di_keys[0]), _parse_di},
        {"do", k_do_keys, sizeof(k_do_keys) / sizeof(k_do_keys[0]), _parse_do},
    };
    for (size_t i = 0; i < sizeof(sections) / sizeof(sections[0]); i++) {
        if (_parse_port_section(dev_cfg, &sections[i], dev_cfg_path, model, err, err_size) != 0) {
            return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Devices                                                            */
/* ------------------------------------------------------------------ */

static const iems_config_port_t *_find_port(const iems_config_model_t *model, const char *name) {
    for (size_t i = 0; i < model->port_count; i++) {
        if (model->ports[i].name[0] && strcmp(model->ports[i].name, name) == 0) return &model->ports[i];
    }
    return NULL;
}

static int _is_endpoint(const char *channel) {
    if (!channel || !channel[0]) return 0;
    const char *colon = strchr(channel, ':');
    if (!colon || colon == channel || colon[1] == '\0') return 0;
    for (const char *p = colon + 1; *p; p++) {
        if (!isdigit((unsigned char)*p)) return 0;
    }
    for (const char *p = channel; p < colon; p++) {
        if (!(isalnum((unsigned char)*p) || *p == '_' || *p == '.' || *p == '-')) return 0;
    }
    return 1;
}

/* 一张协议能力表同时管 R4（从站地址宽度）与 R5（协议/总线匹配）。
 * addr 的 0/空值 -> 1 归一在 _parse_addr 统一做；这里只给上限。
 * addr_max = 65535 表示协议不消费从站地址（DLT645 DI、YDT OAD、CAN ID、
 * IEC104 IOA），保留字段但不参与驱动寻址。 */
typedef enum {
    IEMS_CFG_TRANSPORT_UART = 0,
    IEMS_CFG_TRANSPORT_CAN,
    IEMS_CFG_TRANSPORT_TCP,
} iems_config_transport_t;

typedef struct {
    const char *protocol;
    iems_config_transport_t transport;
    int addr_max;
    const char *addr_note;
} iems_config_protocol_caps_t;

static const iems_config_protocol_caps_t k_protocol_caps[] = {
    {"ModbusRTU", IEMS_CFG_TRANSPORT_UART, 247, "modbus RTU slave address is 1..247"},
    {"ModbusRTUHiCRC", IEMS_CFG_TRANSPORT_UART, 247, "modbus RTU slave address is 1..247"},
    {"ModbusTCP", IEMS_CFG_TRANSPORT_TCP, 255, "modbus TCP unit id is 1..255"},
    {"DLT645", IEMS_CFG_TRANSPORT_UART, 65535, "addr retained; driver uses DI"},
    {"YDT1363", IEMS_CFG_TRANSPORT_UART, 65535, "addr retained; driver uses OAD"},
    {"CAN", IEMS_CFG_TRANSPORT_CAN, 65535, "addr retained; driver uses CAN ID"},
    {"IEC104", IEMS_CFG_TRANSPORT_TCP, 65535, "addr retained; driver uses IOA"},
};

static const iems_config_protocol_caps_t *_protocol_caps(const char *protocol) {
    for (size_t i = 0; i < sizeof(k_protocol_caps) / sizeof(k_protocol_caps[0]); i++) {
        if (strcmp(protocol, k_protocol_caps[i].protocol) == 0) return &k_protocol_caps[i];
    }
    return NULL;
}

static int _protocol_on_bus_allowed(const char *protocol, iems_config_port_kind_t kind) {
    const iems_config_protocol_caps_t *caps = _protocol_caps(protocol);
    if (!caps) return 0;
    if (caps->transport == IEMS_CFG_TRANSPORT_UART) return kind == IEMS_CONFIG_PORT_UART;
    if (caps->transport == IEMS_CFG_TRANSPORT_CAN) return kind == IEMS_CONFIG_PORT_CAN;
    return 0; /* TCP endpoints are not mapped to a local bus */
}

static int _resolve_bus(const iems_config_model_t *model, iems_config_device_t *device,
                        const char *device_path, char *err, size_t err_size) {
    char path[192];
    iems_config_path_key(path, sizeof(path), device_path, "channel");

    if (_is_endpoint(device->channel)) {
        device->bus_id = -1;
        LOG_WARN(LOG_CAT_CORE,
                 "IEMS config: device %s (%s) channel %s has no local bus, loaded UNCHECKED",
                 device->no, device->name, device->channel);
        return 0;
    }

    const iems_config_port_t *port = _find_port(model, device->channel);
    if (!port) {
        return iems_config_fail(err, err_size, "BAD_REFERENCE", path,
                     "channel is neither a declared port nor host:port");
    }

    device->bus_id = -1;
    for (size_t i = 0; i < sizeof(k_channel_bus) / sizeof(k_channel_bus[0]); i++) {
        if (strcmp(device->channel, k_channel_bus[i].name) == 0 &&
            port->kind == k_channel_bus[i].kind) {
            device->bus_id = k_channel_bus[i].bus_id;
            break;
        }
    }
    if (device->bus_id >= 0 && !_protocol_on_bus_allowed(device->protocol, port->kind)) {
        return iems_config_fail(err, err_size, "PROTOCOL_TRANSPORT_MISMATCH", path,
                     "protocol cannot run on this bus transport");
    }
    if (device->bus_id < 0) {
        LOG_WARN(LOG_CAT_CORE,
                 "IEMS config: device %s (%s) channel %s has no provisioned SOUTHBOUND bus, "
                 "loaded UNCHECKED",
                 device->no, device->name, device->channel);
    }
    return 0;
}

static int _parse_addr(const char *value, int *out, const char *path, char *err, size_t err_size) {
    if (!value || value[0] == '\0') {
        *out = 1; /* 与 iems_poll_task_route 的 0 -> 1 归一一致 */
        return 0;
    }
    errno = 0;
    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < 0 || parsed > 65535) {
        return iems_config_fail(err, err_size, "INVALID_ADDR", path, "expected integer 0..65535");
    }
    *out = (parsed == 0) ? 1 : (int)parsed;
    return 0;
}

/* 驱动把从站地址截成 uint8：RTU 协议 248..255 会上线成从站 0（广播），
 * 300 -> 44、65535 -> 255。地址上限必须按协议卡死，不能只存 int。 */
static int _validate_addr_range(const iems_config_device_t *device, const char *path, char *err,
                                size_t err_size) {
    const iems_config_protocol_caps_t *caps = _protocol_caps(device->protocol);
    if (!caps) {
        return iems_config_fail(err, err_size, "UNSUPPORTED_PROTOCOL", path, device->protocol);
    }
    if (device->addr > caps->addr_max) {
        return iems_config_fail(err, err_size, "ADDR_OUT_OF_RANGE", path, caps->addr_note);
    }
    return 0;
}

typedef struct {
    iems_config_model_t *model;
} device_parse_ctx_t;

static int _parse_device_entry(cJSON *dev, const char *class_type, const char *class_path,
                               size_t class_index, size_t device_index, const char *rel,
                               iems_config_model_t *model, char *err, size_t err_size) {
    char path[192];
    iems_config_path_index(path, sizeof(path), class_path, device_index);
    if (!cJSON_IsObject(dev)) return iems_config_fail(err, err_size, "INVALID_TYPE", path, "expected object");
    if (iems_config_sensitive_scan(dev, path, err, err_size) != 0 ||
        iems_config_check_keys(dev, path, k_dev_keys, sizeof(k_dev_keys) / sizeof(k_dev_keys[0]), err,
                    err_size) != 0) {
        return -1;
    }
    if (model->device_count >= IEMS_CONFIG_MAX_DEVICES) {
        return iems_config_fail(err, err_size, "TOO_MANY_DEVICES", path, NULL);
    }

    const char *no = NULL, *name = NULL, *channel = NULL, *template_name = NULL;
    const char *protocol = NULL, *user_def = NULL, *device_type = NULL, *addr = NULL;
    if (_get_string(dev, "no", 1, 1, path, err, err_size, &no) != 0 ||
        _get_string(dev, "name", 1, 0, path, err, err_size, &name) != 0 ||
        _get_string(dev, "channel", 1, 1, path, err, err_size, &channel) != 0 ||
        _get_string(dev, "template", 1, 1, path, err, err_size, &template_name) != 0 ||
        _get_string(dev, "protocol", 1, 1, path, err, err_size, &protocol) != 0 ||
        _get_string(dev, "user_def", 0, 0, path, err, err_size, &user_def) != 0 ||
        _get_string(dev, "type", 0, 0, path, err, err_size, &device_type) != 0 ||
        _get_string(dev, "addr", 0, 0, path, err, err_size, &addr) != 0) {
        return -1;
    }
    if (iems_config_get_int(dev, "period", 0, 0, path, err, err_size, NULL) != 0) return -1;

    iems_config_device_t device;
    memset(&device, 0, sizeof(device));
    device.device_id = (uint16_t)(model->device_count + 1);
    device.class_index = (uint16_t)class_index;
    device.device_index = (uint16_t)device_index;
    device.bus_id = -1;

    char protocol_path[192], addr_path[192];
    iems_config_path_key(protocol_path, sizeof(protocol_path), path, "protocol");
    iems_config_path_key(addr_path, sizeof(addr_path), path, "addr");
    if (iems_driver_protocol_from_config(protocol, device.protocol) != 0) {
        return iems_config_fail(err, err_size, "UNSUPPORTED_PROTOCOL", protocol_path, protocol);
    }
    if (_parse_addr(addr, &device.addr, addr_path, err, err_size) != 0) return -1;
    if (_validate_addr_range(&device, addr_path, err, err_size) != 0) return -1;

    if (_copy_field(device.class_type, sizeof(device.class_type), class_type, path, err,
                    err_size) != 0 ||
        _copy_field(device.no, sizeof(device.no), no, path, err, err_size) != 0 ||
        _copy_field(device.name, sizeof(device.name), name, path, err, err_size) != 0 ||
        _copy_field(device.channel, sizeof(device.channel), channel, path, err, err_size) != 0 ||
        _copy_field(device.template_name, sizeof(device.template_name), template_name, path, err,
                    err_size) != 0 ||
        _copy_field(device.config_protocol, sizeof(device.config_protocol), protocol, path, err,
                    err_size) != 0 ||
        _copy_field(device.source_path, sizeof(device.source_path), rel, path, err, err_size) != 0) {
        return -1;
    }
    if (user_def &&
        _copy_field(device.user_def, sizeof(device.user_def), user_def, path, err, err_size) != 0) {
        return -1;
    }
    if (device_type &&
        _copy_field(device.device_type, sizeof(device.device_type), device_type, path, err,
                    err_size) != 0) {
        return -1;
    }
    if (_resolve_bus(model, &device, path, err, err_size) != 0) return -1;

    model->devices[model->device_count++] = device;
    return 0;
}

static int _parse_device_document(cJSON *root, const char *rel, iems_config_model_t *model,
                                  char *err, size_t err_size) {
    if (iems_config_check_root(root, rel, err, err_size) != 0) return -1;
    if (iems_config_sensitive_scan(root, rel, err, err_size) != 0) return -1;
    if (iems_config_check_keys(root, rel, k_device_root_keys,
                    sizeof(k_device_root_keys) / sizeof(k_device_root_keys[0]), err, err_size) !=
        0) {
        return -1;
    }
    cJSON *classes = NULL;
    if (_get_array(root, "dev_class", 1, rel, err, err_size, &classes) != 0) return -1;

    size_t class_index = 0;
    cJSON *class_item;
    cJSON_ArrayForEach(class_item, classes) {
        char class_path[192];
        iems_config_path_index(class_path, sizeof(class_path), rel, class_index);
        if (!cJSON_IsObject(class_item)) {
            return iems_config_fail(err, err_size, "INVALID_TYPE", class_path, "expected object");
        }
        if (iems_config_sensitive_scan(class_item, class_path, err, err_size) != 0 ||
            iems_config_check_keys(class_item, class_path, k_class_keys,
                        sizeof(k_class_keys) / sizeof(k_class_keys[0]), err, err_size) != 0) {
            return -1;
        }
        const char *class_type = NULL;
        if (_get_string(class_item, "type", 1, 1, class_path, err, err_size, &class_type) != 0) {
            return -1;
        }
        cJSON *devs = NULL;
        if (_get_array(class_item, "devs", 0, class_path, err, err_size, &devs) != 0) return -1;
        if (!devs) {
            class_index++;
            continue;
        }
        char devs_path[192];
        iems_config_path_key(devs_path, sizeof(devs_path), class_path, "devs");
        size_t device_index = 0;
        cJSON *dev_item;
        cJSON_ArrayForEach(dev_item, devs) {
            if (_parse_device_entry(dev_item, class_type, devs_path, class_index, device_index, rel,
                                    model, err, err_size) != 0) {
                return -1;
            }
            device_index++;
        }
        class_index++;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Points                                                             */
/* ------------------------------------------------------------------ */

static int _protocol_family(const char *protocol) {
    if (!protocol) return -1;
    if (strcasecmp(protocol, "ModbusRTU") == 0 || strcasecmp(protocol, "ModbusRTUHiCRC") == 0 ||
        strcasecmp(protocol, "ModbusTCP") == 0 || strcasecmp(protocol, "Modbus") == 0) {
        return 0;
    }
    if (strcasecmp(protocol, "DLT645") == 0) return 1;
    if (strcasecmp(protocol, "CAN") == 0) return 2;
    if (strcasecmp(protocol, "IEC104") == 0) return 3;
    if (strcasecmp(protocol, "YDT1363") == 0) return 4;
    return -1;
}

static int _protocol_has_point(const char *protocol, uint32_t tag_id) {
    switch (_protocol_family(protocol)) {
        case 0: return iems_lookup_modbus_by_tag(tag_id) != NULL;
        case 1: return iems_lookup_dlt645_by_tag(tag_id) != NULL;
        case 2: return iems_lookup_can_by_tag(tag_id) != NULL;
        case 3: return iems_lookup_iec104_by_tag(tag_id) != NULL;
        case 4: return 0; /* YDT1363 还没有产品点表 */
        default: return -1;
    }
}

int iems_config_catalog_static(void *ctx, const char *protocol, iems_config_point_t *out,
                               size_t cap) {
    (void)ctx;
    if (!protocol || !out) return -1;
    if (_protocol_family(protocol) < 0) return -1;

    uint32_t meta_count = 0;
    const edge_tag_meta_t *meta = iems_get_tag_metadata(&meta_count);
    if (!meta) return -1;

    size_t count = 0;
    for (uint32_t i = 0; i < meta_count; i++) {
        int known = _protocol_has_point(protocol, meta[i].id);
        if (known < 0) return -1;
        if (!known) continue;
        uint32_t dense = 0;
        if (iems_point_slot(meta[i].id, &dense) != 0) return -1;
        if (count >= cap) return -1;
        out[count].tag_id = meta[i].id;
        out[count].name = meta[i].name;
        out[count].dense_idx = (uint16_t)dense;
        out[count].native_id = iems_driver_adapter_native_id(protocol, meta[i].id);
        out[count].scale = iems_driver_adapter_tag_scale(protocol, meta[i].id);
        count++;
    }
    return (int)count;
}

static const iems_config_point_catalog_t k_static_catalog = {
    .list_points = iems_config_catalog_static,
    .ctx = NULL,
};

const iems_config_point_catalog_t *iems_config_static_catalog(void) {
    return &k_static_catalog;
}

static int _attach_points(iems_config_model_t *model, const iems_config_point_catalog_t *catalog,
                          char *err, size_t err_size) {
    if (!catalog) return 0;
    for (size_t d = 0; d < model->device_count; d++) {
        iems_config_device_t *device = &model->devices[d];
        if (device->bus_id < 0) continue;

        iems_config_point_t points[IEMS_CONFIG_MAX_POINTS_PER_DEVICE];
        memset(points, 0, sizeof(points));
        int count = catalog->list_points(catalog->ctx, device->protocol, points,
                                         IEMS_CONFIG_MAX_POINTS_PER_DEVICE);
        if (count < 0) {
            return iems_config_fail(err, err_size, "POINT_CATALOG_FAILED", device->source_path,
                         device->protocol);
        }
        if ((size_t)count > IEMS_CONFIG_MAX_POINTS_PER_DEVICE) {
            return iems_config_fail(err, err_size, "POINT_CAPACITY", device->source_path, device->protocol);
        }
        for (int p = 0; p < count; p++) {
            if (points[p].tag_id == IEMS_POINT_TAG_NONE || !points[p].name ||
                points[p].dense_idx >= IEMS_POINT_DENSE_PER_DEVICE) {
                return iems_config_fail(err, err_size, "INVALID_POINT_CATALOG", device->source_path,
                             device->protocol);
            }
            for (int q = 0; q < p; q++) {
                if (points[q].tag_id == points[p].tag_id ||
                    points[q].dense_idx == points[p].dense_idx) {
                    return iems_config_fail(err, err_size, "INVALID_POINT_CATALOG", device->source_path,
                                 "duplicate tag or dense index");
                }
            }
        }
        memcpy(device->points, points, (size_t)count * sizeof(points[0]));
        device->point_count = (uint16_t)count;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Load                                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    char names[IEMS_CONFIG_MAX_LC_ENTRIES][64];
    size_t count;
    int overflow;
} lc_entries_t;

static int _collect_lc_entry(void *ctx, const char *name) {
    lc_entries_t *entries = (lc_entries_t *)ctx;
    if (strlen(name) >= sizeof(entries->names[0]) || entries->count >= IEMS_CONFIG_MAX_LC_ENTRIES) {
        entries->overflow = 1;
        return 1;
    }
    snprintf(entries->names[entries->count], sizeof(entries->names[0]), "%s", name);
    entries->count++;
    return 0;
}

static int _name_cmp(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

static int _load_one(const iems_config_source_t *source, const char *rel, int required,
                     iems_config_model_t *model, int is_port, char *err, size_t err_size) {
    char *data = NULL;
    size_t len = 0;
    iems_config_source_rc_t read_rc = source->read_file(source->ctx, rel, &data, &len, err, err_size);
    if (read_rc == IEMS_CONFIG_SOURCE_ABSENT) {
        if (!required) return 0;
        return iems_config_fail(err, err_size, "READ_FAILED", rel, "required file is missing");
    }
    if (read_rc != IEMS_CONFIG_SOURCE_OK) {
        if (err && err[0]) return -1;
        return iems_config_fail(err, err_size, "READ_FAILED", rel, "cannot read file");
    }
    if (!data || data[len] != '\0') {
        free(data);
        return iems_config_fail(err, err_size, "INVALID_JSON", rel, "source did not NUL-terminate");
    }
    cJSON *root = NULL;
    int parse_rc = iems_config_strict_parse(data, len, rel, &root, err, err_size);
    free(data);
    if (parse_rc != 0) return -1;
    int rc = is_port ? _parse_port_document(root, rel, model, err, err_size)
                     : _parse_device_document(root, rel, model, err, err_size);
    cJSON_Delete(root);
    return rc;
}

int iems_config_load(const iems_config_source_t *source,
                     const iems_config_point_catalog_t *catalog, iems_config_model_t *out,
                     char *err, size_t err_size) {
    if (err && err_size) err[0] = '\0';
    if (!source || !source->read_file || !out) {
        return iems_config_fail(err, err_size, "INVALID_ARGUMENT", "$", "source and output are required");
    }
    memset(out, 0, sizeof(*out));

    /* 端口先加载：设备 channel 的符号引用要靠它校验。 */
    if (_load_one(source, IEMS_CONFIG_PORT_FILE, 1, out, 1, err, err_size) != 0) {
        return -1;
    }

    static const char *const optional_files[] = {IEMS_CONFIG_DEF_FILE, IEMS_CONFIG_LC_FILE};
    if (_load_one(source, IEMS_CONFIG_DEVICE_FILE, 1, out, 0, err, err_size) != 0) {
        return -1;
    }
    for (size_t i = 0; i < sizeof(optional_files) / sizeof(optional_files[0]); i++) {
        if (_load_one(source, optional_files[i], 0, out, 0, err, err_size) != 0) {
            return -1;
        }
    }

    if (source->list_dir) {
        lc_entries_t entries;
        memset(&entries, 0, sizeof(entries));
        iems_config_source_rc_t listed =
            source->list_dir(source->ctx, "lc_devs", _collect_lc_entry, &entries, err, err_size);
        if (entries.overflow) {
            return iems_config_fail(err, err_size, "TOO_MANY_FILES", "lc_devs", NULL);
        }
        if (listed == IEMS_CONFIG_SOURCE_ERROR) return -1;
        if (listed == IEMS_CONFIG_SOURCE_OK) {
            qsort(entries.names, entries.count, sizeof(entries.names[0]), _name_cmp);
            for (size_t i = 0; i < entries.count; i++) {
                char rel[128];
                int n = snprintf(rel, sizeof(rel), "lc_devs/%s/device_config.json", entries.names[i]);
                if (n <= 0 || (size_t)n >= sizeof(rel)) {
                    return iems_config_fail(err, err_size, "VALUE_TOO_LONG", entries.names[i], NULL);
                }
                if (_load_one(source, rel, 1, out, 0, err, err_size) != 0) {
                    return -1;
                }
            }
        }
    }

    if (out->device_count == 0) {
        return iems_config_fail(err, err_size, "EMPTY_DEVICE_SET", IEMS_CONFIG_DEVICE_FILE,
                     "explicit config dir must describe at least one device");
    }

    if (_attach_points(out, catalog, err, err_size) != 0) return -1;

    size_t unmapped = 0;
    for (size_t i = 0; i < out->device_count; i++) {
        if (out->devices[i].bus_id < 0) unmapped++;
    }
    LOG_INFO(LOG_CAT_CORE, "IEMS config: %zu device(s), %zu port(s), %zu device(s) UNCHECKED",
             out->device_count, out->port_count, unmapped);
    return 0;
}

/* ------------------------------------------------------------------ */
/* SQLite apply                                                       */
/* ------------------------------------------------------------------ */

static void _sql_error(char *err, size_t err_size, const char *where, sqlite3 *db) {
    iems_config_fail(err, err_size, "SQL_FAILED", where, sqlite3_errmsg(db));
}

int iems_config_load_apply(const char *root, sqlite_svc_t *db, char *err, size_t err_size) {
    if (!root || !root[0] || !db) {
        return iems_config_fail(err, err_size, "INVALID_ARGUMENT", "$", "root and db are required");
    }
    iems_config_file_source_t file_source;
    iems_config_file_source_init(&file_source, root);
    iems_config_source_t source = iems_config_file_source_port(&file_source);
    iems_config_model_t *model = (iems_config_model_t *)calloc(1, sizeof(*model));
    if (!model) return iems_config_fail(err, err_size, "OUT_OF_MEMORY", root, NULL);

    int rc = iems_config_load(&source, iems_config_static_catalog(), model, err, err_size);
    if (rc == 0) rc = iems_config_apply_sqlite(model, db, err, err_size);
    if (rc == 0) {
        LOG_INFO(LOG_CAT_CORE, "IEMS config: loaded %zu device(s), %zu port(s)",
                 model->device_count, model->port_count);
    }
    free(model);
    return rc;
}

int iems_config_apply_sqlite(const iems_config_model_t *model, sqlite_svc_t *db_svc, char *err,
                             size_t err_size) {
    if (err && err_size) err[0] = '\0';
    if (!model || !db_svc) {
        return iems_config_fail(err, err_size, "INVALID_ARGUMENT", "$", "model and db are required");
    }
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(db_svc);
    if (!db) return iems_config_fail(err, err_size, "SQL_FAILED", "$", "no database handle");

    /* 生产迁移没有建 poll_tasks；加载器是事实源接线的一部分，补建幂等表。 */
    char *sql_err = NULL;
    if (sqlite3_exec(db,
                     "CREATE TABLE IF NOT EXISTS poll_tasks "
                     "(bus_id INTEGER, device_id INTEGER, tag_id INTEGER);",
                     NULL, NULL, &sql_err) != SQLITE_OK) {
        iems_config_fail(err, err_size, "SQL_FAILED", "create poll_tasks", sql_err);
        if (sql_err) sqlite3_free(sql_err);
        return -1;
    }
    if (sqlite_svc_begin_trans(db_svc) != 0) {
        return iems_config_fail(err, err_size, "SQL_FAILED", "begin transaction", sqlite3_errmsg(db));
    }

    int rc = -1;
    sqlite3_stmt *insert_device = NULL;
    sqlite3_stmt *insert_task = NULL;
    const char *delete_sql = "DELETE FROM inventory; DELETE FROM poll_tasks;";
    if (sqlite3_exec(db, delete_sql, NULL, NULL, &sql_err) != SQLITE_OK) {
        iems_config_fail(err, err_size, "SQL_FAILED", "clear registry", sql_err);
        if (sql_err) sqlite3_free(sql_err);
        goto rollback;
    }
    if (sqlite3_prepare_v2(
            db, "INSERT INTO inventory (id, type, addr, protocol) VALUES (?, ?, ?, ?);", -1,
            &insert_device, NULL) != SQLITE_OK) {
        _sql_error(err, err_size, "prepare inventory", db);
        goto rollback;
    }
    if (sqlite3_prepare_v2(
            db, "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (?, ?, ?);", -1,
            &insert_task, NULL) != SQLITE_OK) {
        _sql_error(err, err_size, "prepare poll_tasks", db);
        goto rollback;
    }

    for (size_t d = 0; d < model->device_count; d++) {
        const iems_config_device_t *device = &model->devices[d];
        char id[16];
        snprintf(id, sizeof(id), "%u", (unsigned)device->device_id);
        sqlite3_bind_text(insert_device, 1, id, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert_device, 2, device->class_type, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(insert_device, 3, device->addr);
        sqlite3_bind_text(insert_device, 4, device->protocol, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(insert_device) != SQLITE_DONE) {
            _sql_error(err, err_size, "insert inventory", db);
            goto rollback;
        }
        sqlite3_reset(insert_device);
        sqlite3_clear_bindings(insert_device);

        if (device->bus_id < 0) continue;
        for (uint16_t p = 0; p < device->point_count; p++) {
            sqlite3_bind_int(insert_task, 1, device->bus_id);
            sqlite3_bind_int(insert_task, 2, (int)device->device_id);
            sqlite3_bind_int(insert_task, 3, (int)device->points[p].tag_id);
            if (sqlite3_step(insert_task) != SQLITE_DONE) {
                _sql_error(err, err_size, "insert poll_tasks", db);
                goto rollback;
            }
            sqlite3_reset(insert_task);
            sqlite3_clear_bindings(insert_task);
        }
    }

    if (sqlite_svc_commit_trans(db_svc) != 0) {
        _sql_error(err, err_size, "commit", db);
        goto rollback;
    }
    rc = 0;

rollback:
    if (insert_task) sqlite3_finalize(insert_task);
    if (insert_device) sqlite3_finalize(insert_device);
    if (rc != 0) sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    return rc;
}

