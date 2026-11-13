#include "rest_internal.h"
#include "rest_server.h"
#include <uv.h>

#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "arbiter.h"
#include "contract_data_types.h"
#include "datahub.h"
#include "edge_log.h"
#include "edge_params.h"
#include "edge_service.h"
#include "edge_version.h"
#include "rest_control_port.h"

#define REST_MAX_CLIENTS REST_DEFAULT_MAX_CLIENTS
#define REST_RX_BUF_SIZE 4096
#define REST_TX_BUF_SIZE 65536
#define REST_MAX_METHOD 16
#define REST_MAX_PATH 128
#define REST_MAX_BIND_ADDR 64
#define REST_MAX_CORS_ORIGIN 128

typedef struct {
    char method[REST_MAX_METHOD];
    char path[REST_MAX_PATH];
    size_t content_length;
    size_t header_bytes;
    size_t body_bytes;
    uint64_t deadline_ms;
} rest_client_state_t;

typedef struct rest_client_s {
    uv_tcp_t tcp;
    rest_server_t *server;
    bool in_use;
    bool headers_done;
    bool responded;
    bool close_after_write;
    uv_write_t write_req;
    char *dyn_tx_buf;
    char *header_buf;
    size_t header_len;
    size_t header_cap;
    char *body_buf;
    rest_client_state_t state;
    char rx_buf[REST_RX_BUF_SIZE];
    char tx_buf[REST_TX_BUF_SIZE];
} rest_client_t;

struct rest_server_s {
    uv_loop_t *loop;
    service_t *svc;
    int port;
    uv_tcp_t server;
    uv_timer_t timeout_timer;
    bool is_allocated;
    bool timer_init;
    rest_limits_t limits;
    char bind_addr[REST_MAX_BIND_ADDR];
    char cors_origin[REST_MAX_CORS_ORIGIN];
    rest_client_t clients[REST_MAX_CLIENTS];
};

// --- SQL 回调 ---
typedef struct {
    cJSON *array;
    int count;
} query_ctx_t;
static int on_sqlite_row(void *user_data, PipeMsg *msg) {
    query_ctx_t *ctx = (query_ctx_t *)user_data;
    cJSON *item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "ts", (double)msg->ts);
    cJSON_AddNumberToObject(item, "val", msg->payload.val.f);
    cJSON_AddItemToArray(ctx->array, item);
    ctx->count++;
    return 0;
}

static uint64_t get_query_param_uint(const char *path, const char *key, uint64_t default_val) {
    char search_key[32];
    snprintf(search_key, sizeof(search_key), "%s=", key);
    char *p = strstr(path, search_key);
    if (!p) return default_val;
    return (uint64_t)atoll(p + strlen(search_key));
}

// --- API Handlers ---

static char *handle_system_health(service_t *svc) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddNumberToObject(root, "uptime", (double)uv_now(service_get_loop(svc)) / 1000);
    cJSON_AddStringToObject(root, "version", (const char *)edge_get_version());

    datahub_t *hub = service_get_datahub(svc);
    struct net_buf_pool *io_pool = datahub_get_io_pool(hub);
    if (io_pool) {
        net_buf_pool_stats_t stats;
        net_buf_pool_audit(io_pool, 0, &stats);
        cJSON *pool = cJSON_AddObjectToObject(root, "net_buf_pool");
        cJSON_AddNumberToObject(pool, "total", stats.total_count);
        cJSON_AddNumberToObject(pool, "free", stats.free_count);
        cJSON_AddNumberToObject(pool, "peak", stats.peak_usage);
    }
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static char *handle_system_stats(service_t *svc) {
    cJSON *root = cJSON_CreateObject();

    // 注入无法伪造的生命证明：高精度 Uptime

    double uptime = (double)uv_now(service_get_loop(svc)) / 1000.0;

    cJSON_AddNumberToObject(root, "uptime_sec_proof", uptime);

    cJSON_AddStringToObject(root, "engine_status", "ALIVE_AND_KICKING");

    // 1. DataHub

    datahub_t *hub = service_get_datahub(svc);
    if (hub) {
        cJSON *dh = cJSON_AddObjectToObject(root, "datahub");
        cJSON_AddNumberToObject(dh, "total_writes", (double)datahub_get_ring_total_writes(hub));
        cJSON_AddNumberToObject(dh, "listener_count", (double)datahub_get_listener_count(hub));
    }

    // 2. Arbiter
    arbiter_t *arb = service_get_arbiter(svc);
    if (arb) {
        arbiter_bus_status_t bus_stats[32];  // 限制上报点位数量，防止栈溢出
        int count = arbiter_get_statuses(arb, bus_stats, 32);
        cJSON *bus_arr = cJSON_AddArrayToObject(root, "buses");
        for (int i = 0; i < count; i++) {
            cJSON *b = cJSON_CreateObject();
            cJSON_AddNumberToObject(b, "id", bus_stats[i].bus_id);
            cJSON_AddBoolToObject(b, "busy", bus_stats[i].is_busy);
            cJSON_AddNumberToObject(b, "queue", bus_stats[i].queue_len);
            cJSON_AddItemToArray(bus_arr, b);
        }
    }

    // 3. Pipeline (Stage 11.2)
    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    if (control && control->stream_topology_json) {
        char *engine_json = control->stream_topology_json(control->ctx);
        if (engine_json) {
            cJSON *ej = cJSON_Parse(engine_json);
            if (ej) cJSON_AddItemToObject(root, "engine", ej);
            free(engine_json);
        }
    }

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static char *handle_contracts_get(service_t *svc) {
    const char *path = service_get_contract_path(svc);
    if (!path || access(path, F_OK) != 0) return strdup("{\"error\":\"not_found\"}");
    FILE *f = fopen(path, "r");
    if (!f) return strdup("{\"error\":\"read_failed\"}");
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)len + 1);
    fread(buf, 1, (size_t)len, f);
    fclose(f);
    buf[len] = '\0';
    return buf;
}

