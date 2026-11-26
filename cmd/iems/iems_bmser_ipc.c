/**
 * @file iems_bmser_ipc.c
 * @brief P3-01b: strict loader for the BMSER-BIN northbound IPC device file.
 *
 * Reuses the shared strict JSON boundary helpers from iems_config_loader.c so
 * this file gets the same duplicate-key, trailing-data, UTF-8, and key
 * whitelist checks as the device/port config families.
 */

#include "iems_bmser_ipc.h"

#include <cjson/cJSON.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iems_config_loader_priv.h"

static const char *const k_root_keys[] = {"schema_version", "dev"};
static const char *const k_dev_keys[] = {"dev_id", "module", "type", "blocks"};
static const char *const k_block_keys[] = {"block_id", "cycle", "sub_id"};
static const char *const k_sub_keys[] = {"name", "type", "tag", "scale"};

static const char *const k_type_names[] = {"U16", "S16", "U32", "S32", "FLOAT", "BI16"};
#define IEMS_BMSER_TYPE_COUNT (sizeof(k_type_names) / sizeof(k_type_names[0]))

const char *iems_bmser_type_name(iems_bmser_type_t type) {
    if (type < 0 || (size_t)type >= IEMS_BMSER_TYPE_COUNT) return NULL;
    return k_type_names[type];
}

static int _type_from_string(const char *name, iems_bmser_type_t *out) {
    for (size_t i = 0; i < IEMS_BMSER_TYPE_COUNT; i++) {
        if (strcmp(name, k_type_names[i]) == 0) {
            *out = (iems_bmser_type_t)i;
            return 0;
        }
    }
    return -1;
}

/* "place" followed by one or more decimal digits means an unused wire slot. */
static int _is_placeholder(const char *tag) {
    if (strncmp(tag, "place", 5) != 0 || tag[5] == '\0') return 0;
    for (const char *p = tag + 5; *p; p++) {
        if (!isdigit((unsigned char)*p)) return 0;
    }
    return 1;
}

static int _get_string(cJSON *object, const char *key, int required, int nonempty, size_t cap,
                       const char *path, char *err, size_t err_size, char *dst) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    char field[192];
    iems_config_path_key(field, sizeof(field), path, key);
    if (!item) {
        if (required) return iems_config_fail(err, err_size, "MISSING_REQUIRED", field, NULL);
        dst[0] = '\0';
        return 0;
    }
    if (!cJSON_IsString(item)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected string");
    }
    size_t len = strlen(item->valuestring);
    if (nonempty && len == 0) {
        return iems_config_fail(err, err_size, "INVALID_VALUE", field, "must be non-empty");
    }
    if (len >= cap) return iems_config_fail(err, err_size, "VALUE_TOO_LONG", field, NULL);
    memcpy(dst, item->valuestring, len + 1);
    return 0;
}

static int _get_scale(cJSON *object, const char *path, char *err, size_t err_size, double *out) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, "scale");
    char field[192];
    iems_config_path_key(field, sizeof(field), path, "scale");
    if (!item) return iems_config_fail(err, err_size, "MISSING_REQUIRED", field, NULL);
    if (!cJSON_IsNumber(item)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected number");
    }
    double value = item->valuedouble;
    if (!isfinite(value))
        return iems_config_fail(err, err_size, "INVALID_VALUE", field, "not finite");
    if (value == 0)
        return iems_config_fail(err, err_size, "INVALID_VALUE", field, "must be non-zero");
    *out = value;
    return 0;
}

/* v3 analysis_addr: "0x" prefix means hex, anything else decimal. This parser
 * accepts only well-formed uint32 text; sscanf would accept leading spaces,
 * signs, and trailing garbage. */
static int _parse_block_id(const char *text, uint32_t *out) {
    const char *p = text;
    unsigned base = 10;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        base = 16;
        p += 2;
    }
    size_t digits = strlen(p);
    if (digits == 0 || digits > (base == 16 ? 8u : 10u)) return -1;
    uint64_t value = 0;
    for (const char *q = p; *q; q++) {
        unsigned digit;
        if (isdigit((unsigned char)*q)) {
            digit = (unsigned)(*q - '0');
        } else if (base == 16 && isxdigit((unsigned char)*q)) {
            digit = (unsigned)(tolower((unsigned char)*q) - 'a' + 10);
        } else {
            return -1;
        }
        if (digit >= base) return -1;
        value = value * base + digit;
        if (value > UINT32_MAX) return -1;
    }
    *out = (uint32_t)value;
    return 0;
}

