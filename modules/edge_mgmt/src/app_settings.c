/**
 * @file app_settings.c
 * @brief Manifest settings schema validation and value checks.
 */

#include "edgeos/app_settings.h"

#include <cjson/cJSON.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETTING_TITLE_MAX 128
#define SETTING_DESCRIPTION_MAX 512
#define SETTING_UNIT_MAX 16
#define SETTING_GROUP_MAX 64
#define SETTING_ORDER_MAX 1000000.0
#define SETTING_LENGTH_MAX 1000000.0
#define SETTING_INTEGER_MAX 9007199254740992.0 /* 2^53, exact in a double */

enum {
    SETTING_TYPE_INVALID = 0,
    SETTING_TYPE_STRING,
    SETTING_TYPE_NUMBER,
    SETTING_TYPE_INTEGER,
    SETTING_TYPE_BOOLEAN,
    SETTING_TYPE_STRING_ARRAY,
};

static const char *const k_field_keys[] = {
    "type",    "title",     "description", "default",  "minimum",
    "maximum", "minLength", "maxLength",   "enum",     "secret",
    "unit",    "group",     "order",       "readonly", "requiresRestart",
};

static int fail(char *reason, size_t size, const char *text) {
    if (reason && size) snprintf(reason, size, "%s", text);
    return -1;
}

static int fail_key(char *reason, size_t size, const char *key, const char *text) {
    if (reason && size) snprintf(reason, size, "%s: %s", key, text);
    return -1;
}

static int field_type(const cJSON *def) {
    const cJSON *type = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "type");
    if (!cJSON_IsString(type)) return SETTING_TYPE_INVALID;
    if (strcmp(type->valuestring, "string") == 0) return SETTING_TYPE_STRING;
    if (strcmp(type->valuestring, "number") == 0) return SETTING_TYPE_NUMBER;
    if (strcmp(type->valuestring, "integer") == 0) return SETTING_TYPE_INTEGER;
    if (strcmp(type->valuestring, "boolean") == 0) return SETTING_TYPE_BOOLEAN;
    if (strcmp(type->valuestring, "array<string>") == 0) return SETTING_TYPE_STRING_ARRAY;
    return SETTING_TYPE_INVALID;
}

static int is_integer_number(const cJSON *item) {
    if (item == NULL || !cJSON_IsNumber(item)) return 0;
    double value = item->valuedouble;
    return isfinite(value) && fabs(value) <= SETTING_INTEGER_MAX &&
           value == (double)(long long)value;
}

static int read_length(const cJSON *def, const char *name, size_t *out, int *present) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)def, name);
    *present = item != NULL;
    if (!item) return 0;
    if (!is_integer_number(item) || item->valuedouble < 0.0 ||
        item->valuedouble > SETTING_LENGTH_MAX) {
        return -1;
    }
    *out = (size_t)item->valuedouble;
    return 0;
}

static int check_string_field(const cJSON *def, const char *name, size_t max, char *reason,
                              size_t size) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)def, name);
    if (!item) return 0;
    if (!cJSON_IsString(item) || strlen(item->valuestring) >= max) {
        return fail_key(reason, size, name, "must be a short string");
    }
    return 0;
}

static int check_bool_field(const cJSON *def, const char *name, char *reason, size_t size) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)def, name);
    if (item && !cJSON_IsBool(item)) return fail_key(reason, size, name, "must be boolean");
    return 0;
}

/* length bounds apply to string length and array size */
static int check_length_bounds(const cJSON *def, char *reason, size_t size) {
    size_t min_len = 0, max_len = 0;
    int has_min = 0, has_max = 0;
    if (read_length(def, "minLength", &min_len, &has_min) != 0 ||
        read_length(def, "maxLength", &max_len, &has_max) != 0) {
        return fail(reason, size, "minLength/maxLength must be non-negative integers");
    }
    if (has_min && has_max && min_len > max_len) {
        return fail(reason, size, "minLength must not exceed maxLength");
    }
    return 0;
}

static int check_number_bounds(const cJSON *def, char *reason, size_t size) {
    const cJSON *min = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "minimum");
    const cJSON *max = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "maximum");
    if (min && (!cJSON_IsNumber(min) || !isfinite(min->valuedouble))) {
        return fail(reason, size, "minimum must be a number");
    }
    if (max && (!cJSON_IsNumber(max) || !isfinite(max->valuedouble))) {
        return fail(reason, size, "maximum must be a number");
    }
    if (min && max && min->valuedouble > max->valuedouble) {
        return fail(reason, size, "minimum must not exceed maximum");
    }
    return 0;
}