static char *handle_devices_get(service_t *svc) {
    (void)svc;
    cJSON *root = cJSON_CreateArray();
    const char *mock_ids[] = {"meter_01", "pcs_01"};
    for (int i = 0; i < 2; i++) {
        cJSON *dev = cJSON_CreateObject();
        cJSON_AddStringToObject(dev, "id", mock_ids[i]);
        cJSON_AddStringToObject(dev, "status", "online");
        cJSON_AddItemToArray(root, dev);
    }
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static bool g_rest_write_enabled = false;

void rest_server_enable_write(bool enable) {
    g_rest_write_enabled = enable;
}

static bool g_rest_legacy_upload = false;

void rest_server_set_legacy_upload(bool enable) {
    g_rest_legacy_upload = enable;
}

static char *handle_data_latest(service_t *svc, uint32_t tag_id) {
    datahub_t *hub = service_get_datahub(svc);
    PipeMsg msg;
    cJSON *root = cJSON_CreateObject();
    int rc = datahub_read_shadow(hub, tag_id, &msg);
    if (rc == 0) {
        LOG_INFO(LOG_CAT_CORE, "[TRACE-REST] Req Tag %u -> Found in Shadow: Tag %u, Val %.2f",
                 tag_id, msg.tag_id, msg.payload.val.f);
        cJSON_AddNumberToObject(root, "tag_id", msg.tag_id);
        cJSON_AddNumberToObject(
            root, "value",
            (msg.type == PIPEMSG_TYPE_SCALAR) ? msg.payload.val.f : (double)msg.payload.val.i);
        cJSON_AddNumberToObject(root, "ts", (double)msg.ts);
    } else {
        LOG_WARN(LOG_CAT_CORE, "[TRACE-REST] Req Tag %u -> NOT FOUND in Shadow (rc=%d)", tag_id,
                 rc);
        cJSON_AddStringToObject(root, "status", "not_found");
    }
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

#include "edge_api.h"

static char *handle_data_write(service_t *svc, const char *body) {
    if (!g_rest_write_enabled)
        return strdup("{\"status\":\"error\",\"reason\":\"write_disabled\"}");

    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *dev_item = cJSON_GetObjectItem(root, "dev_id");
    cJSON *tag_item = cJSON_GetObjectItem(root, "tag");
    cJSON *val_item = cJSON_GetObjectItem(root, "value");

    if (!dev_item || !tag_item || !val_item) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }

    // [Omnis] 简化实现：默认使用总线 0 和 ModbusRTU
    // 实际生产中应从 inventory 查找 dev_id 对应的 bus 和 protocol
    uint32_t tag_id = 0;
    if (cJSON_IsNumber(tag_item)) {
        tag_id = (uint32_t)tag_item->valuedouble;
    } else if (cJSON_IsString(tag_item)) {
        tag_id = (uint32_t)atoi(tag_item->valuestring);
    }

    double val = val_item->valuedouble;

    edge_api_write_tag(0, "ModbusRTU", 1, tag_id, val, NULL, NULL);

    cJSON_Delete(root);
    return strdup("{\"status\":\"ok\"}");
}

static uint8_t g_last_raw_rx[1024];
static size_t g_last_raw_rx_len = 0;

static void on_raw_io_complete(int status, net_buf_t *rx_buf) {
    if (status == 0 && rx_buf) {
        g_last_raw_rx_len = rx_buf->b.len > 1024 ? 1024 : rx_buf->b.len;
        memcpy(g_last_raw_rx, rx_buf->b.data, g_last_raw_rx_len);
        printf("[REST-DEBUG] Captured Raw RX: %zu bytes\n", g_last_raw_rx_len);
    }
}

static char *handle_io_raw(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *bus_item = cJSON_GetObjectItem(root, "bus_id");
    cJSON *payload_item = cJSON_GetObjectItem(root, "payload");

    if (!bus_item || !payload_item) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }

    int bus_id = bus_item->valueint;
    const char *hex = payload_item->valuestring;
    size_t hex_len = strlen(hex);

    datahub_t *hub = service_get_datahub(svc);
    net_buf_t *tx = net_buf_alloc(datahub_get_io_pool(hub), 0);

    for (size_t i = 0; i < hex_len; i += 2) {
        unsigned int b;
        sscanf(hex + i, "%02x", &b);
        net_buf_simple_add_u8(&tx->b, (uint8_t)b);
    }

    g_last_raw_rx_len = 0;  // Clear previous
    edge_api_raw_io(bus_id, tx, on_raw_io_complete);

    cJSON_Delete(root);
    return strdup("{\"status\":\"ok\"}");
}

static char *handle_io_raw_latest(void) {
    cJSON *root = cJSON_CreateObject();
    if (g_last_raw_rx_len > 0) {
        char *hex = malloc(g_last_raw_rx_len * 2 + 1);
        for (size_t i = 0; i < g_last_raw_rx_len; i++) {
            sprintf(hex + i * 2, "%02X", g_last_raw_rx[i]);
        }
        cJSON_AddStringToObject(root, "payload", hex);
        free(hex);
    } else {
        cJSON_AddStringToObject(root, "payload", "");
    }
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static char *handle_buffer_snapshot(service_t *svc) {
    (void)svc;
    // A placeholder for high-frequency vibration data buffer.
    // In a real system, this would fetch from a DMA ring buffer or circular array.
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddStringToObject(root, "description", "High-frequency vibration snapshot (Mock)");
    cJSON *data_arr = cJSON_CreateArray();
    for (int i = 0; i < 100; i++) {
        cJSON_AddItemToArray(data_arr, cJSON_CreateNumber((double)i * 0.1));
    }
    cJSON_AddItemToObject(root, "data", data_arr);
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static char *handle_stream_topology(service_t *svc) {
    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    if (control && control->stream_topology_json) {
        char *json = control->stream_topology_json(control->ctx);
        if (json) return json;
    }
    return strdup("{\"error\":\"engine_not_ready\"}");
}

static char *handle_apps_list(service_t *svc) {
    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    char *json = (control && control->apps_json) ? control->apps_json(control->ctx) : NULL;
    return json ? json : strdup("[]");
}

static char *handle_apps_start(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");
    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    if (!id_item || !id_item->valuestring) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_app_id\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    int rc = (control && control->start_app)
                 ? control->start_app(control->ctx, id_item->valuestring)
                 : -1;

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"start_failed\"}");
}

static char *handle_apps_stop(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");
    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    if (!id_item || !id_item->valuestring) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_app_id\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    int rc = (control && control->stop_app)
                 ? control->stop_app(control->ctx, id_item->valuestring)
                 : -1;

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"stop_failed\"}");
}