static int _parse_sub(cJSON *sub, const iems_bmser_dev_t *dev, const char *sub_path,
                      iems_bmser_sub_t *out, char *err, size_t err_size) {
    if (!cJSON_IsObject(sub)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", sub_path, "expected object");
    }
    if (iems_config_check_keys(sub, sub_path, k_sub_keys,
                               sizeof(k_sub_keys) / sizeof(k_sub_keys[0]), err, err_size) != 0) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    if (_get_string(sub, "name", 0, 0, IEMS_BMSER_MAX_NAME_LEN, sub_path, err, err_size,
                    out->name) != 0) {
        return -1;
    }

    char type_name[16];
    if (_get_string(sub, "type", 1, 1, sizeof(type_name), sub_path, err, err_size, type_name) !=
        0) {
        return -1;
    }
    if (_type_from_string(type_name, &out->type) != 0) {
        char field[192];
        iems_config_path_key(field, sizeof(field), sub_path, "type");
        return iems_config_fail(err, err_size, "INVALID_VALUE", field, "unknown type");
    }

    if (_get_string(sub, "tag", 1, 1, IEMS_BMSER_MAX_TAG_LEN, sub_path, err, err_size, out->tag) !=
        0) {
        return -1;
    }
    if (strlen(dev->dev_id) + 1 + strlen(out->tag) >= IEMS_BMSER_MAX_TAG_KEY_LEN) {
        char field[192];
        iems_config_path_key(field, sizeof(field), sub_path, "tag");
        return iems_config_fail(err, err_size, "VALUE_TOO_LONG", field,
                                "dev_id.tag must fit 63 bytes");
    }

    return _get_scale(sub, sub_path, err, err_size, &out->scale);
}

static int _parse_block(cJSON *block, size_t block_index, const iems_bmser_dev_t *dev,
                        const char *dev_path, iems_bmser_ipc_t *model, iems_bmser_block_t *out,
                        char *err, size_t err_size) {
    char blocks_path[192];
    iems_config_path_key(blocks_path, sizeof(blocks_path), dev_path, "blocks");
    char block_path[224];
    iems_config_path_index(block_path, sizeof(block_path), blocks_path, block_index);
    if (!cJSON_IsObject(block)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", block_path, "expected object");
    }
    if (iems_config_check_keys(block, block_path, k_block_keys,
                               sizeof(k_block_keys) / sizeof(k_block_keys[0]), err,
                               err_size) != 0) {
        return -1;
    }
    memset(out, 0, sizeof(*out));

    char id_text[IEMS_BMSER_MAX_BLOCK_ID_TEXT];
    if (_get_string(block, "block_id", 1, 1, sizeof(id_text), block_path, err, err_size, id_text) !=
        0) {
        return -1;
    }
    if (_parse_block_id(id_text, &out->block_id) != 0) {
        char field[224];
        iems_config_path_key(field, sizeof(field), block_path, "block_id");
        return iems_config_fail(err, err_size, "INVALID_VALUE", field,
                                "expected 0x hex or decimal uint32");
    }
    for (size_t i = 0; i < block_index; i++) {
        if (dev->blocks[i].block_id == out->block_id) {
            char field[224];
            iems_config_path_key(field, sizeof(field), block_path, "block_id");
            return iems_config_fail(err, err_size, "DUPLICATE_BLOCK_ID", field, NULL);
        }
    }

    int cycle = 0;
    if (iems_config_get_int(block, "cycle", 1, 0, block_path, err, err_size, &cycle) != 0) {
        return -1;
    }
    if (cycle != 0 && (cycle % 1000 != 0)) {
        char field[224];
        iems_config_path_key(field, sizeof(field), block_path, "cycle");
        return iems_config_fail(err, err_size, "INVALID_VALUE", field,
                                "must be 0 or a multiple of 1000 ms");
    }
    out->cycle_ms = (uint32_t)cycle;

    cJSON *subs = cJSON_GetObjectItemCaseSensitive(block, "sub_id");
    char subs_path[224];
    iems_config_path_key(subs_path, sizeof(subs_path), block_path, "sub_id");
    if (!subs) return iems_config_fail(err, err_size, "MISSING_REQUIRED", subs_path, NULL);
    if (!cJSON_IsArray(subs)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", subs_path, "expected array");
    }
    int sub_count = cJSON_GetArraySize(subs);
    if (sub_count > (int)IEMS_BMSER_MAX_SUBS_PER_BLOCK) {
        return iems_config_fail(err, err_size, "TOO_MANY_ITEMS", subs_path, "at most 512 entries");
    }
    if (sub_count == 0) return 0;

    out->subs = calloc((size_t)sub_count, sizeof(*out->subs));
    if (!out->subs) return iems_config_fail(err, err_size, "ALLOC_FAILED", subs_path, NULL);

    size_t sub_index = 0;
    cJSON *sub;
    cJSON_ArrayForEach(sub, subs) {
        char sub_path[256];
        iems_config_path_index(sub_path, sizeof(sub_path), subs_path, sub_index);
        if (_parse_sub(sub, dev, sub_path, &out->subs[sub_index], err, err_size) != 0) return -1;
        out->sub_count++;
        model->sub_count++;
        if (_is_placeholder(out->subs[sub_index].tag)) model->placeholder_count++;
        sub_index++;
    }
    return 0;
}

