/**
 * @file edge_app_task_sqlite.h
 * @brief SQLite adapter for the edge_mgmt app task store port.
 *
 * edge_mgmt owns the edge_app_task_store_t contract. This adapter lives on the
 * composition-root side so the component never includes sqlite_service.h.
 */

#ifndef EDGE_APP_TASK_SQLITE_H
#define EDGE_APP_TASK_SQLITE_H

#include "edgeos/app_task.h"

struct sqlite_svc_s;
typedef struct sqlite_svc_s sqlite_svc_t;

edge_app_task_store_t edge_app_task_sqlite(sqlite_svc_t *db);

#endif /* EDGE_APP_TASK_SQLITE_H */
