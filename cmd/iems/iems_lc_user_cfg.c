/**
 * @file iems_lc_user_cfg.c
 * @brief P1-07a: strict loader for converted lc_devs/<LC>/lc_user_cfg.json.
 *
 * Reuses the shared strict JSON boundary helpers from iems_config_loader.c
 * (private header) so the device/port and per-LC trust boundaries stay
 * identical. v3 vlc_cfg.c merges a 125 default for PowerLimit and
 * Installedcab_coeff, but its only call site is commented out; this loader
 * keeps the source values and leaves missing entries missing for the consumer.
 */

#include "iems_lc_user_cfg.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "edge_log.h"
#include "iems_config_loader_priv.h"

#define IEMS_LC_SCHEMA_VERSION "1.0.0"
#define IEMS_LC_DEVS_DIR "lc_devs"

static const char *const k_envelope_keys[] = {"schema_version", "config"};
static const char *const k_int_item_keys[] = {"type", "name", "int_value"};
static const char *const k_double_item_keys[] = {"type", "name", "double_value"};

/* ------------------------------------------------------------------ */
/* Document parsing                                                   */
/* ------------------------------------------------------------------ */

static int _check_envelope(cJSON *root, const char *rel, char *err, size_t err_size) {
    if (!cJSON_IsObject(root)) {
        return iems_config_fail(err, err_size, "INVALID_ROOT", rel,
                                "expected {schema_version, config} object");
    }
    if (iems_config_check_keys(root, rel, k_envelope_keys,
                               sizeof(k_envelope_keys) / sizeof(k_envelope_keys[0]), err,
                               err_size) != 0) {
        return -1;
    }

    char path[192];
    iems_config_path_key(path, sizeof(path), rel, "schema_version");
    cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "schema_version");
    if (!version) return iems_config_fail(err, err_size, "MISSING_SCHEMA_VERSION", path, NULL);
    if (!cJSON_IsString(version)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", path, "expected string");
    }
    if (strcmp(version->valuestring, IEMS_LC_SCHEMA_VERSION) != 0) {
        return iems_config_fail(err, err_size, "UNSUPPORTED_SCHEMA_VERSION", path,
                                version->valuestring);
    }

    iems_config_path_key(path, sizeof(path), rel, "config");
    cJSON *config = cJSON_GetObjectItemCaseSensitive(root, "config");
    if (!config) return iems_config_fail(err, err_size, "MISSING_REQUIRED", path, NULL);
    if (!cJSON_IsArray(config)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", path, "expected array");
    }
    return 0;
}

static int _get_double_value(cJSON *item, const char *path, char *err, size_t err_size,
                             double *out) {
    cJSON *value = cJSON_GetObjectItemCaseSensitive(item, "double_value");
    if (!value) return 0;
    char field[224];
    iems_config_path_key(field, sizeof(field), path, "double_value");
    if (!cJSON_IsNumber(value)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected number");
    }
    if (!isfinite(value->valuedouble)) {
        return iems_config_fail(err, err_size, "INVALID_VALUE", field, "not finite");
    }
    *out = value->valuedouble;
    return 0;
}

