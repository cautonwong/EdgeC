/**
 * @file iems_alarm_registry.c
 * @brief P5-04a: strict alarm_def.json loader and typed registry queries.
 *
 * Reuses the shared strict JSON boundary helpers from iems_config_loader.c
 * (private header) so the alarm file gets the same duplicate-key, UTF-8,
 * control-character and trailing-data checks as the other config families.
 */

#include "iems_alarm_registry.h"

#include <cjson/cJSON.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iems_config_loader_priv.h"

#define IEMS_ALARM_FILE "alarm_def.json"

static const char *const k_root_keys[] = {"lang", "vendor_models"};
static const char *const k_vendor_model_keys[] = {"type", "vendor", "model", "tags"};
static const char *const k_tag_keys[] = {
    "tag",
    "alarmCode",
    "alarmInfo",
    "categoryDisplay",
    "severityDisplay",
    "entityTypeDisplay",
    "reasonCode",
    "reason",
    "additionalInfo",
    "additionalInfoParas",
    "solution",
};

_Static_assert(sizeof(k_tag_keys) / sizeof(k_tag_keys[0]) == 11, "alarm tag schema has 11 keys");

static int _get_string(cJSON *object, const char *key, bool required, bool nonempty,
                       const char *path, char *err, size_t err_size, const char **out) {
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

static int _get_array(cJSON *object, const char *key, const char *path, char *err, size_t err_size,
                      cJSON **out) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    char field[192];
    iems_config_path_key(field, sizeof(field), path, key);
    if (!item) return iems_config_fail(err, err_size, "MISSING_REQUIRED", field, NULL);
    if (!cJSON_IsArray(item)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected array");
    }
    *out = item;
    return 0;
}

/** Copy one string into the registry arena; the registry owns it afterwards. */
static int _arena_put(iems_alarm_registry_t *reg, const char *value, const char *path, char *err,
                      size_t err_size, const char **out) {
    size_t len = strlen(value) + 1;
    size_t used = (size_t)reg->strings_used;
    if (len > (size_t)IEMS_ALARM_MAX_STRINGS - used) {
        return iems_config_fail(err, err_size, "CAPACITY_EXCEEDED", path, "string arena full");
    }
    memcpy(reg->strings + used, value, len);
    if (out) *out = reg->strings + used;
    reg->strings_used = (uint32_t)(used + len);
    return 0;
}

static int _parse_tag(cJSON *tag_obj, const char *path, const char *vendor, const char *model,
                      const char *class_type, iems_alarm_registry_t *reg, char *err,
                      size_t err_size) {
    if (!cJSON_IsObject(tag_obj)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", path, "expected object");
    }
    if (iems_config_check_keys(tag_obj, path, k_tag_keys,
                               sizeof(k_tag_keys) / sizeof(k_tag_keys[0]), err, err_size) != 0) {
        return -1;
    }
    if (reg->def_count >= IEMS_ALARM_MAX_DEFS) {
        return iems_config_fail(err, err_size, "CAPACITY_EXCEEDED", path,
                                "definition capacity reached");
    }

    const char *tag = NULL;
    const char *name = NULL;
    const char *category = NULL;
    const char *severity = NULL;
    const char *entity = NULL;
    const char *reason = NULL;
    const char *solution = NULL;
    if (_get_string(tag_obj, "tag", true, true, path, err, err_size, &tag) != 0) return -1;
    if (_get_string(tag_obj, "alarmInfo", true, true, path, err, err_size, &name) != 0) return -1;
    if (_get_string(tag_obj, "categoryDisplay", true, false, path, err, err_size, &category) != 0) {
        return -1;
    }
    if (_get_string(tag_obj, "severityDisplay", true, false, path, err, err_size, &severity) != 0) {
        return -1;
    }
    if (_get_string(tag_obj, "entityTypeDisplay", true, false, path, err, err_size, &entity) != 0) {
        return -1;
    }
    if (_get_string(tag_obj, "reason", true, false, path, err, err_size, &reason) != 0) return -1;
    if (_get_string(tag_obj, "solution", true, false, path, err, err_size, &solution) != 0) {
        return -1;
    }

    /* Validated at the boundary, not modeled in this slice. */
    const char *discarded = NULL;
    if (_get_string(tag_obj, "additionalInfo", true, false, path, err, err_size, &discarded) != 0) {
        return -1;
    }
    if (_get_string(tag_obj, "additionalInfoParas", false, false, path, err, err_size,
                    &discarded) != 0) {
        return -1;
    }

    int code = -1;
    int reason_code = -1;
    if (iems_config_get_int(tag_obj, "alarmCode", 1, 0, path, err, err_size, &code) != 0) return -1;
    if (iems_config_get_int(tag_obj, "reasonCode", 1, 0, path, err, err_size, &reason_code) != 0) {
        return -1;
    }

    iems_alarm_def_t *def = &reg->defs[reg->def_count];
    const char *stored = NULL;
    if (_arena_put(reg, tag, path, err, err_size, &stored) != 0) return -1;
    def->tag = stored;
    if (_arena_put(reg, name, path, err, err_size, &stored) != 0) return -1;
    def->name = stored;
    if (_arena_put(reg, category, path, err, err_size, &stored) != 0) return -1;
    def->category = stored;
    if (_arena_put(reg, severity, path, err, err_size, &stored) != 0) return -1;
    def->severity = stored;
    if (_arena_put(reg, entity, path, err, err_size, &stored) != 0) return -1;
    def->entity = stored;
    if (_arena_put(reg, reason, path, err, err_size, &stored) != 0) return -1;
    def->reason = stored;
    if (_arena_put(reg, solution, path, err, err_size, &stored) != 0) return -1;
    def->solution = stored;

    def->vendor = vendor;
    def->model = model;
    def->class_type = class_type;
    def->code = (uint32_t)code;
    def->reason_code = reason_code;
    reg->def_count++;
    return 0;
}

