/**
 * @file iems_bmser_ipc.h
 * @brief P3-01b: typed model for the v3 BMSER-BIN northbound IPC device file.
 *
 * Data shape: dev[] { dev_id, module, type, blocks[] { block_id, cycle,
 * sub_id[] { name, type, tag, scale } } }. Each sub_id slot is one value in a
 * block's binary payload, addressed by its 1-based position. The field
 * semantics and wire format are frozen in docs/design/bmser-ipc-contract.md.
 *
 * Trust boundary: iems_bmser_ipc_load accepts the strict form and rejects the
 * values v3 would silently coerce (missing scale becomes 0, cycle 1..999
 * divides by zero, duplicate ids shadow earlier entries). The model owns one
 * heap allocation per array and copies every string; it keeps no pointer into
 * the JSON document.
 */

#ifndef IEMS_BMSER_IPC_H
#define IEMS_BMSER_IPC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "iems_config_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IEMS_BMSER_IPC_FILE "bmser_ipc_bin.json"

/* v3 consumer limits: CHANNEL_DEVS_MAX (tag.h), MAX_MQTT_NORTH_BLOCK_NUM and
 * MAX_MQTT_NORTH_SUB_NUM (ems/mqtt_north.h). A file beyond them is not
 * representable in the v3 process. */
#define IEMS_BMSER_MAX_DEVS 256u
#define IEMS_BMSER_MAX_BLOCKS_PER_DEV 256u
#define IEMS_BMSER_MAX_SUBS_PER_BLOCK 512u

/* v3 buffer lengths: MAX_BASE_SHORT_LEN for dev_id/module/type, name[64] and
 * the "<dev_id>.<tag>" key buffer in mqtt_sub_info_t. */
#define IEMS_BMSER_MAX_ID_LEN 128u
#define IEMS_BMSER_MAX_NAME_LEN 64u
#define IEMS_BMSER_MAX_TAG_LEN 64u
#define IEMS_BMSER_MAX_TAG_KEY_LEN 64u

/* "0x" + 8 hex digits, or 10 decimal digits. */
#define IEMS_BMSER_MAX_BLOCK_ID_TEXT 16u

typedef enum {
    IEMS_BMSER_TYPE_U16 = 0,
    IEMS_BMSER_TYPE_S16,
    IEMS_BMSER_TYPE_U32,
    IEMS_BMSER_TYPE_S32,
    IEMS_BMSER_TYPE_FLOAT,
    IEMS_BMSER_TYPE_BI16,
    IEMS_BMSER_TYPE_ERROR = -1
} iems_bmser_type_t;

typedef struct {
    /* Optional human label. v3 parses the file but never reads this field. */
    char name[IEMS_BMSER_MAX_NAME_LEN];
    iems_bmser_type_t type;
    /* Global tag identity is "<dev_id>.<tag>". Duplicate tags are valid: the
     * v3 file repeats them, and placeholders ("place<N>") resolve to no tag. */
    char tag[IEMS_BMSER_MAX_TAG_LEN];
    /* wire = engineering * scale on GetResp; engineering = wire / scale on Set.
     * Must be finite and non-zero. */
    double scale;
} iems_bmser_sub_t;

typedef struct {
    /* Parsed from a "0x..." hex string or a decimal string. */
    uint32_t block_id;
    /* 0 disables the periodic GetResp push. Otherwise a positive multiple of
     * 1000 ms; v3's 1 s cycle task honors whole seconds only. */
    uint32_t cycle_ms;
    size_t sub_count;
    iems_bmser_sub_t *subs;
} iems_bmser_block_t;

typedef struct {
    char dev_id[IEMS_BMSER_MAX_ID_LEN];
    char module[IEMS_BMSER_MAX_ID_LEN];
    char type[IEMS_BMSER_MAX_ID_LEN];
    size_t block_count;
    iems_bmser_block_t *blocks;
} iems_bmser_dev_t;

typedef struct {
    iems_bmser_dev_t *devs;
    size_t dev_count;
    size_t block_count;
    size_t sub_count;         /* tag slots across all blocks */
    size_t placeholder_count; /* slots whose tag is "place<N>" */
    bool loaded;
} iems_bmser_ipc_t;

/** @return the v3 type string, or NULL for an unknown enum value. */
const char *iems_bmser_type_name(iems_bmser_type_t type);

/**
 * @brief Parse and validate a BMSER-BIN IPC device file.
 *
 * `relative_path` is NULL to read IEMS_BMSER_IPC_FILE. `out` must be zeroed
 * or previously freed; it is zeroed on every failure path, so a failed load
 * still leaves a freeable model. A missing file is an error: v3 treats an
 * unreadable device file as a disabled northbound client.
 *
 * @return 0 on success; -1 with "KIND path: detail" in err on failure.
 */
int iems_bmser_ipc_load(const iems_config_source_t *source, const char *relative_path,
                        iems_bmser_ipc_t *out, char *err, size_t err_size);

/** @brief Release the model's arrays and zero it. Accepts NULL and a zeroed model. */
void iems_bmser_ipc_free(iems_bmser_ipc_t *model);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_BMSER_IPC_H */