static int check_enum(const cJSON *def, int type, char *reason, size_t size) {
    const cJSON *items = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "enum");
    if (!items) return 0;
    if (!cJSON_IsArray(items) || cJSON_GetArraySize(items) == 0 ||
        cJSON_GetArraySize(items) > APP_SETTING_ENUM_MAX) {
        return fail(reason, size, "enum must be a non-empty array");
    }
    if (type == SETTING_TYPE_STRING_ARRAY) {
        return fail(reason, size, "enum is not supported for array<string>");
    }
    for (const cJSON *item = items->child; item; item = item->next) {
        int matches = type == SETTING_TYPE_STRING    ? cJSON_IsString(item)
                      : type == SETTING_TYPE_BOOLEAN ? cJSON_IsBool(item)
                      : type == SETTING_TYPE_INTEGER ? is_integer_number(item)
                                                     : cJSON_IsNumber(item);
        if (!matches) return fail(reason, size, "enum values must match the field type");
    }
    return 0;
}

static int value_in_enum(const cJSON *def, const cJSON *value, int type) {
    const cJSON *items = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "enum");
    if (!items) return 1;
    for (const cJSON *item = items->child; item; item = item->next) {
        if (type == SETTING_TYPE_STRING && cJSON_IsString(item) &&
            strcmp(item->valuestring, value->valuestring) == 0) {
            return 1;
        }
        if ((type == SETTING_TYPE_NUMBER || type == SETTING_TYPE_INTEGER) && cJSON_IsNumber(item) &&
            item->valuedouble == value->valuedouble) {
            return 1;
        }
        if (type == SETTING_TYPE_BOOLEAN && cJSON_IsBool(item) &&
            cJSON_IsTrue(item) == cJSON_IsTrue(value)) {
            return 1;
        }
    }
    return 0;
}

static int validate_value(const cJSON *def, const cJSON *value, const char *key, char *reason,
                          size_t size) {
    int type = field_type(def);
    if (type == SETTING_TYPE_STRING) {
        if (!cJSON_IsString(value)) return fail_key(reason, size, key, "type_mismatch");
        size_t min_len = 0, max_len = 0;
        int has_min = 0, has_max = 0;
        (void)read_length(def, "minLength", &min_len, &has_min);
        (void)read_length(def, "maxLength", &max_len, &has_max);
        size_t len = strlen(value->valuestring);
        if ((has_min && len < min_len) || (has_max && len > max_len)) {
            return fail_key(reason, size, key, "out_of_range");
        }
    } else if (type == SETTING_TYPE_NUMBER || type == SETTING_TYPE_INTEGER) {
        if (!cJSON_IsNumber(value) || !isfinite(value->valuedouble)) {
            return fail_key(reason, size, key, "type_mismatch");
        }
        if (type == SETTING_TYPE_INTEGER) {
            double number = value->valuedouble;
            if (fabs(number) > SETTING_INTEGER_MAX || number != (double)(long long)number) {
                return fail_key(reason, size, key, "type_mismatch");
            }
        }
        const cJSON *min = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "minimum");
        const cJSON *max = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "maximum");
        if ((min && value->valuedouble < min->valuedouble) ||
            (max && value->valuedouble > max->valuedouble)) {
            return fail_key(reason, size, key, "out_of_range");
        }
    } else if (type == SETTING_TYPE_BOOLEAN) {
        if (!cJSON_IsBool(value)) return fail_key(reason, size, key, "type_mismatch");
    } else if (type == SETTING_TYPE_STRING_ARRAY) {
        if (!cJSON_IsArray(value)) return fail_key(reason, size, key, "type_mismatch");
        size_t min_len = 0, max_len = 0;
        int has_min = 0, has_max = 0;
        (void)read_length(def, "minLength", &min_len, &has_min);
        (void)read_length(def, "maxLength", &max_len, &has_max);
        size_t count = (size_t)cJSON_GetArraySize(value);
        if ((has_min && count < min_len) || (has_max && count > max_len)) {
            return fail_key(reason, size, key, "out_of_range");
        }
        for (const cJSON *item = value->child; item; item = item->next) {
            if (!cJSON_IsString(item)) return fail_key(reason, size, key, "type_mismatch");
        }
    } else {
        return fail_key(reason, size, key, "invalid type");
    }
    if (!value_in_enum(def, value, type)) return fail_key(reason, size, key, "invalid_value");
    return 0;
}