static char *handle_apps_uninstall(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");
    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    cJSON *disposition_item = cJSON_GetObjectItem(root, "disposition");
    cJSON *export_item = cJSON_GetObjectItem(root, "export_path");
    if (!cJSON_IsString(id_item) || (disposition_item && !cJSON_IsString(disposition_item)) ||
        (export_item && !cJSON_IsString(export_item))) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_app_id\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    int rc = (control && control->uninstall_app)
                 ? control->uninstall_app(control->ctx, id_item->valuestring,
                                          cJSON_IsString(disposition_item)
                                              ? disposition_item->valuestring
                                              : "keep",
                                          cJSON_IsString(export_item) ? export_item->valuestring
                                                                      : NULL)
                 : -1;

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"uninstall_rejected\"}");
}

/* Decode an even-length hex payload; caller frees. NULL when invalid. */
uint8_t *rest_hex_decode(const char *hex, size_t *out_len) {
    *out_len = 0;
    if (!hex) return NULL;
    size_t hex_len = strlen(hex);
    if (hex_len == 0 || (hex_len % 2) != 0) return NULL;
    uint8_t *buf = malloc(hex_len / 2 + 1);
    if (!buf) return NULL;
    size_t len = 0;
    for (size_t i = 0; i < hex_len; i += 2) {
        unsigned int b = 0;
        if (sscanf(hex + i, "%2x", &b) != 1) {
            free(buf);
            return NULL;
        }
        buf[len++] = (uint8_t)b;
    }
    *out_len = len;
    return buf;
}

static char *handle_apps_upgrade(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");
    cJSON *manifest_item = cJSON_GetObjectItem(root, "manifest");
    if (!cJSON_IsString(manifest_item) || !manifest_item->valuestring[0]) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_manifest\"}");
    }

    cJSON *payload_item = cJSON_GetObjectItem(root, "payload_hex");
    cJSON *key_item = cJSON_GetObjectItem(root, "idempotency_key");
    if ((payload_item && !cJSON_IsString(payload_item)) ||
        (key_item && !cJSON_IsString(key_item))) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
    }

    uint8_t *wasm_buf = NULL;
    size_t wasm_len = 0;
    if (payload_item) {
        wasm_buf = rest_hex_decode(payload_item->valuestring, &wasm_len);
        if (!wasm_buf) {
            cJSON_Delete(root);
            return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
        }
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    int rc = (control && control->upgrade_app)
                 ? control->upgrade_app(control->ctx, manifest_item->valuestring, wasm_buf,
                                        wasm_len, cJSON_IsString(key_item)
                                                       ? key_item->valuestring
                                                       : NULL)
                 : -1;

    free(wasm_buf);
    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"upgrade_rejected\"}");
}

static char *handle_apps_rollback(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");
    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    if (!cJSON_IsString(id_item) || !id_item->valuestring[0]) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_app_id\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    int rc = (control && control->rollback_app)
                 ? control->rollback_app(control->ctx, id_item->valuestring)
                 : -1;

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"rollback_rejected\"}");
}

static char *handle_apps_grant(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    cJSON *cap_item = cJSON_GetObjectItem(root, "capability");
    cJSON *scope_item = cJSON_GetObjectItem(root, "scope");
    if (!cJSON_IsString(id_item) || !cJSON_IsString(cap_item) || !cJSON_IsString(scope_item)) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }

    cJSON *purpose_item = cJSON_GetObjectItem(root, "purpose");
    cJSON *expires_item = cJSON_GetObjectItem(root, "expires_at");
    cJSON *sensitivity_item = cJSON_GetObjectItem(root, "sensitivity_max");
    cJSON *actor_item = cJSON_GetObjectItem(root, "actor");
    const char *purpose = cJSON_IsString(purpose_item) ? purpose_item->valuestring : "";
    uint64_t expires_at = cJSON_IsNumber(expires_item) ? (uint64_t)expires_item->valuedouble : 0;
    uint8_t sensitivity =
        cJSON_IsNumber(sensitivity_item) ? (uint8_t)sensitivity_item->valueint : 0;
    const char *actor = cJSON_IsString(actor_item) ? actor_item->valuestring : "admin";

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    int rc = (control && control->approve_grant)
                 ? control->approve_grant(control->ctx, id_item->valuestring,
                                          cap_item->valuestring, scope_item->valuestring, purpose,
                                          expires_at, sensitivity, actor)
                 : -1;

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"grant_rejected\"}");
}

static char *handle_apps_revoke_grant(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    cJSON *cap_item = cJSON_GetObjectItem(root, "capability");
    if (!cJSON_IsString(id_item) || !cJSON_IsString(cap_item)) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    int rc = (control && control->revoke_grant)
                 ? control->revoke_grant(control->ctx, id_item->valuestring,
                                         cap_item->valuestring)
                 : -1;

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"revoke_rejected\"}");
}

static char *handle_apps_grants(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    const char *app_id = "";
    if (root) {
        cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
        if (cJSON_IsString(id_item)) app_id = id_item->valuestring;
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    char *json = (control && control->grants_json) ? control->grants_json(control->ctx, app_id)
                                                   : NULL;

    cJSON_Delete(root);
    return json ? json : strdup("[]");
}

static int _hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char *_secret_error(const char *reason) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"store_error\"}");
    cJSON_AddStringToObject(root, "status", "error");
    cJSON_AddStringToObject(root, "reason", reason && reason[0] ? reason : "store_error");
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json ? json : strdup("{\"status\":\"error\",\"reason\":\"store_error\"}");
}

static char *handle_apps_secret_set(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    cJSON *name_item = cJSON_GetObjectItem(root, "name");
    cJSON *value_item = cJSON_GetObjectItem(root, "value");
    if (!cJSON_IsString(id_item) || !cJSON_IsString(name_item) || !cJSON_IsString(value_item)) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }

    const char *hex = value_item->valuestring;
    size_t hex_len = strlen(hex);
    if (hex_len == 0 || (hex_len % 2) != 0) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
    }
    size_t value_len = hex_len / 2;
    uint8_t *value = malloc(value_len);
    if (!value) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"out_of_memory\"}");
    }
    for (size_t i = 0; i < value_len; i++) {
        int high = _hex_nibble(hex[i * 2]);
        int low = _hex_nibble(hex[i * 2 + 1]);
        if (high < 0 || low < 0) {
            free(value);
            cJSON_Delete(root);
            return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
        }
        value[i] = (uint8_t)((high << 4) | low);
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    char reason[32] = "";
    int rc = (control && control->secret_set)
                 ? control->secret_set(control->ctx, id_item->valuestring, name_item->valuestring,
                                       value, value_len, reason, sizeof(reason))
                 : -1;

    free(value);
    cJSON_Delete(root);
    if (rc == 0) return strdup("{\"status\":\"ok\"}");
    return _secret_error(reason);
}

