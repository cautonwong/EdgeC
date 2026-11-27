/**
 * @file iems_lc_user_cfg.h
 * @brief P1-07a: typed per-LC point overrides from converted lc_user_cfg.json.
 *
 * Data shape: one fixed-size record per lc_devs/<LC> directory; each entry is
 * a tagged (type, name, value) triple. There is no dynamic map, and an ABSENT
 * file stays distinguishable from an empty one (state).
 *
 * Trust boundary: only P1-03b converted artifacts are accepted: object root
 * {"schema_version":"1.0.0","config":[...]}, item type 0 carries int_value and
 * type 1 carries double_value, unknown keys and non-finite doubles are
 * rejected. A missing file is ABSENT: the LC stays enumerated and empty and
 * startup continues. Present but invalid content is an error and the caller
 * must not apply the model. v3's 125 default merge (vlc_cfg.c) is NOT applied
 * here: a missing PowerLimit/Installedcab_coeff stays missing for the consumer.
 */

#ifndef IEMS_LC_USER_CFG_H
#define IEMS_LC_USER_CFG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "iems_config_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

/* same cap as the device loader's lc_devs enumeration */
#define IEMS_LC_MAX_LCS 32u
/* per-LC override count; lc.schema.json has no maxItems */
#define IEMS_LC_MAX_ENTRIES 32u
/* device loader's lc_entries_t name buffer */
#define IEMS_LC_DIR_NAME_MAX 64u
/* v3 vlc_cfg_it.name */
#define IEMS_LC_ENTRY_NAME_MAX 96u

typedef enum {
    IEMS_LC_VALUE_INT = 0,
    IEMS_LC_VALUE_DOUBLE = 1,
} iems_lc_value_type_t;

typedef struct {
    char name[IEMS_LC_ENTRY_NAME_MAX];
    iems_lc_value_type_t type;
    int32_t int_value;
    double double_value;
} iems_lc_entry_t;

typedef enum {
    IEMS_LC_FILE_ABSENT = 0,
    IEMS_LC_FILE_LOADED = 1,
} iems_lc_file_state_t;

typedef struct {
    char name[IEMS_LC_DIR_NAME_MAX];
    iems_lc_entry_t entries[IEMS_LC_MAX_ENTRIES];
    size_t count;
    iems_lc_file_state_t state;
} iems_lc_user_cfg_t;

typedef struct {
    iems_lc_user_cfg_t lcs[IEMS_LC_MAX_LCS];
    size_t count;
    /* true only when lc_devs was listed and every enumerated entry supplied a
     * valid file; false when any file (or lc_devs itself) is missing. */
    bool loaded;
} iems_lc_user_cfgs_t;

/* NULL when the LC name was not enumerated. ABSENT LCs are still enumerated:
 * cfg->state tells "missing file" from "empty file". */
const iems_lc_user_cfg_t *iems_lc_user_cfgs_find(const iems_lc_user_cfgs_t *cfgs,
                                                const char *lc_name);

/* NULL when the entry is absent; a present entry exposes type and value, so
 * a consumer can tell a missing PowerLimit from a configured 0. */
const iems_lc_entry_t *iems_lc_user_cfg_find(const iems_lc_user_cfg_t *cfg, const char *name);

/**
 * @brief Load lc_devs/<LC>/lc_user_cfg.json from the injected source.
 *
 * Sorted enumeration via list_dir("lc_devs") then one file per entry. ABSENT
 * lc_devs means no LCs (rc 0, count 0). A missing LC file is ABSENT: the LC
 * stays enumerated, empty, and startup continues. Invalid content or a source
 * error is -1 with "KIND path: field detail" in err and leaves *out
 * byte-identical (the model is assembled fresh and written once at the end).
 *
 * @return 0 on success (including ABSENT files); -1 otherwise.
 */
int iems_lc_user_cfgs_load(const iems_config_source_t *source, iems_lc_user_cfgs_t *out,
                           char *err, size_t err_size);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_LC_USER_CFG_H */
