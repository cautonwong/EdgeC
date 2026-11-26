/**
 * @file iems_config_loader_priv.h
 * @brief Private strict-parsing boundary shared by the config loaders.
 *
 * These helpers are the loader's trust boundary (strict JSON precheck, root
 * version, key whitelist, typed field reads). They are exported only inside
 * cmd/iems so iems_pcs_settings.c validates converted ems/lc files with the
 * same rules as device/port loading. Not part of any public API.
 */

#ifndef IEMS_CONFIG_LOADER_PRIV_H
#define IEMS_CONFIG_LOADER_PRIV_H

#include <stddef.h>

#include <cjson/cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

int iems_config_fail(char *err, size_t err_size, const char *kind, const char *path,
                     const char *detail);
void iems_config_path_key(char *buf, size_t cap, const char *base, const char *key);
void iems_config_path_index(char *buf, size_t cap, const char *base, size_t index);
int iems_config_sensitive_scan(cJSON *object, const char *path, char *err, size_t err_size);
int iems_config_check_keys(cJSON *object, const char *path, const char *const *allowed,
                           size_t allowed_count, char *err, size_t err_size);
int iems_config_check_root(cJSON *root, const char *rel, char *err, size_t err_size);
int iems_config_get_int(cJSON *object, const char *key, int required, int minimum,
                        const char *path, char *err, size_t err_size, int *out);
int iems_config_strict_parse(const char *data, size_t len, const char *rel, cJSON **out,
                             char *err, size_t err_size);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_CONFIG_LOADER_PRIV_H */