static char *handle_apps_secret_delete(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    cJSON *name_item = cJSON_GetObjectItem(root, "name");
    if (!cJSON_IsString(id_item) || !cJSON_IsString(name_item)) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    int rc = (control && control->secret_delete)
                 ? control->secret_delete(control->ctx, id_item->valuestring,
                                          name_item->valuestring)
                 : -1;

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"delete_failed\"}");
}

static char *handle_apps_secret_list(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    if (!cJSON_IsString(id_item) || !id_item->valuestring[0]) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_app_id\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    char *json = (control && control->secret_list)
                     ? control->secret_list(control->ctx, id_item->valuestring)
                     : NULL;

    cJSON_Delete(root);
    return json ? json : strdup("[]");
}

static const char *_json_string(cJSON *root, const char *key) {
    cJSON *item = cJSON_GetObjectItem(root, key);
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

static char *handle_apps_task_submit(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    const char *kind = _json_string(root, "kind");
    const char *key = _json_string(root, "idempotency_key");
    if (!kind || !key || !key[0]) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    char *json = (control && control->task_submit)
                     ? control->task_submit(control->ctx, kind, key)
                     : NULL;

    cJSON_Delete(root);
    return json ? json : strdup("{\"status\":\"error\",\"reason\":\"task_rejected\"}");
}

static char *handle_apps_task_status(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    const char *id = _json_string(root, "id");
    if (!id || !id[0]) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_id\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    char *json = (control && control->task_status) ? control->task_status(control->ctx, id) : NULL;

    cJSON_Delete(root);
    return json ? json : strdup("{\"status\":\"error\",\"reason\":\"not_found\"}");
}

static char *handle_apps_task_finish(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    const char *id = _json_string(root, "id");
    const char *state = _json_string(root, "state");
    const char *result = _json_string(root, "result_json");
    if (!id || !state) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    int rc = (control && control->task_finish)
                 ? control->task_finish(control->ctx, id, state, result)
                 : -1;

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"task_update_rejected\"}");
}

static char *handle_apps_task_purge(service_t *svc) {
    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    char *json = (control && control->task_purge) ? control->task_purge(control->ctx) : NULL;
    return json ? json : strdup("{\"status\":\"error\",\"reason\":\"purge_failed\"}");
}

static char *handle_apps_consumer_heartbeat(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    const char *name = _json_string(root, "name");
    cJSON *generation_item = cJSON_GetObjectItem(root, "applied_generation");
    if (!name || !name[0] || !cJSON_IsNumber(generation_item)) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    int rc = (control && control->consumer_heartbeat)
                 ? control->consumer_heartbeat(control->ctx, name,
                                               (unsigned long long)generation_item->valuedouble)
                 : -1;

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"heartbeat_rejected\"}");
}

static char *handle_apps_registry_status(service_t *svc, const char *app_id) {
    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    char *json = (control && control->registry_status_json)
                     ? control->registry_status_json(control->ctx, app_id)
                     : NULL;
    return json ? json : strdup("{\"status\":\"error\",\"reason\":\"registry_unavailable\"}");
}

static char *handle_apps_registry_snapshot(service_t *svc) {
    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    char *json = (control && control->registry_snapshot)
                     ? control->registry_snapshot(control->ctx)
                     : NULL;
    return json ? json : strdup("{\"status\":\"error\",\"reason\":\"snapshot_failed\"}");
}

static char *handle_apps_upload(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *name_item = cJSON_GetObjectItem(root, "name");
    cJSON *trigger_item = cJSON_GetObjectItem(root, "trigger_tag");
    cJSON *entry_item = cJSON_GetObjectItem(root, "entry_func");
    cJSON *payload_item = cJSON_GetObjectItem(root, "payload_hex");
    cJSON *manifest_item = cJSON_GetObjectItem(root, "manifest");
    cJSON *key_item = cJSON_GetObjectItem(root, "idempotency_key");

    if (!name_item || !name_item->valuestring || !payload_item || !payload_item->valuestring) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
    }
    if ((manifest_item && !manifest_item->valuestring) ||
        (entry_item && !entry_item->valuestring) ||
        (key_item && !cJSON_IsString(key_item)) ||
        (trigger_item && !cJSON_IsNumber(trigger_item))) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
    }

    size_t wasm_len = 0;
    uint8_t *wasm_buf = rest_hex_decode(payload_item->valuestring, &wasm_len);
    if (!wasm_buf) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    if (!control || !control->install_app) {
        free(wasm_buf);
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"app_manager_unavailable\"}");
    }
    int rc = control->install_app(control->ctx,
                                  manifest_item ? manifest_item->valuestring : NULL,
                                  name_item->valuestring,
                                  trigger_item ? (uint32_t)trigger_item->valueint : 0,
                                  entry_item ? entry_item->valuestring : NULL,
                                  wasm_buf, wasm_len,
                                  cJSON_IsString(key_item) ? key_item->valuestring : NULL);

    free(wasm_buf);
    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"install_failed\"}");
}

static char *handle_system_time_get(service_t *svc) {
    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    if (!control || !control->time_status_json) {
        return strdup("{\"status\":\"error\",\"reason\":\"time_state_unavailable\"}");
    }
    char *json = control->time_status_json(control->ctx);
    return json ? json : strdup("{\"status\":\"error\",\"reason\":\"time_state_unavailable\"}");
}

static char *handle_system_time_set(service_t *svc, const char *body) {
    cJSON *root = body ? cJSON_Parse(body) : NULL;
    uint64_t epoch_ms = 0;
    if (root) {
        cJSON *item = cJSON_GetObjectItem(root, "epoch_ms");
        if (cJSON_IsNumber(item) && item->valuedouble > 0) {
            epoch_ms = (uint64_t)item->valuedouble;
        }
        cJSON_Delete(root);
    }
    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    char reason[160] = "";
    int rc = (control && control->time_mark)
                 ? control->time_mark(control->ctx, epoch_ms, reason, sizeof(reason))
                 : -1;
    if (rc == 0) return strdup("{\"status\":\"ok\"}");
    cJSON *out = cJSON_CreateObject();
    if (!out) return strdup("{\"status\":\"error\",\"reason\":\"clock_mark_failed\"}");
    cJSON_AddStringToObject(out, "status", "error");
    cJSON_AddStringToObject(out, "reason", "clock_mark_failed");
    cJSON_AddStringToObject(out, "detail", reason);
    char *json = cJSON_PrintUnformatted(out);
    cJSON_Delete(out);
    return json;
}