/* An entry is either {type:0, name, int_value?} or {type:1, name, double_value?}. */
static int _parse_entry(cJSON *item, const char *item_path, iems_lc_entry_t *out, char *err,
                        size_t err_size) {
    if (!cJSON_IsObject(item)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", item_path, "expected object");
    }

    char field[224];
    iems_config_path_key(field, sizeof(field), item_path, "type");
    cJSON *type = cJSON_GetObjectItemCaseSensitive(item, "type");
    if (!type) return iems_config_fail(err, err_size, "MISSING_REQUIRED", field, NULL);
    if (!cJSON_IsNumber(type)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected integer");
    }
    double raw_type = type->valuedouble;
    if (raw_type != 0.0 && raw_type != 1.0) {
        return iems_config_fail(err, err_size, "INVALID_VALUE", field, "type must be 0 or 1");
    }
    int type_id = (int)raw_type;

    const char *const *allowed = type_id == 0 ? k_int_item_keys : k_double_item_keys;
    _Static_assert(sizeof(k_int_item_keys) / sizeof(k_int_item_keys[0]) ==
                       sizeof(k_double_item_keys) / sizeof(k_double_item_keys[0]),
                   "both item key sets carry type, name and one value key");
    size_t allowed_count = sizeof(k_int_item_keys) / sizeof(k_int_item_keys[0]);
    if (iems_config_check_keys(item, item_path, allowed, allowed_count, err, err_size) != 0) {
        return -1;
    }

    iems_config_path_key(field, sizeof(field), item_path, "name");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
    if (!name) return iems_config_fail(err, err_size, "MISSING_REQUIRED", field, NULL);
    if (!cJSON_IsString(name)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected string");
    }
    if (name->valuestring[0] == '\0') {
        return iems_config_fail(err, err_size, "INVALID_VALUE", field, "must be non-empty");
    }
    if (strlen(name->valuestring) >= IEMS_LC_ENTRY_NAME_MAX) {
        return iems_config_fail(err, err_size, "VALUE_TOO_LONG", field, NULL);
    }

    memset(out, 0, sizeof(*out));
    out->type = (iems_lc_value_type_t)type_id;
    memcpy(out->name, name->valuestring, strlen(name->valuestring) + 1);

    if (type_id == 0) {
        int int_value = 0;
        if (iems_config_get_int(item, "int_value", 0, INT_MIN, item_path, err, err_size,
                                &int_value) != 0) {
            return -1;
        }
        out->int_value = int_value;
    } else if (_get_double_value(item, item_path, err, err_size, &out->double_value) != 0) {
        return -1;
    }
    return 0;
}

static int _parse_document(cJSON *root, const char *rel, const char *lc_name,
                           iems_lc_user_cfg_t *cfg, char *err, size_t err_size) {
    if (_check_envelope(root, rel, err, err_size) != 0) return -1;

    char config_path[192];
    iems_config_path_key(config_path, sizeof(config_path), rel, "config");
    cJSON *config = cJSON_GetObjectItemCaseSensitive(root, "config");
    int entry_count = cJSON_GetArraySize(config);
    if (entry_count > (int)IEMS_LC_MAX_ENTRIES) {
        return iems_config_fail(err, err_size, "TOO_MANY_ITEMS", config_path,
                                "at most 32 entries");
    }

    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->name, sizeof(cfg->name), "%s", lc_name);
    cfg->state = IEMS_LC_FILE_LOADED;

    size_t index = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, config) {
        char item_path[224];
        iems_config_path_index(item_path, sizeof(item_path), config_path, index);
        iems_lc_entry_t entry;
        if (_parse_entry(item, item_path, &entry, err, err_size) != 0) return -1;
        for (size_t i = 0; i < index; i++) {
            if (strcmp(cfg->entries[i].name, entry.name) == 0) {
                char field[224];
                iems_config_path_key(field, sizeof(field), item_path, "name");
                return iems_config_fail(err, err_size, "DUPLICATE_NAME", field, entry.name);
            }
        }
        cfg->entries[index] = entry;
        index++;
    }
    cfg->count = index;
    return 0;
}

