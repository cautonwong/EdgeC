/**
 * @file app_points_sqlite.h
 * @brief App point contracts and slot allocation over the control database.
 *
 * Each app owns one contiguous block of 256 slots in the 4096-slot app region.
 * The product computes app_tag = app_base + slot. Names, not slots, are the
 * public interface; slots are device-local and reclaimed on uninstall.
 */
#ifndef APP_POINTS_SQLITE_H
#define APP_POINTS_SQLITE_H

#include <stddef.h>

#include "edgeos/app_manifest.h"

struct sqlite_svc_s;
typedef struct sqlite_svc_s sqlite_svc_t;

#define APP_POINT_SLOT_BLOCK 256
#define APP_POINT_SLOT_BLOCKS 16
#define APP_POINT_SLOT_TOTAL (APP_POINT_SLOT_BLOCK * APP_POINT_SLOT_BLOCKS)

typedef struct {
    char name[APP_SPEC_POINT_NAME_SIZE];
    char unit[APP_SPEC_POINT_UNIT_SIZE];
    int slot;
    int type;
    int contract_version;
    int retention;
    int stale_after_ms;
    int stability;
    int source;
} app_point_row_t;

/**
 * Reconcile an app's contributed points with its declarations.
 *
 * The first call allocates a free 256-slot block. Existing names keep their
 * slots; type, unit and contractVersion changes are rejected; undeclared names
 * are removed. count == 0 releases the block. Returns 0 or -1 with a reason.
 */
int app_points_sync(sqlite_svc_t *db, const char *app_id, const app_point_spec_t *points,
                    size_t count, char *reason, size_t reason_size);

/**
 * Dry run for install/upgrade: checks contract changes and slot capacity
 * without writing. Returns 0 when sync would succeed.
 */
int app_points_check(sqlite_svc_t *db, const char *app_id, const app_point_spec_t *points,
                     size_t count, char *reason, size_t reason_size);

/** Release the app's point rows and slot block. */
int app_points_release(sqlite_svc_t *db, const char *app_id);

/** List points ordered by slot. out may be NULL to count only. */
int app_points_list(sqlite_svc_t *db, const char *app_id, app_point_row_t *out, size_t cap,
                    size_t *count, char *reason, size_t reason_size);

/**
 * Resolve a full point name "app.<app>.<point>" to its owner and slot.
 * Returns 1 when found, 0 when absent, -1 on error.
 */
int app_points_find_full(sqlite_svc_t *db, const char *full_name, char *app_id,
                         size_t app_id_size, int *slot);

/**
 * Resolve a slot back to its owner and short name.
 * Returns 1 when found, 0 when absent, -1 on error.
 */
int app_points_find_slot(sqlite_svc_t *db, int slot, char *app_id, size_t app_id_size,
                         char *name, size_t name_size);

/** Drop points and blocks for apps with no registry row or an uninstalled one. */
int app_points_reconcile(sqlite_svc_t *db, size_t *released, char *reason, size_t reason_size);

#endif /* APP_POINTS_SQLITE_H */
