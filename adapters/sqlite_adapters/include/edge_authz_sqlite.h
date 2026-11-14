/**
 * @file edge_authz_sqlite.h
 * @brief SQLite adapter for the edge_mgmt authorization repository port.
 *
 * edge_mgmt owns the edge_authz_repo_t contract. This adapter lives on the
 * composition-root side so the component never includes sqlite_service.h.
 */

#ifndef EDGE_AUTHZ_SQLITE_H
#define EDGE_AUTHZ_SQLITE_H

#include "edgeos/edge_authz.h"

struct sqlite_svc_s;
typedef struct sqlite_svc_s sqlite_svc_t;

edge_authz_repo_t edge_authz_sqlite(sqlite_svc_t *db);
edge_authz_store_t edge_authz_sqlite_store(sqlite_svc_t *db);

#endif /* EDGE_AUTHZ_SQLITE_H */