static int _parse_vendor_model(cJSON *vm_obj, size_t index, const char *vm_array_path,
                               iems_alarm_registry_t *reg, char *err, size_t err_size) {
    char vm_path[192];
    iems_config_path_index(vm_path, sizeof(vm_path), vm_array_path, index);
    if (!cJSON_IsObject(vm_obj)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", vm_path, "expected object");
    }
    if (iems_config_check_keys(vm_obj, vm_path, k_vendor_model_keys,
                               sizeof(k_vendor_model_keys) / sizeof(k_vendor_model_keys[0]), err,
                               err_size) != 0) {
        return -1;
    }
    if (reg->vendor_model_count >= IEMS_ALARM_MAX_VENDOR_MODELS) {
        return iems_config_fail(err, err_size, "CAPACITY_EXCEEDED", vm_path,
                                "vendor_model capacity reached");
    }

    const char *vendor = NULL;
    const char *model = NULL;
    const char *class_type = NULL;
    if (_get_string(vm_obj, "vendor", true, true, vm_path, err, err_size, &vendor) != 0) return -1;
    if (_get_string(vm_obj, "model", true, true, vm_path, err, err_size, &model) != 0) return -1;
    if (_get_string(vm_obj, "type", true, true, vm_path, err, err_size, &class_type) != 0) {
        return -1;
    }

    const char *stored_vendor = NULL;
    const char *stored_model = NULL;
    const char *stored_class = NULL;
    if (_arena_put(reg, vendor, vm_path, err, err_size, &stored_vendor) != 0) return -1;
    if (_arena_put(reg, model, vm_path, err, err_size, &stored_model) != 0) return -1;
    if (_arena_put(reg, class_type, vm_path, err, err_size, &stored_class) != 0) return -1;

    cJSON *tags = NULL;
    if (_get_array(vm_obj, "tags", vm_path, err, err_size, &tags) != 0) return -1;

    iems_alarm_vendor_model_t *vm = &reg->vendor_models[reg->vendor_model_count];
    memset(vm, 0, sizeof(*vm));
    vm->vendor = stored_vendor;
    vm->model = stored_model;
    vm->class_type = stored_class;

    char tags_path[192];
    iems_config_path_key(tags_path, sizeof(tags_path), vm_path, "tags");
    uint32_t start = reg->def_count;
    size_t tag_index = 0;
    cJSON *tag_obj;
    cJSON_ArrayForEach(tag_obj, tags) {
        char tag_path[192];
        iems_config_path_index(tag_path, sizeof(tag_path), tags_path, tag_index);
        if (_parse_tag(tag_obj, tag_path, stored_vendor, stored_model, stored_class, reg, err,
                       err_size) != 0) {
            return -1;
        }
        tag_index++;
    }
    vm->def_start = start;
    vm->def_count = reg->def_count - start;
    reg->vendor_model_count++;
    return 0;
}