static int check_definition(const cJSON *def, const char *key, char *reason, size_t size) {
    if (!cJSON_IsObject(def)) return fail_key(reason, size, key, "must be an object");
    for (const cJSON *item = def->child; item; item = item->next) {
        int known = 0;
        for (size_t i = 0; i < sizeof(k_field_keys) / sizeof(k_field_keys[0]); i++) {
            if (strcmp(item->string, k_field_keys[i]) == 0) known = 1;
        }
        if (!known) return fail_key(reason, size, item->string, "is not supported");
    }

    int type = field_type(def);
    if (type == SETTING_TYPE_INVALID) return fail_key(reason, size, key, "type is invalid");
    if (check_string_field(def, "title", SETTING_TITLE_MAX, reason, size) != 0 ||
        check_string_field(def, "description", SETTING_DESCRIPTION_MAX, reason, size) != 0 ||
        check_string_field(def, "unit", SETTING_UNIT_MAX, reason, size) != 0 ||
        check_string_field(def, "group", SETTING_GROUP_MAX, reason, size) != 0 ||
        check_bool_field(def, "secret", reason, size) != 0 ||
        check_bool_field(def, "readonly", reason, size) != 0 ||
        check_bool_field(def, "requiresRestart", reason, size) != 0) {
        return -1;
    }

    const cJSON *order = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "order");
    if (order && (!is_integer_number(order) || order->valuedouble < 0.0 ||
                  order->valuedouble > SETTING_ORDER_MAX)) {
        return fail_key(reason, size, "order", "must be a non-negative integer");
    }
    if (type == SETTING_TYPE_NUMBER || type == SETTING_TYPE_INTEGER) {
        if (check_number_bounds(def, reason, size) != 0) return -1;
    } else if (cJSON_GetObjectItemCaseSensitive((cJSON *)def, "minimum") ||
               cJSON_GetObjectItemCaseSensitive((cJSON *)def, "maximum")) {
        return fail_key(reason, size, key, "minimum/maximum require a numeric type");
    }
    if (type == SETTING_TYPE_STRING || type == SETTING_TYPE_STRING_ARRAY) {
        if (check_length_bounds(def, reason, size) != 0) return -1;
    } else if (cJSON_GetObjectItemCaseSensitive((cJSON *)def, "minLength") ||
               cJSON_GetObjectItemCaseSensitive((cJSON *)def, "maxLength")) {
        return fail_key(reason, size, key, "minLength/maxLength require a string type");
    }
    if (check_enum(def, type, reason, size) != 0) return -1;

    const cJSON *default_value = cJSON_GetObjectItemCaseSensitive((cJSON *)def, "default");
    if (default_value && validate_value(def, default_value, key, reason, size) != 0) {
        return -1;
    }
    return 0;
}

/* Stored schemas are canonical: {"schemaVersion":1,"fields":{...}}. */
static const cJSON *settings_fields(const cJSON *settings) {
    const cJSON *fields = cJSON_GetObjectItemCaseSensitive((cJSON *)settings, "fields");
    return cJSON_IsObject(fields) ? fields : settings;
}

static int check_field_key(const char *key, char *reason, size_t size) {
    size_t len = strlen(key);
    if (len == 0 || len >= APP_SETTING_KEY_SIZE ||
        !((key[0] >= 'A' && key[0] <= 'Z') || (key[0] >= 'a' && key[0] <= 'z'))) {
        return fail(reason, size, "settings keys must start with a letter");
    }
    for (size_t i = 0; i < len; i++) {
        char c = key[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-' || c == '.')) {
            return fail(reason, size, "settings key has an invalid character");
        }
    }
    return 0;
}

static int check_settings_object(const cJSON *settings, char *reason, size_t size) {
    const cJSON *raw_fields = cJSON_GetObjectItemCaseSensitive((cJSON *)settings, "fields");
    const cJSON *version = cJSON_GetObjectItemCaseSensitive((cJSON *)settings, "schemaVersion");
    const cJSON *fields = settings;
    if (raw_fields || version) {
        for (const cJSON *item = settings->child; item; item = item->next) {
            if (strcmp(item->string, "fields") != 0 && strcmp(item->string, "schemaVersion") != 0) {
                return fail_key(reason, size, item->string, "is not supported");
            }
        }
        if (version && (!is_integer_number(version) || version->valuedouble != 1.0)) {
            return fail(reason, size, "schemaVersion must be 1");
        }
        if (!cJSON_IsObject(raw_fields)) {
            return fail(reason, size, "settings.fields must be an object");
        }
        fields = raw_fields;
    }

    size_t count = 0;
    for (const cJSON *field = fields->child; field; field = field->next) {
        if (count >= APP_SETTING_MAX_FIELDS) {
            return fail(reason, size, "too many settings fields");
        }
        if (check_field_key(field->string, reason, size) != 0) return -1;
        for (const cJSON *prev = fields->child; prev && prev != field; prev = prev->next) {
            if (strcmp(prev->string, field->string) == 0) {
                return fail_key(reason, size, field->string, "is a duplicate key");
            }
        }
        if (check_definition(field, field->string, reason, size) != 0) return -1;
        count++;
    }
    return 0;
}

