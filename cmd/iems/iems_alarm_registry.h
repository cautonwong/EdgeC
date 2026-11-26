/**
 * @file iems_alarm_registry.h
 * @brief P5-04a: typed alarm definition registry loaded from v3 alarm_def.json.
 *
 * Data shape: one flat definition array in file order plus a vendor_model
 * table that keeps every file entry, including entries whose tags array is
 * empty. Strings live in registry-owned storage, so a loaded registry must
 * not be copied or moved and returned definitions must not outlive it.
 *
 * Trust boundary: only the repository alarm_def.json format is accepted.
 * Parsing is strict JSON (unknown keys, duplicate keys, wrong types, control
 * characters and bad UTF-8 are rejected). The model stores the fields this
 * slice needs; additionalInfo and additionalInfoParas are type-checked at the
 * boundary and discarded. Alarm identity is (alarmCode, reasonCode), mirroring
 * v3's "<alarmCode>.<reasonCode>" code key (alarm_init.c:284); tag lookup also
 * matches class_type, mirroring v3's "<type>.<vendor>.<model>.<tag>" info key
 * (alarm_init.c:294).
 *
 * Real-file evidence (zh_CN, 2026-11-24 snapshot): 16 vendor_model entries
 * (15 with tags), 3821 definitions, 1 vendor, 5 distinct vendor/model pairs,
 * 174 distinct (vendor, model, class_type, tag) keys, 51 distinct alarm codes
 * and 3819 distinct (alarmCode, reasonCode) pairs (two pairs repeat). The file
 * repeats (vendor, model, tag) and alarmCode within an entry (PCS FaultCode1
 * carries 365 reasonCode variants), and only file order separates the
 * variants. Lookups return the first match in file order; the protocol variant
 * index is not modeled in this slice. Caps below leave headroom over that
 * snapshot and overflow fails the whole load closed.
 */

#ifndef IEMS_ALARM_REGISTRY_H
#define IEMS_ALARM_REGISTRY_H

#include <stddef.h>
#include <stdint.h>

#include "iems_config_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IEMS_ALARM_MAX_DEFS 4096u
#define IEMS_ALARM_MAX_VENDOR_MODELS 32u
#define IEMS_ALARM_MAX_STRINGS (3u * 1024u * 1024u)
#define IEMS_ALARM_LANG_MAX 16u

/** @brief One alarm definition, named after the JSON fields it comes from. */
typedef struct {
    const char *vendor;     /* vendor_models[].vendor */
    const char *model;      /* vendor_models[].model */
    const char *class_type; /* vendor_models[].type */
    const char *tag;        /* tags[].tag */
    uint32_t code;          /* tags[].alarmCode, >= 0 */
    int32_t reason_code;    /* tags[].reasonCode, >= 0 */
    const char *name;       /* tags[].alarmInfo */
    const char *category;   /* tags[].categoryDisplay */
    const char *severity;   /* tags[].severityDisplay, may be empty */
    const char *entity;     /* tags[].entityTypeDisplay */
    const char *reason;     /* tags[].reason */
    const char *solution;   /* tags[].solution, may be empty */
} iems_alarm_def_t;

/** @brief One vendor_models[] entry; vendor/model/class_type are shared by its defs. */
typedef struct {
    const char *vendor;
    const char *model;
    const char *class_type;
    uint32_t def_start;
    uint32_t def_count; /* 0 for entries without tags */
} iems_alarm_vendor_model_t;

typedef struct {
    char lang[IEMS_ALARM_LANG_MAX];
    iems_alarm_vendor_model_t vendor_models[IEMS_ALARM_MAX_VENDOR_MODELS];
    uint32_t vendor_model_count;
    iems_alarm_def_t defs[IEMS_ALARM_MAX_DEFS];
    uint32_t def_count;
    char strings[IEMS_ALARM_MAX_STRINGS];
    uint32_t strings_used;
} iems_alarm_registry_t;

/**
 * @brief Load one language's alarm_def.json through the injected source port.
 *
 * Reads the fixed relative path "alarm_def.json". On success every count,
 * string and lookup is populated. On failure the registry is emptied
 * (counts zero, lang empty) and every lookup returns NULL.
 *
 * The shared iems_config_file_source_t rejects files over 1 MiB and the real
 * alarm file is ~3.1 MiB, so composition must inject a source that accepts
 * it; the loader itself has no file-size assumption beyond the arena cap.
 *
 * @return 0 on success; -1 with "KIND path: detail" in err on failure.
 */
int iems_alarm_registry_load(const iems_config_source_t *source, iems_alarm_registry_t *out,
                             char *err, size_t err_size);

/**
 * @brief First file-order match for (class_type, vendor, model, tag).
 *
 * Mirrors v3's "<type>.<vendor>.<model>.<tag>" info key.
 * @return definition pointer, or NULL when absent or any argument is NULL.
 */
const iems_alarm_def_t *iems_alarm_registry_find(const iems_alarm_registry_t *reg,
                                                 const char *class_type, const char *vendor,
                                                 const char *model, const char *tag);

/**
 * @brief First file-order match for (alarmCode, reasonCode).
 *
 * Mirrors v3's "<alarmCode>.<reasonCode>" code key.
 * @return definition pointer, or NULL when absent.
 */
const iems_alarm_def_t *iems_alarm_registry_find_by_code(const iems_alarm_registry_t *reg,
                                                         uint32_t code, int32_t reason_code);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_ALARM_REGISTRY_H */