static int _parse_dev(cJSON *dev, size_t dev_index, const char *rel, iems_bmser_ipc_t *model,
                      iems_bmser_dev_t *out, char *err, size_t err_size) {
    char devs_path[192];
    iems_config_path_key(devs_path, sizeof(devs_path), rel, "dev");
    char dev_path[224];
    iems_config_path_index(dev_path, sizeof(dev_path), devs_path, dev_index);
    if (!cJSON_IsObject(dev)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", dev_path, "expected object");
    }
    if (iems_config_check_keys(dev, dev_path, k_dev_keys,
                               sizeof(k_dev_keys) / sizeof(k_dev_keys[0]), err, err_size) != 0) {
        return -1;
    }
    memset(out, 0, sizeof(*out));

    if (_get_string(dev, "dev_id", 1, 1, IEMS_BMSER_MAX_ID_LEN, dev_path, err, err_size,
                    out->dev_id) != 0) {
        return -1;
    }
    for (size_t i = 0; i < dev_index; i++) {
        if (strcmp(model->devs[i].dev_id, out->dev_id) == 0) {
            char field[224];
            iems_config_path_key(field, sizeof(field), dev_path, "dev_id");
            return iems_config_fail(err, err_size, "DUPLICATE_DEV_ID", field, NULL);
        }
    }
    if (_get_string(dev, "module", 1, 1, IEMS_BMSER_MAX_ID_LEN, dev_path, err, err_size,
                    out->module) != 0) {
        return -1;
    }
    if (_get_string(dev, "type", 1, 1, IEMS_BMSER_MAX_ID_LEN, dev_path, err, err_size, out->type) !=
        0) {
        return -1;
    }

    cJSON *blocks = cJSON_GetObjectItemCaseSensitive(dev, "blocks");
    char blocks_path[224];
    iems_config_path_key(blocks_path, sizeof(blocks_path), dev_path, "blocks");
    if (!blocks) return iems_config_fail(err, err_size, "MISSING_REQUIRED", blocks_path, NULL);
    if (!cJSON_IsArray(blocks)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", blocks_path, "expected array");
    }
    int block_count = cJSON_GetArraySize(blocks);
    if (block_count > (int)IEMS_BMSER_MAX_BLOCKS_PER_DEV) {
        return iems_config_fail(err, err_size, "TOO_MANY_ITEMS", blocks_path,
                                "at most 256 entries");
    }
    if (block_count == 0) return 0;

    out->blocks = calloc((size_t)block_count, sizeof(*out->blocks));
    if (!out->blocks) return iems_config_fail(err, err_size, "ALLOC_FAILED", blocks_path, NULL);
    /* Set the slot count before parsing so a mid-array failure still lets
     * iems_bmser_ipc_free release every partially filled block. */
    out->block_count = (size_t)block_count;

    size_t block_index = 0;
    cJSON *block;
    cJSON_ArrayForEach(block, blocks) {
        if (_parse_block(block, block_index, out, dev_path, model, &out->blocks[block_index], err,
                         err_size) != 0) {
            return -1;
        }
        model->block_count++;
        block_index++;
    }
    return 0;
}

