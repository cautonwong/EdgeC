/**
 * @file edge_secret_sqlite.h
 * @brief SQLite adapter for the edge_mgmt secret store port.
 *
 * edge_mgmt owns the edge_secret_store_t contract. This adapter lives on the
 * composition-root side so the component never includes sqlite_service.h.
 */

#ifndef EDGE_SECRET_SQLITE_H
#define EDGE_SECRET_SQLITE_H

#include "edgeos/app_secret.h"

struct sqlite_svc_s;
typedef struct sqlite_svc_s sqlite_svc_t;

edge_secret_store_t edge_secret_sqlite(sqlite_svc_t *db);

#endif /* EDGE_SECRET_SQLITE_H */
