/**
 * @file app_settings_sqlite.h
 * @brief Per-app settings schema and values over the control database.
 *
 * The schema column holds the canonical settings object validated by
 * edgeos/app_settings.h. Values are JSON text keyed by (app_id, key).
 */
#ifndef APP_SETTINGS_SQLITE_H
#define APP_SETTINGS_SQLITE_H

#include <stddef.h>

struct sqlite_svc_s;
typedef struct sqlite_svc_s sqlite_svc_t;

/** Persist the canonical schema; NULL deletes the row. Returns 0 or -1. */
int app_settings_schema_save(sqlite_svc_t *db, const char *app_id, const char *schema_json);

/**
 * Validate value_json against the stored schema and persist it.
 * Returns 0, or -1 with a code in reason: invalid, no_schema, unknown_key,
 * readonly, type_mismatch, out_of_range, invalid_value, store_error.
 */
int app_settings_value_set(sqlite_svc_t *db, const char *app_id, const char *key,
                           const char *value_json, char *reason, size_t reason_size);

/**
 * Read one setting. Returns 1 when a value (stored or default) was written,
 * 0 when the field has no value or default, -1 on error with a code in reason:
 * not_found, not_set, too_large, store_error.
 */
int app_settings_value_get(sqlite_svc_t *db, const char *app_id, const char *key, char *out,
                           size_t cap, char *reason, size_t reason_size);

/** Delete the schema and every value for an app. Returns 0 or -1. */
int app_settings_remove(sqlite_svc_t *db, const char *app_id);

#endif /* APP_SETTINGS_SQLITE_H */