static int _parse_root(cJSON *root, const char *rel, iems_bmser_ipc_t *model, char *err,
                       size_t err_size) {
    if (!cJSON_IsObject(root)) {
        return iems_config_fail(err, err_size, "INVALID_ROOT", rel, "expected object");
    }
    if (iems_config_check_keys(root, rel, k_root_keys, sizeof(k_root_keys) / sizeof(k_root_keys[0]),
                               err, err_size) != 0) {
        return -1;
    }
    cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "schema_version");
    if (version) {
        char path[192];
        iems_config_path_key(path, sizeof(path), rel, "schema_version");
        if (!cJSON_IsString(version)) {
            return iems_config_fail(err, err_size, "INVALID_TYPE", path, "expected string");
        }
        if (strcmp(version->valuestring, "1.0.0") != 0) {
            return iems_config_fail(err, err_size, "UNSUPPORTED_SCHEMA_VERSION", path,
                                    version->valuestring);
        }
    }

    cJSON *devs = cJSON_GetObjectItemCaseSensitive(root, "dev");
    char devs_path[192];
    iems_config_path_key(devs_path, sizeof(devs_path), rel, "dev");
    if (!devs) return iems_config_fail(err, err_size, "MISSING_REQUIRED", devs_path, NULL);
    if (!cJSON_IsArray(devs)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", devs_path, "expected array");
    }
    int dev_count = cJSON_GetArraySize(devs);
    if (dev_count > (int)IEMS_BMSER_MAX_DEVS) {
        return iems_config_fail(err, err_size, "TOO_MANY_ITEMS", devs_path, "at most 256 entries");
    }
    if (dev_count == 0) return 0;

    model->devs = calloc((size_t)dev_count, sizeof(*model->devs));
    if (!model->devs) return iems_config_fail(err, err_size, "ALLOC_FAILED", devs_path, NULL);
    /* Set the slot count before parsing so a mid-array failure still lets
     * iems_bmser_ipc_free release every partially filled device. */
    model->dev_count = (size_t)dev_count;

    size_t dev_index = 0;
    cJSON *dev;
    cJSON_ArrayForEach(dev, devs) {
        if (_parse_dev(dev, dev_index, rel, model, &model->devs[dev_index], err, err_size) != 0) {
            return -1;
        }
        dev_index++;
    }
    return 0;
}

int iems_bmser_ipc_load(const iems_config_source_t *source, const char *relative_path,
                        iems_bmser_ipc_t *out, char *err, size_t err_size) {
    if (err && err_size) err[0] = '\0';
    if (!source || !source->read_file || !out) {
        return iems_config_fail(err, err_size, "INVALID_ARGUMENT", "$", NULL);
    }
    memset(out, 0, sizeof(*out));
    const char *rel = relative_path ? relative_path : IEMS_BMSER_IPC_FILE;

    char *data = NULL;
    size_t len = 0;
    iems_config_source_rc_t read_rc =
        source->read_file(source->ctx, rel, &data, &len, err, err_size);
    if (read_rc == IEMS_CONFIG_SOURCE_ABSENT) {
        return iems_config_fail(err, err_size, "MISSING_REQUIRED", rel, NULL);
    }
    if (read_rc != IEMS_CONFIG_SOURCE_OK) return -1;

    cJSON *root = NULL;
    int rc = iems_config_strict_parse(data, len, rel, &root, err, err_size);
    free(data);
    if (rc != 0) return -1;

    iems_bmser_ipc_t model;
    memset(&model, 0, sizeof(model));
    rc = _parse_root(root, rel, &model, err, err_size);
    cJSON_Delete(root);
    if (rc != 0) {
        iems_bmser_ipc_free(&model);
        return -1;
    }
    model.loaded = true;
    *out = model;
    return 0;
}

void iems_bmser_ipc_free(iems_bmser_ipc_t *model) {
    if (!model) return;
    for (size_t i = 0; i < model->dev_count; i++) {
        iems_bmser_dev_t *dev = &model->devs[i];
        for (size_t j = 0; j < dev->block_count; j++) free(dev->blocks[j].subs);
        free(dev->blocks);
    }
    free(model->devs);
    memset(model, 0, sizeof(*model));
}