char *rest_dispatch_ex(service_t *svc, const char *method, const char *path, const char *body,
                       int *status) {
    // rt-alloc-ok: management-plane REST router, not an execution callback; cJSON allocates internally
    if (status) *status = 200;
    if (!svc) {
        fprintf(stderr, "[REST] ERROR: Dispatch called with NULL service context!\n");
        return strdup("{\"error\":\"no_context\"}");
    }
    if (!method || !path) return strdup("{\"error\":\"bad_request\"}");
    body = body ? body : "";

    if (strcmp(method, "POST") == 0) {
        if (strncmp(path, "/api/v1/system/time", 19) == 0) return handle_system_time_set(svc, body);
        if (strncmp(path, "/api/v1/actor/set", 17) == 0) return handle_data_write(svc, body);
        if (strncmp(path, "/api/v1/io/raw", 14) == 0) return handle_io_raw(svc, body);
        if (strncmp(path, "/api/v1/apps/upload", 19) == 0) {
            if (!g_rest_legacy_upload) {
                if (status) *status = 410;
                return strdup("{\"status\":\"error\",\"reason\":\"legacy_upload_disabled\","
                              "\"hint\":\"use /api/v1/apps/package with a signed .eapp\"}");
            }
            return handle_apps_upload(svc, body);
        }
        if (strncmp(path, "/api/v1/apps/package", 21) == 0)
            return rest_apps_package(svc, body);
        if (strncmp(path, "/api/v1/apps/task/status", 25) == 0)
            return handle_apps_task_status(svc, body);
        if (strncmp(path, "/api/v1/apps/task/finish", 25) == 0)
            return handle_apps_task_finish(svc, body);
        if (strncmp(path, "/api/v1/apps/task/purge", 24) == 0) return handle_apps_task_purge(svc);
        if (strncmp(path, "/api/v1/apps/task", 18) == 0) return handle_apps_task_submit(svc, body);
        if (strncmp(path, "/api/v1/apps/snapshot", 22) == 0)
            return handle_apps_registry_snapshot(svc);
        if (strncmp(path, "/api/v1/apps/consumers/heartbeat", 33) == 0)
            return handle_apps_consumer_heartbeat(svc, body);
        if (strncmp(path, "/api/v1/apps/rollback", 22) == 0)
            return handle_apps_rollback(svc, body);
        if (strncmp(path, "/api/v1/apps/upgrade_package", 29) == 0)
            return rest_apps_upgrade_package(svc, body);
        if (strncmp(path, "/api/v1/apps/upgrade", 21) == 0) return handle_apps_upgrade(svc, body);
        if (strncmp(path, "/api/v1/apps/secret/delete", 26) == 0)
            return handle_apps_secret_delete(svc, body);
        if (strncmp(path, "/api/v1/apps/secret/list", 24) == 0)
            return handle_apps_secret_list(svc, body);
        if (strncmp(path, "/api/v1/apps/secret", 19) == 0)
            return handle_apps_secret_set(svc, body);
        if (strncmp(path, "/api/v1/apps/revoke_grant", 25) == 0)
            return handle_apps_revoke_grant(svc, body);
        if (strncmp(path, "/api/v1/apps/grants", 19) == 0) return handle_apps_grants(svc, body);
        if (strncmp(path, "/api/v1/apps/grant", 18) == 0) return handle_apps_grant(svc, body);
        if (strncmp(path, "/api/v1/apps/start", 18) == 0) return handle_apps_start(svc, body);
        if (strncmp(path, "/api/v1/apps/stop", 17) == 0) return handle_apps_stop(svc, body);
        if (strncmp(path, "/api/v1/apps/uninstall", 22) == 0)
            return handle_apps_uninstall(svc, body);
    } else if (strcmp(method, "GET") != 0) {
        if (status) *status = 405;
        return strdup("{\"status\":\"error\",\"reason\":\"method_not_allowed\"}");
    }

    if (strncmp(path, "/api/v1/system/time", 19) == 0) return handle_system_time_get(svc);
    if (strncmp(path, "/api/v1/system/health", 21) == 0) return handle_system_health(svc);
    if (strncmp(path, "/api/v1/system/stats", 20) == 0) return handle_system_stats(svc);
    if (strncmp(path, "/api/v1/contracts", 17) == 0) return handle_contracts_get(svc);
    if (strncmp(path, "/api/v1/devices", 15) == 0) return handle_devices_get(svc);
    if (strncmp(path, "/api/v1/io/raw/latest", 21) == 0) return handle_io_raw_latest();
    if (strncmp(path, "/api/v1/stream/topology", 23) == 0) return handle_stream_topology(svc);
    if (strncmp(path, "/api/v1/stream/snapshot", 23) == 0) return handle_buffer_snapshot(svc);
    if (strncmp(path, "/api/v1/apps/list", 17) == 0) return handle_apps_list(svc);
    if (strncmp(path, "/api/v1/apps/status/", 20) == 0)
        return handle_apps_registry_status(svc, path + 20);
    if (strncmp(path, "/api/v1/apps/status", 20) == 0)
        return handle_apps_registry_status(svc, NULL);
    if (strncmp(path, "/api/v1/data/latest/", 20) == 0)
        return handle_data_latest(svc, (uint32_t)atoi(path + 20));
    return strdup("{\"error\":\"not_found\"}");
}

char *rest_dispatch(service_t *svc, const char *method, const char *path, const char *body) {
    return rest_dispatch_ex(svc, method, path, body, NULL);
}

static void on_client_close(uv_handle_t *handle) {
    rest_client_t *client = (rest_client_t *)handle->data;
    if (client) {
        free(client->dyn_tx_buf);
        free(client->header_buf);
        free(client->body_buf);
        client->dyn_tx_buf = NULL;
        client->header_buf = NULL;
        client->body_buf = NULL;
        client->in_use = false;
        client->responded = false;
        client->headers_done = false;
    }
}

