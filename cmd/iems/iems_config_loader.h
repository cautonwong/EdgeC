/**
 * @file iems_config_loader.h
 * @brief P6-04b: load converted device/port config families into the runtime
 *        registry (inventory/poll_tasks) without touching the poll scheduler.
 *
 * Trust boundary: only P1-03b converted artifacts (object root with
 * schema_version "1.0.0") are accepted. The loader revalidates the schema
 * subset it consumes, rejects unknown keys and plaintext secrets, and keeps
 * $secret:/${ENV} references verbatim. It never resolves them.
 */

#ifndef IEMS_CONFIG_LOADER_H
#define IEMS_CONFIG_LOADER_H

#include <stddef.h>
#include <stdint.h>

#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

/* device_id 0 is the tag-level compact view; real devices start at 1 and the
 * DataHub slot formula device_id*16+dense must stay < 1024. */
#define IEMS_CONFIG_MAX_DEVICES 63u
#define IEMS_CONFIG_MAX_POINTS_PER_DEVICE 16u
#define IEMS_CONFIG_MAX_PORTS 64u

typedef enum {
    IEMS_CONFIG_SOURCE_ERROR = -1, /* unexpected failure; loader always fails */
    IEMS_CONFIG_SOURCE_OK = 0,
    IEMS_CONFIG_SOURCE_ABSENT = 1 /* only ENOENT (missing path component) */
} iems_config_source_rc_t;

/* Consumer-defined source port. Paths are relative to the injected root; the
 * loader never reads env.
 *
 * read_file: OK with *data malloc'd and NUL-terminated at *len (loader frees);
 *            ABSENT only when the path is missing; ERROR with
 *            "KIND path: detail" in err for everything else (EACCES, EISDIR,
 *            oversized, short read, allocation failure).
 * list_dir:  OK/ABSENT/ERROR with the same meaning; a non-directory at the
 *            requested path is ERROR, not ABSENT. The loader sorts entries. */
typedef struct {
    iems_config_source_rc_t (*read_file)(void *ctx, const char *relative_path, char **data,
                                        size_t *len, char *err, size_t err_size);
    iems_config_source_rc_t (*list_dir)(void *ctx, const char *relative_dir,
                                        int (*on_entry)(void *cb_ctx, const char *name),
                                        void *cb_ctx, char *err, size_t err_size);
    void *ctx;
} iems_config_source_t;

typedef struct {
    char root[512];
    char real_root[1024]; /* realpath(root); root itself may be a symlink */
} iems_config_file_source_t;

void iems_config_file_source_init(iems_config_file_source_t *fs, const char *root);
iems_config_source_t iems_config_file_source_port(iems_config_file_source_t *fs);

typedef enum {
    IEMS_CONFIG_PORT_UART = 0,
    IEMS_CONFIG_PORT_CAN,
    IEMS_CONFIG_PORT_LAN,
    IEMS_CONFIG_PORT_WAN,
    IEMS_CONFIG_PORT_DI,
    IEMS_CONFIG_PORT_DO
} iems_config_port_kind_t;

typedef struct {
    iems_config_port_kind_t kind;
    char name[48];
    char disp_name[48];
    uint32_t baud;
    uint16_t data_bit;
    uint16_t stop_bit;
    char parity;
    uint16_t filter_count;
    char ip[40];
    char mask[40];
    char gateway[40];
    char metric[16];
    char type[8];
    int ncno;
} iems_config_port_t;

/* name is owned by the catalog and must outlive the model. */
typedef struct {
    uint32_t tag_id;
    const char *name;
    uint16_t dense_idx;
    uint32_t native_id;
    double scale;
} iems_config_point_t;

/* Consumer-defined point directory. The default is the product's static
 * protocol tables; a template-backed implementation lands with P0-03d. */
typedef struct {
    int (*list_points)(void *ctx, const char *protocol, iems_config_point_t *out, size_t cap);
    void *ctx;
} iems_config_point_catalog_t;

const iems_config_point_catalog_t *iems_config_static_catalog(void);

typedef struct {
    uint16_t device_id; /* 1..63, deterministic traversal order */
    char class_type[32];
    char device_type[32]; /* devs[].type, empty when absent */
    char no[48];
    char name[64];
    char channel[64];
    char template_name[96];
    char user_def[128];
    char config_protocol[24];
    char protocol[16]; /* canonical driver name */
    int addr;          /* slave address; missing/"0" becomes 1 */
    int bus_id;        /* -1 = no provisioned SOUTHBOUND bus (UNCHECKED) */
    uint16_t point_count;
    iems_config_point_t points[IEMS_CONFIG_MAX_POINTS_PER_DEVICE];
    char source_path[128];
    uint16_t class_index;
    uint16_t device_index;
} iems_config_device_t;

typedef struct {
    iems_config_port_t ports[IEMS_CONFIG_MAX_PORTS];
    size_t port_count;
    iems_config_device_t devices[IEMS_CONFIG_MAX_DEVICES];
    size_t device_count;
} iems_config_model_t;

/**
 * Parse and validate the configured families. port_config.json and the root
 * device_config.json are required; def/lc/lc_devs files are optional, and
 * lc_devs entries are sorted for stable device ids.
 *
 * @return 0 on success; -1 with "KIND path: detail" in err on failure.
 */
int iems_config_load(const iems_config_source_t *source,
                     const iems_config_point_catalog_t *catalog, iems_config_model_t *out,
                     char *err, size_t err_size);

/**
 * Replace inventory/poll_tasks with the model in one transaction. Devices
 * without a provisioned bus keep an inventory row but get no poll tasks.
 *
 * @return 0 on success; -1 with a SQLite error in err (transaction rolled back).
 */
int iems_config_apply_sqlite(const iems_config_model_t *model, sqlite_svc_t *db, char *err,
                             size_t err_size);

/* Default catalog: points the product already knows for a protocol. */
int iems_config_catalog_static(void *ctx, const char *protocol, iems_config_point_t *out,
                               size_t cap);

/**
 * @brief 组合根便捷入口：文件源 + 静态点位目录 + load + apply。
 *
 * 装配期只调这一个函数；失败信息与 load/apply 相同，供启动链 fail closed。
 */
int iems_config_load_apply(const char *root, sqlite_svc_t *db, char *err, size_t err_size);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_CONFIG_LOADER_H */