int edge_app_settings_validate_json(const char *settings_json, char *reason, size_t reason_size) {
    if (!settings_json) return fail(reason, reason_size, "settings is empty");
    cJSON *settings = cJSON_Parse(settings_json);
    if (!settings) return fail(reason, reason_size, "settings is not valid JSON");
    int rc = cJSON_IsObject(settings) ? check_settings_object(settings, reason, reason_size)
                                      : fail(reason, reason_size, "settings must be an object");
    cJSON_Delete(settings);
    return rc;
}

char *edge_app_settings_normalize_json(const char *settings_json, char *reason,
                                       size_t reason_size) {
    if (edge_app_settings_validate_json(settings_json, reason, reason_size) != 0) return NULL;
    cJSON *settings = cJSON_Parse(settings_json);
    if (!settings) return NULL;

    cJSON *root = cJSON_CreateObject();
    cJSON *fields = cJSON_CreateObject();
    if (!root || !fields) {
        cJSON_Delete(root);
        cJSON_Delete(fields);
        cJSON_Delete(settings);
        return NULL;
    }
    const cJSON *source = settings_fields(settings);
    for (const cJSON *field = source->child; field; field = field->next) {
        cJSON *copy = cJSON_Duplicate(field, 1);
        if (!copy || !cJSON_AddItemToObject(fields, field->string, copy)) {
            cJSON_Delete(copy);
            cJSON_Delete(root);
            cJSON_Delete(fields);
            cJSON_Delete(settings);
            return NULL;
        }
    }
    cJSON_AddNumberToObject(root, "schemaVersion", 1);
    if (!cJSON_AddItemToObject(root, "fields", fields)) {
        cJSON_Delete(fields);
        cJSON_Delete(root);
        cJSON_Delete(settings);
        return NULL;
    }
    char *printed = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    cJSON_Delete(settings);
    return printed;
}

int edge_app_setting_validate_value(const char *settings_json, const char *key,
                                    const char *value_json, char *reason, size_t reason_size) {
    if (!settings_json || !key || !value_json) {
        return fail(reason, reason_size, "invalid");
    }
    cJSON *settings = cJSON_Parse(settings_json);
    if (!settings) return fail(reason, reason_size, "settings is not valid JSON");
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(settings_fields(settings), key);
    if (!cJSON_IsObject(field)) {
        cJSON_Delete(settings);
        return fail(reason, reason_size, "unknown_key");
    }
    const cJSON *readonly = cJSON_GetObjectItemCaseSensitive((cJSON *)field, "readonly");
    if (cJSON_IsBool(readonly) && cJSON_IsTrue(readonly)) {
        cJSON_Delete(settings);
        return fail(reason, reason_size, "readonly");
    }
    cJSON *value = cJSON_Parse(value_json);
    if (!value) {
        cJSON_Delete(settings);
        return fail(reason, reason_size, "invalid_value");
    }
    int rc = validate_value(field, value, key, reason, reason_size);
    cJSON_Delete(value);
    cJSON_Delete(settings);
    return rc;
}

int edge_app_setting_declared(const char *settings_json, const char *key) {
    if (!settings_json || !key) return -1;
    cJSON *settings = cJSON_Parse(settings_json);
    if (!settings) return -1;
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(settings_fields(settings), key);
    int declared = cJSON_IsObject(field) ? 1 : 0;
    cJSON_Delete(settings);
    return declared;
}

int edge_app_setting_default_json(const char *settings_json, const char *key, char *out, size_t cap,
                                  char *reason, size_t reason_size) {
    if (!settings_json || !key || !out || cap == 0) {
        return fail(reason, reason_size, "invalid");
    }
    cJSON *settings = cJSON_Parse(settings_json);
    if (!settings) return fail(reason, reason_size, "settings is not valid JSON");
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(settings_fields(settings), key);
    const cJSON *default_value =
        cJSON_IsObject(field) ? cJSON_GetObjectItemCaseSensitive((cJSON *)field, "default") : NULL;
    if (!cJSON_IsObject(field)) {
        cJSON_Delete(settings);
        return fail(reason, reason_size, "not_found");
    }
    if (!default_value) {
        cJSON_Delete(settings);
        return 0;
    }
    char *printed = cJSON_PrintUnformatted(default_value);
    if (!printed) {
        cJSON_Delete(settings);
        return fail(reason, reason_size, "internal");
    }
    size_t len = strlen(printed);
    if (len + 1 > cap) {
        free(printed);
        cJSON_Delete(settings);
        return fail(reason, reason_size, "too_large");
    }
    memcpy(out, printed, len + 1);
    free(printed);
    cJSON_Delete(settings);
    return 1;
}