static void on_write(uv_write_t *req, int status) {
    (void)status;
    rest_client_t *client = (rest_client_t *)req->handle->data;
    if (!client) return;
    free(client->dyn_tx_buf);
    client->dyn_tx_buf = NULL;
    if (!uv_is_closing((uv_handle_t *)&client->tcp)) {
        uv_close((uv_handle_t *)&client->tcp, on_client_close);
    }
}

static void on_alloc(uv_handle_t *h, size_t suggested, uv_buf_t *buf) {
    (void)suggested;
    rest_client_t *client = (rest_client_t *)h->data;
    if (client) {
        buf->base = client->rx_buf;
        buf->len = sizeof(client->rx_buf);
    } else {
        buf->base = NULL;
        buf->len = 0;
    }
}

static const char *status_reason(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 410: return "Gone";
        case 413: return "Payload Too Large";
        case 431: return "Request Header Fields Too Large";
        default: return "Error";
    }
}

static void client_respond(rest_client_t *client, int status, const char *json) {
    if (!client || client->responded) return;
    client->responded = true;
    client->state.deadline_ms = 0;
    uv_read_stop((uv_stream_t *)&client->tcp);
    if (!json) json = "{}";
    size_t json_len = strlen(json);
    const char *cors =
        client->server->limits.allow_cors_origin ? client->server->limits.allow_cors_origin : "";
    const char *allow = (status == 405) ? "Allow: GET, POST\r\n" : "";
    // 512 覆盖状态行、CORS、长度和其他固定头，余量足够。
    size_t need = 512 + strlen(cors) + json_len;
    char *buf;
    if (need <= sizeof(client->tx_buf)) {
        buf = client->tx_buf;
        client->dyn_tx_buf = NULL;
    } else {
        buf = malloc(need);
        if (!buf) {
            if (!uv_is_closing((uv_handle_t *)&client->tcp)) {
                uv_close((uv_handle_t *)&client->tcp, on_client_close);
            }
            return;
        }
        client->dyn_tx_buf = buf;
    }
    int n = snprintf(buf, need,
                     "HTTP/1.1 %d %s\r\n"
                     "Content-Type: application/json\r\n"
                     "%s"
                     "Content-Length: %zu\r\n"
                     "%s%s%s"
                     "Connection: close\r\n"
                     "\r\n",
                     status, status_reason(status), allow, json_len,
                     cors[0] ? "Access-Control-Allow-Origin: " : "",
                     cors[0] ? cors : "",
                     cors[0] ? "\r\n" : "");
    if (n < 0 || (size_t)n + json_len >= need) {
        if (client->dyn_tx_buf) free(client->dyn_tx_buf);
        client->dyn_tx_buf = NULL;
        if (!uv_is_closing((uv_handle_t *)&client->tcp)) {
            uv_close((uv_handle_t *)&client->tcp, on_client_close);
        }
        return;
    }
    memcpy(buf + n, json, json_len);
    client->close_after_write = true;
    client->write_req.data = client;
    uv_buf_t write_buf = uv_buf_init(buf, (unsigned int)((size_t)n + json_len));
    if (uv_write(&client->write_req, (uv_stream_t *)&client->tcp, &write_buf, 1, on_write) != 0 &&
        !uv_is_closing((uv_handle_t *)&client->tcp)) {
        uv_close((uv_handle_t *)&client->tcp, on_client_close);
    }
}

static char *find_header_end(const char *buf, size_t len) {
    for (size_t i = 0; i + 3 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            return (char *)buf + i;
        }
    }
    return NULL;
}

static bool parse_size_value(const char *s, size_t len, size_t *out) {
    while (len && (*s == ' ' || *s == '\t')) {
        s++;
        len--;
    }
    while (len && (s[len - 1] == ' ' || s[len - 1] == '\t')) len--;
    if (len == 0) return false;
    size_t value = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];
        if (!isdigit(ch)) return false;
        size_t digit = (size_t)(ch - '0');
        if (value > (SIZE_MAX - digit) / 10) return false;
        value = value * 10 + digit;
    }
    *out = value;
    return true;
}

static bool header_name_equals(const char *name, size_t len, const char *want) {
    size_t want_len = strlen(want);
    if (len != want_len) return false;
    for (size_t i = 0; i < len; i++) {
        if (tolower((unsigned char)name[i]) != tolower((unsigned char)want[i])) return false;
    }
    return true;
}

static bool header_name_is_token(const char *name, size_t len) {
    if (len == 0) return false;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)name[i];
        if (!isalnum(c) && !strchr("!#$%&'*+-.^_`|~", c)) return false;
    }
    return true;
}

static int parse_request_head(rest_client_state_t *st, char *head, const rest_limits_t *limits) {
    char *line_end = strstr(head, "\r\n");
    char *headers = NULL;
    if (line_end) {
        *line_end = '\0';
        headers = line_end + 2;
    }
    char version[16] = {0};
    char extra[2] = {0};
    int fields = sscanf(head, "%15s %127s %15s %1s", st->method, st->path, version, extra);
    if (line_end) *line_end = '\r';
    if (fields != 3) return 400;
    if (strncmp(version, "HTTP/1.", 7) != 0) return 400;
    if (st->path[0] != '/') return 400;
    for (const char *p = st->method; *p; p++) {
        if (!isupper((unsigned char)*p)) return 400;
    }
    if (strcmp(st->method, "GET") != 0 && strcmp(st->method, "POST") != 0) return 405;

    bool have_content_length = false;
    char *end = head + strlen(head);
    char *p = headers;
    while (p && p < end) {
        char *eol = strstr(p, "\r\n");
        size_t line_len = eol ? (size_t)(eol - p) : (size_t)(end - p);
        if (line_len == 0) break;
        char *colon = memchr(p, ':', line_len);
        if (!colon) return 400;
        size_t name_len = (size_t)(colon - p);
        if (!header_name_is_token(p, name_len)) return 400;
        if (header_name_equals(p, name_len, "content-length")) {
            if (have_content_length) return 400;
            if (!parse_size_value(colon + 1, line_len - name_len - 1, &st->content_length)) {
                return 400;
            }
            have_content_length = true;
        } else if (header_name_equals(p, name_len, "transfer-encoding")) {
            return 400;  // 管理面只接受 Content-Length 定长 body
        }
        p = eol ? eol + 2 : end;
    }
    if (st->content_length > limits->max_body_bytes) return 413;
    return 0;
}