/* 0 = parsed into cfg; 1 = ABSENT (cfg cleared); -1 = error. */
static int _read_lc_document(const iems_config_source_t *source, const char *rel,
                             const char *lc_name, iems_lc_user_cfg_t *cfg, char *err,
                             size_t err_size) {
    char *data = NULL;
    size_t len = 0;
    iems_config_source_rc_t read_rc =
        source->read_file(source->ctx, rel, &data, &len, err, err_size);
    if (read_rc == IEMS_CONFIG_SOURCE_ABSENT) {
        memset(cfg, 0, sizeof(*cfg));
        snprintf(cfg->name, sizeof(cfg->name), "%s", lc_name);
        cfg->state = IEMS_LC_FILE_ABSENT;
        return 1;
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
    int rc = _parse_document(root, rel, lc_name, cfg, err, err_size);
    cJSON_Delete(root);
    return rc == 0 ? 0 : -1;
}

/* ------------------------------------------------------------------ */
/* Enumeration                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    char names[IEMS_LC_MAX_LCS][IEMS_LC_DIR_NAME_MAX];
    size_t count;
    int overflow;
} _lc_dirs_t;

static int _collect_lc_dir(void *ctx, const char *name) {
    _lc_dirs_t *dirs = (_lc_dirs_t *)ctx;
    if (strlen(name) >= IEMS_LC_DIR_NAME_MAX || dirs->count >= IEMS_LC_MAX_LCS) {
        dirs->overflow = 1;
        return 1;
    }
    snprintf(dirs->names[dirs->count], IEMS_LC_DIR_NAME_MAX, "%s", name);
    dirs->count++;
    return 0;
}

static int _dir_name_cmp(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

int iems_lc_user_cfgs_load(const iems_config_source_t *source, iems_lc_user_cfgs_t *out,
                           char *err, size_t err_size) {
    if (err && err_size) err[0] = '\0';
    if (!source || !source->read_file || !source->list_dir || !out) {
        return iems_config_fail(err, err_size, "INVALID_ARGUMENT", "$",
                                "source with read_file/list_dir and output are required");
    }

    /* 32 LC records do not fit a sane stack frame; load is a constructor. */
    iems_lc_user_cfgs_t *result = malloc(sizeof(*result));
    if (!result) {
        return iems_config_fail(err, err_size, "OUT_OF_MEMORY", "$", "cannot allocate lc model");
    }
    memset(result, 0, sizeof(*result));

    _lc_dirs_t dirs;
    memset(&dirs, 0, sizeof(dirs));
    iems_config_source_rc_t listed =
        source->list_dir(source->ctx, IEMS_LC_DEVS_DIR, _collect_lc_dir, &dirs, err, err_size);
    if (dirs.overflow) {
        free(result);
        return iems_config_fail(err, err_size, "TOO_MANY_FILES", IEMS_LC_DEVS_DIR, NULL);
    }
    if (listed == IEMS_CONFIG_SOURCE_ERROR) {
        free(result);
        return -1;
    }

    if (listed == IEMS_CONFIG_SOURCE_OK) {
        bool all_present = true;
        qsort(dirs.names, dirs.count, sizeof(dirs.names[0]), _dir_name_cmp);
        for (size_t i = 0; i < dirs.count; i++) {
            char rel[160];
            int n = snprintf(rel, sizeof(rel), IEMS_LC_DEVS_DIR "/%s/lc_user_cfg.json",
                             dirs.names[i]);
            if (n <= 0 || (size_t)n >= sizeof(rel)) {
                free(result);
                return iems_config_fail(err, err_size, "VALUE_TOO_LONG", dirs.names[i], NULL);
            }
            iems_lc_user_cfg_t cfg;
            int rc = _read_lc_document(source, rel, dirs.names[i], &cfg, err, err_size);
            if (rc < 0) {
                free(result);
                return -1;
            }
            if (rc == 1) {
                all_present = false;
                LOG_INFO(LOG_CAT_CORE, "IEMS lc user cfg: %s absent; lc kept empty", rel);
            } else {
                LOG_INFO(LOG_CAT_CORE, "IEMS lc user cfg: %s loaded (%zu entries)", rel,
                         cfg.count);
            }
            result->lcs[result->count++] = cfg;
        }
        result->loaded = all_present;
    }

    *out = *result;
    free(result);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Accessors                                                          */
/* ------------------------------------------------------------------ */

const iems_lc_user_cfg_t *iems_lc_user_cfgs_find(const iems_lc_user_cfgs_t *cfgs,
                                                const char *lc_name) {
    if (!cfgs || !lc_name) return NULL;
    for (size_t i = 0; i < cfgs->count; i++) {
        if (strcmp(cfgs->lcs[i].name, lc_name) == 0) return &cfgs->lcs[i];
    }
    return NULL;
}

const iems_lc_entry_t *iems_lc_user_cfg_find(const iems_lc_user_cfg_t *cfg, const char *name) {
    if (!cfg || !name) return NULL;
    for (size_t i = 0; i < cfg->count; i++) {
        if (strcmp(cfg->entries[i].name, name) == 0) return &cfg->entries[i];
    }
    return NULL;
}
