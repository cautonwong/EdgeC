/**
 * @file app_repo_sqlite.h
 * @brief SQLite adapter for the edge_mgmt app repository port.
 *
 * The component (edge_mgmt) owns the edge_app_repo_t contract. This adapter
 * lives on the composition-root side so the component never includes
 * sqlite_service.h.
 */

#ifndef APP_REPO_SQLITE_H
#define APP_REPO_SQLITE_H

#include "edgeos/edge_app_mgr.h"

struct sqlite_svc_s;
typedef struct sqlite_svc_s sqlite_svc_t;

edge_app_repo_t app_repo_sqlite(sqlite_svc_t *db);

#endif /* APP_REPO_SQLITE_H */