static void client_dispatch(rest_client_t *client) {
    int status = 200;
    char *json = rest_dispatch_ex(client->server->svc, client->state.method, client->state.path,
                                  client->body_buf ? client->body_buf : "", &status);
    client_respond(client, status, json ? json : "{}");
    free(json);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    rest_client_t *client = (rest_client_t *)stream->data;
    if (!client) return;
    if (nread == 0) return;
    if (nread < 0) {
        if (!uv_is_closing((uv_handle_t *)stream)) {
            uv_close((uv_handle_t *)stream, on_client_close);
        }
        return;
    }
    if (client->responded) return;

    const rest_limits_t *limits = &client->server->limits;
    const char *data = buf->base;
    size_t len = (size_t)nread;

    if (!client->headers_done) {
        if (!client->header_buf) {
            client->header_cap = limits->max_header_bytes + REST_RX_BUF_SIZE + 1;
            client->header_buf = malloc(client->header_cap);
            if (!client->header_buf) {
                client_respond(client, 400, "{\"error\":\"out_of_memory\"}");
                return;
            }
            client->header_len = 0;
        }
        size_t room = client->header_cap - 1 - client->header_len;
        size_t copy_len = len < room ? len : room;
        memcpy(client->header_buf + client->header_len, data, copy_len);
        client->header_len += copy_len;
        client->header_buf[client->header_len] = '\0';

        char *head_end = find_header_end(client->header_buf, client->header_len);
        if (!head_end) {
            if (client->header_len >= limits->max_header_bytes || copy_len < len) {
                client_respond(client, 431, "{\"error\":\"request_header_fields_too_large\"}");
            }
            return;
        }
        size_t header_bytes = (size_t)(head_end - client->header_buf) + 4;
        if (header_bytes > limits->max_header_bytes) {
            client_respond(client, 431, "{\"error\":\"request_header_fields_too_large\"}");
            return;
        }
        client->state.header_bytes = header_bytes;
        *head_end = '\0';
        int parse_rc = parse_request_head(&client->state, client->header_buf, limits);
        if (parse_rc != 0) {
            const char *error =
                parse_rc == 405 ? "{\"error\":\"method_not_allowed\"}" : "{\"error\":\"bad_request\"}";
            client_respond(client, parse_rc, error);
            return;
        }
        client->headers_done = true;

        size_t body_avail = client->header_len - header_bytes;
        if (body_avail > client->state.content_length) {
            client_respond(client, 400, "{\"error\":\"bad_request\"}");
            return;
        }
        if (client->state.content_length > 0) {
            client->body_buf = malloc(client->state.content_length + 1);
            if (!client->body_buf) {
                client_respond(client, 400, "{\"error\":\"out_of_memory\"}");
                return;
            }
            memcpy(client->body_buf, client->header_buf + header_bytes, body_avail);
            client->state.body_bytes = body_avail;
            client->body_buf[body_avail] = '\0';
        }
        free(client->header_buf);
        client->header_buf = NULL;
        client->header_cap = 0;
    } else {
        size_t remaining = client->state.content_length - client->state.body_bytes;
        if (len > remaining) {
            client_respond(client, 400, "{\"error\":\"bad_request\"}");
            return;
        }
        if (len > 0) {
            memcpy(client->body_buf + client->state.body_bytes, data, len);
            client->state.body_bytes += len;
            client->body_buf[client->state.body_bytes] = '\0';
        }
    }

    if (client->state.body_bytes >= client->state.content_length) {
        client_dispatch(client);
    }
}

static void on_timeout_tick(uv_timer_t *handle) {
    rest_server_t *srv = (rest_server_t *)handle->data;
    if (!srv) return;
    uint64_t now = uv_now(srv->loop);
    for (int i = 0; i < srv->limits.max_clients; i++) {
        rest_client_t *client = &srv->clients[i];
        if (!client->in_use || client->responded || client->state.deadline_ms == 0) continue;
        if (now >= client->state.deadline_ms) {
            client_respond(client, 408, "{\"error\":\"request_timeout\"}");
        }
    }
}

static void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) return;
    rest_server_t *srv = (rest_server_t *)server->data;
    rest_client_t *client = NULL;
    for (int i = 0; i < srv->limits.max_clients; i++) {
        if (!srv->clients[i].in_use) {
            client = &srv->clients[i];
            break;
        }
    }
    if (!client) {
        uv_tcp_t tmp;
        uv_tcp_init(server->loop, &tmp);
        if (uv_accept(server, (uv_stream_t *)&tmp) == 0) {
            uv_close((uv_handle_t *)&tmp, NULL);
        }
        return;
    }

    memset(client, 0, sizeof(*client));
    client->in_use = true;
    client->server = srv;
    client->state.deadline_ms = uv_now(server->loop) + (uint64_t)srv->limits.read_timeout_ms;
    uv_tcp_init(server->loop, &client->tcp);
    client->tcp.data = client;
    if (uv_accept(server, (uv_stream_t *)&client->tcp) == 0) {
        uv_read_start((uv_stream_t *)&client->tcp, on_alloc, on_read);
    } else {
        uv_close((uv_handle_t *)&client->tcp, on_client_close);
    }
}

void rest_limits_default(rest_limits_t *limits) {
    if (!limits) return;
    memset(limits, 0, sizeof(*limits));
    limits->bind_addr = REST_DEFAULT_BIND_ADDR;
    limits->max_clients = REST_DEFAULT_MAX_CLIENTS;
    limits->max_header_bytes = REST_DEFAULT_MAX_HEADER_BYTES;
    limits->max_body_bytes = REST_DEFAULT_MAX_BODY_BYTES;
    limits->read_timeout_ms = REST_DEFAULT_READ_TIMEOUT_MS;
}

