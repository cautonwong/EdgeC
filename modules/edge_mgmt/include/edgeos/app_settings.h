/**
 * @file app_settings.h
 * @brief Manifest settings schema: subset validation, defaults and value checks.
 *
 * The schema is the manifest `settings` object. Accepted field keywords are
 * type, title, description, default, minimum, maximum, minLength, maxLength,
 * enum, secret, unit, group, order, readonly and requiresRestart. Unknown
 * keywords ($ref, oneOf, allOf, patternProperties, pattern, ...) are rejected.
 * Values are JSON text; arrays are limited to array<string>.
 */
#ifndef EDGEOS_APP_SETTINGS_H
#define EDGEOS_APP_SETTINGS_H

#include <stddef.h>

#define APP_SETTING_KEY_SIZE 64
#define APP_SETTING_REASON_SIZE 64
#define APP_SETTING_MAX_FIELDS 128
#define APP_SETTING_ENUM_MAX 64

/**
 * Validate a settings object in JSON text. Accepts both the documented
 * {schemaVersion, fields} shape and a bare key -> field map.
 * Returns 0 or -1 with a field-level reason.
 */
int edge_app_settings_validate_json(const char *settings_json, char *reason, size_t reason_size);

/**
 * Validate and canonicalize a settings object into
 * {"schemaVersion":1,"fields":{...}}. Returns malloc'd JSON (caller frees),
 * or NULL with a reason when the schema is invalid.
 */
char *edge_app_settings_normalize_json(const char *settings_json, char *reason, size_t reason_size);

/**
 * Validate value_json against key in a canonical settings schema.
 * Returns 0, or -1 with one of: unknown_key, readonly, type_mismatch,
 * out_of_range, invalid_value.
 */
int edge_app_setting_validate_value(const char *settings_json, const char *key,
                                    const char *value_json, char *reason, size_t reason_size);

/**
 * Report whether key is declared by the schema. Returns 1 declared, 0 not
 * declared, -1 when the schema cannot be parsed.
 */
int edge_app_setting_declared(const char *settings_json, const char *key);

/**
 * Write the canonical default for key as JSON text.
 * Returns 1 when written, 0 when the field has no default, -1 for an unknown
 * key or a bad schema.
 */
int edge_app_setting_default_json(const char *settings_json, const char *key, char *out, size_t cap,
                                  char *reason, size_t reason_size);

#endif /* EDGEOS_APP_SETTINGS_H */
