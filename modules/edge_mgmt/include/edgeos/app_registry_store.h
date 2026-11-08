/**
 * @file app_registry_store.h
 * @brief 应用注册表的持久化（属于 edge_mgmt 的这张表，不下沉到 edge_infra）
 */

#ifndef APP_REGISTRY_STORE_H
#define APP_REGISTRY_STORE_H

#include "edgeos/edge_app_mgr.h"
#include "sqlite_service.h"

int app_registry_save(sqlite_svc_t *svc, const app_manifest_t *manifest);
int app_registry_get(sqlite_svc_t *svc, const char *app_id, app_manifest_t *out_manifest);
int app_registry_delete(sqlite_svc_t *svc, const char *app_id);
int app_registry_list(sqlite_svc_t *svc, app_manifest_t *out_apps, int max_count);

#endif  // APP_REGISTRY_STORE_H