static void rest_server_apply_limits(rest_server_t *srv, const rest_limits_t *in) {
    rest_limits_t defaults;
    rest_limits_default(&defaults);
    if (!in) in = &defaults;
    srv->limits = *in;
    const char *bind = (in->bind_addr && in->bind_addr[0]) ? in->bind_addr : REST_DEFAULT_BIND_ADDR;
    snprintf(srv->bind_addr, sizeof(srv->bind_addr), "%s", bind);
    snprintf(srv->cors_origin, sizeof(srv->cors_origin), "%s",
             in->allow_cors_origin ? in->allow_cors_origin : "");
    srv->limits.bind_addr = srv->bind_addr;
    srv->limits.allow_cors_origin = srv->cors_origin;
    if (srv->limits.max_clients <= 0) srv->limits.max_clients = REST_MAX_CLIENTS;
    if (srv->limits.max_clients > REST_MAX_CLIENTS) srv->limits.max_clients = REST_MAX_CLIENTS;
    if (srv->limits.max_header_bytes == 0) {
        srv->limits.max_header_bytes = REST_DEFAULT_MAX_HEADER_BYTES;
    }
    if (srv->limits.max_body_bytes == 0) {
        srv->limits.max_body_bytes = REST_DEFAULT_MAX_BODY_BYTES;
    }
    if (srv->limits.read_timeout_ms <= 0) {
        srv->limits.read_timeout_ms = REST_DEFAULT_READ_TIMEOUT_MS;
    }
}

static int _rest_server_bind_and_listen(rest_server_t *srv, uv_loop_t *loop, service_t *svc,
                                        int port) {
    srv->loop = loop;
    srv->svc = svc;
    srv->port = port;
    uv_tcp_init(loop, &srv->server);
    srv->server.data = srv;
    uv_timer_init(loop, &srv->timeout_timer);
    srv->timeout_timer.data = srv;
    srv->timer_init = true;

    struct sockaddr_in addr;
    int rc = uv_ip4_addr(srv->limits.bind_addr, port, &addr);
    if (rc != 0) {
        fprintf(stderr, "[REST] Invalid bind address '%s': %s\n", srv->limits.bind_addr,
                uv_strerror(rc));
        return rc;
    }
    rc = uv_tcp_bind(&srv->server, (const struct sockaddr *)&addr, 0);
    if (rc != 0) {
        fprintf(stderr, "[REST] Failed to bind %s:%d: %s\n", srv->limits.bind_addr, port,
                uv_strerror(rc));
        return rc;
    }
    rc = uv_listen((uv_stream_t *)&srv->server, 128, on_new_connection);
    if (rc != 0) {
        fprintf(stderr, "[REST] Failed to listen on %s:%d: %s\n", srv->limits.bind_addr, port,
                uv_strerror(rc));
        return rc;
    }

    int tick = srv->limits.read_timeout_ms / 4;
    if (tick < 50) tick = 50;
    if (tick > 1000) tick = 1000;
    uv_timer_start(&srv->timeout_timer, on_timeout_tick, (uint64_t)tick, (uint64_t)tick);

    char bound[64] = {0};
    int bound_port = port;
    struct sockaddr_storage ss;
    int ss_len = (int)sizeof(ss);
    if (uv_tcp_getsockname(&srv->server, (struct sockaddr *)&ss, &ss_len) == 0 &&
        ss.ss_family == AF_INET) {
        inet_ntop(AF_INET, &((struct sockaddr_in *)&ss)->sin_addr, bound, sizeof(bound));
        bound_port = ntohs(((struct sockaddr_in *)&ss)->sin_port);
    }
    fprintf(stderr, "[REST] Server listening on %s:%d\n",
            bound[0] ? bound : srv->limits.bind_addr, bound_port);
    return 0;
}

rest_server_t *rest_server_create_in_ex(rest_server_storage_t *storage, uv_loop_t *loop,
                                        service_t *svc, int port,
                                        const rest_limits_t *limits) {
    if (!loop) return NULL;
    _Static_assert(sizeof(rest_server_t) <= sizeof(rest_server_storage_t), "rest_server_t exceeds storage");
    rest_server_t *srv;
    if (storage) {
        srv = (rest_server_t *)storage;
        memset(srv, 0, sizeof(*srv));
        srv->is_allocated = false;
    } else {
        srv = calloc(1, sizeof(rest_server_t));
        if (!srv) return NULL;
        srv->is_allocated = true;
    }
    rest_server_apply_limits(srv, limits);
    _rest_server_bind_and_listen(srv, loop, svc, port);
    return srv;
}

rest_server_t *rest_server_create_in(rest_server_storage_t *storage, uv_loop_t *loop, service_t *svc, int port) {
    return rest_server_create_in_ex(storage, loop, svc, port, NULL);
}

rest_server_t *rest_server_create(uv_loop_t *loop, service_t *svc, int port) {
    return rest_server_create_in_ex(NULL, loop, svc, port, NULL);
}

int rest_server_get_bound_addr(const rest_server_t *srv, char *ip, size_t ip_len, int *port) {
    if (!srv || !ip || ip_len == 0) return -1;
    struct sockaddr_storage ss;
    int ss_len = (int)sizeof(ss);
    if (uv_tcp_getsockname((uv_tcp_t *)&srv->server, (struct sockaddr *)&ss, &ss_len) != 0) {
        return -1;
    }
    char tmp[INET6_ADDRSTRLEN] = {0};
    int bound_port = 0;
    if (ss.ss_family == AF_INET) {
        inet_ntop(AF_INET, &((struct sockaddr_in *)&ss)->sin_addr, tmp, sizeof(tmp));
        bound_port = ntohs(((struct sockaddr_in *)&ss)->sin_port);
    } else if (ss.ss_family == AF_INET6) {
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&ss)->sin6_addr, tmp, sizeof(tmp));
        bound_port = ntohs(((struct sockaddr_in6 *)&ss)->sin6_port);
    } else {
        return -1;
    }
    snprintf(ip, ip_len, "%s", tmp);
    if (port) *port = bound_port;
    return 0;
}

static void _on_rest_server_close(uv_handle_t *h) {
    rest_server_t *srv = (rest_server_t *)h->data;
    if (srv && srv->is_allocated) {
        free(srv);
    }
}

void rest_server_destroy(rest_server_t *srv) {
    if (!srv) return;
    for (int i = 0; i < srv->limits.max_clients; i++) {
        if (srv->clients[i].in_use && !uv_is_closing((uv_handle_t *)&srv->clients[i].tcp)) {
            uv_close((uv_handle_t *)&srv->clients[i].tcp, on_client_close);
        }
    }
    if (srv->timer_init && !uv_is_closing((uv_handle_t *)&srv->timeout_timer)) {
        uv_timer_stop(&srv->timeout_timer);
        uv_close((uv_handle_t *)&srv->timeout_timer, NULL);
    }
    if (!uv_is_closing((uv_handle_t *)&srv->server)) {
        uv_close((uv_handle_t *)&srv->server, _on_rest_server_close);
    }
}
