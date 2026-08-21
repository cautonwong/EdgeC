/**
 * @file edge_infra_base.c
 * @brief Base runtime anchor and graceful degradation stubs for edge_infra
 *
 * @note 优雅降级与错误传播语义 (Fallback Error Propagation Semantics):
 * 当通过 Kconfig 禁用某个后端（如 CONFIG_EDGE_INFRA_SQLITE=n）时，此文件提供合规的符号桩。
 * 1. 构造函数（如 sqlite_svc_create）返回 NULL；
 * 2. 所有写/查询/事务操作统一返回明确错误码（-1 / NULL / 0 count），杜绝假成功，确保调用方能够感知后端未启用；
 * 3. 析构函数（如 sqlite_svc_destroy）作为无状态安全函数保持空操作。
 */

#include "edge_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *edge_infra_get_version(void) {
    return "2.0.0-modular";
}

/* ---------------- Fallback Stubs when Backends are Disabled via Kconfig ---------------- */

#ifndef CONFIG_EDGE_INFRA_SQLITE
#include "sqlite_service.h"
#include "storage_ops.h"

sqlite_svc_t *sqlite_svc_create(uv_loop_t *loop, sqlite_config_t *cfg) {
    (void)loop; (void)cfg;
    return NULL;
}

int sqlite_svc_init_from_contract(sqlite_svc_t *svc, const edge_contract_t *contract) {
    (void)svc; (void)contract;
    return -1;
}

int sqlite_svc_insert(sqlite_svc_t *svc, const PipeMsg *msg) {
    (void)svc; (void)msg;
    return -1;
}

int sqlite_svc_query(sqlite_svc_t *svc, uint64_t start_ts, uint64_t end_ts, sqlite_svc_query_cb cb, void *user_data) {
    (void)svc; (void)start_ts; (void)end_ts; (void)cb; (void)user_data;
    return -1;
}

int sqlite_svc_begin_trans(sqlite_svc_t *svc) {
    (void)svc;
    return -1;
}

int sqlite_svc_commit_trans(sqlite_svc_t *svc) {
    (void)svc;
    return -1;
}

int sqlite_svc_exec(sqlite_svc_t *svc, const char *sql, sqlite_exec_cb cb, void *user_data) {
    (void)svc; (void)sql;
    if (cb) cb(user_data, -1, NULL);
    return -1;
}

void sqlite_svc_destroy(sqlite_svc_t *svc) {
    (void)svc;
}

int sqlite_svc_set_param(sqlite_svc_t *svc, const char *product, uint32_t tag_id, const void *val, size_t len) {
    (void)svc; (void)product; (void)tag_id; (void)val; (void)len;
    return -1;
}

int sqlite_svc_get_param(sqlite_svc_t *svc, const char *product, uint32_t tag_id, void *buf, size_t buflen) {
    (void)svc; (void)product; (void)tag_id; (void)buf; (void)buflen;
    return -1;
}

void *sqlite_svc_get_db_handle(sqlite_svc_t *svc) {
    (void)svc;
    return NULL;
}

int sqlite_svc_save_app(sqlite_svc_t *svc, const void *manifest_ptr) {
    (void)svc; (void)manifest_ptr;
    return -1;
}

int sqlite_svc_get_app(sqlite_svc_t *svc, const char *app_id, void *out_manifest_ptr) {
    (void)svc; (void)app_id; (void)out_manifest_ptr;
    return -1;
}

int sqlite_svc_delete_app(sqlite_svc_t *svc, const char *app_id) {
    (void)svc; (void)app_id;
    return -1;
}

int sqlite_svc_list_apps(sqlite_svc_t *svc, void *manifests_array_ptr, int max_count) {
    (void)svc; (void)manifests_array_ptr; (void)max_count;
    return 0; // count = 0
}

const storage_ops_t g_sqlite_storage_ops = {0};

storage_driver_t *storage_driver_sqlite_create(uv_loop_t *loop, const storage_config_t *cfg) {
    (void)loop; (void)cfg;
    return NULL;
}
#endif

#ifndef CONFIG_EDGE_INFRA_DDS
#include "datahub_dds.h"
const datahub_ops_t g_datahub_dds_ops = {0};
int datahub_dds_init(datahub_t *hub, const datahub_config_t *config) {
    (void)hub; (void)config;
    return -1;
}
#endif

#ifndef CONFIG_EDGE_INFRA_MQTT
#include "datahub_mqtt.h"
const datahub_ops_t g_datahub_mqtt_ops = {0};
int datahub_mqtt_init(datahub_t *hub, const datahub_config_t *config) {
    (void)hub; (void)config;
    return -1;
}
#endif