int iems_alarm_registry_load(const iems_config_source_t *source, iems_alarm_registry_t *out,
                             char *err, size_t err_size) {
    if (!out) return iems_config_fail(err, err_size, "INVALID_ARGUMENT", "$", "registry is NULL");
    memset(out, 0, sizeof(*out));
    if (!source || !source->read_file) {
        return iems_config_fail(err, err_size, "INVALID_ARGUMENT", "$", "source port is NULL");
    }

    char *data = NULL;
    size_t len = 0;
    iems_config_source_rc_t rc =
        source->read_file(source->ctx, IEMS_ALARM_FILE, &data, &len, err, err_size);
    if (rc != IEMS_CONFIG_SOURCE_OK) {
        if (rc == IEMS_CONFIG_SOURCE_ABSENT) {
            return iems_config_fail(err, err_size, "MISSING_FILE", IEMS_ALARM_FILE, NULL);
        }
        return -1;
    }

    cJSON *root = NULL;
    int ret = -1;
    const char *lang = NULL;
    cJSON *vendor_models = NULL;
    char vm_array_path[192];
    size_t index = 0;
    cJSON *vm_obj = NULL;
    if (iems_config_strict_parse(data, len, IEMS_ALARM_FILE, &root, err, err_size) != 0) {
        free(data);
        return -1;
    }
    free(data);

    if (!cJSON_IsObject(root)) {
        iems_config_fail(err, err_size, "INVALID_ROOT", IEMS_ALARM_FILE, "expected object");
        goto done;
    }
    if (iems_config_check_keys(root, IEMS_ALARM_FILE, k_root_keys,
                               sizeof(k_root_keys) / sizeof(k_root_keys[0]), err, err_size) != 0) {
        goto done;
    }

    if (_get_string(root, "lang", true, true, IEMS_ALARM_FILE, err, err_size, &lang) != 0) {
        goto done;
    }
    if (strlen(lang) >= sizeof(out->lang)) {
        iems_config_fail(err, err_size, "VALUE_TOO_LONG", IEMS_ALARM_FILE ".lang", NULL);
        goto done;
    }
    memcpy(out->lang, lang, strlen(lang) + 1);

    if (_get_array(root, "vendor_models", IEMS_ALARM_FILE, err, err_size, &vendor_models) != 0) {
        goto done;
    }
    iems_config_path_key(vm_array_path, sizeof(vm_array_path), IEMS_ALARM_FILE, "vendor_models");
    cJSON_ArrayForEach(vm_obj, vendor_models) {
        if (_parse_vendor_model(vm_obj, index, vm_array_path, out, err, err_size) != 0) goto done;
        index++;
    }
    ret = 0;

done:
    cJSON_Delete(root);
    if (ret != 0) {
        out->lang[0] = '\0';
        out->vendor_model_count = 0;
        out->def_count = 0;
        out->strings_used = 0;
    }
    return ret;
}

const iems_alarm_def_t *iems_alarm_registry_find(const iems_alarm_registry_t *reg,
                                                 const char *class_type, const char *vendor,
                                                 const char *model, const char *tag) {
    if (!reg || !class_type || !vendor || !model || !tag) return NULL;
    for (uint32_t i = 0; i < reg->def_count; i++) {
        const iems_alarm_def_t *def = &reg->defs[i];
        if (strcmp(def->class_type, class_type) == 0 && strcmp(def->vendor, vendor) == 0 &&
            strcmp(def->model, model) == 0 && strcmp(def->tag, tag) == 0) {
            return def;
        }
    }
    return NULL;
}

const iems_alarm_def_t *iems_alarm_registry_find_by_code(const iems_alarm_registry_t *reg,
                                                         uint32_t code, int32_t reason_code) {
    if (!reg) return NULL;
    for (uint32_t i = 0; i < reg->def_count; i++) {
        if (reg->defs[i].code == code && reg->defs[i].reason_code == reason_code) {
            return &reg->defs[i];
        }
    }
    return NULL;
}
