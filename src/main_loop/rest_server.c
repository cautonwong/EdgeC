#include "rest_server.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "contract_data_types.h"
#include "datahub.h"
#include "edge_log.h"
#include "edge_params.h"
#include "edge_service.h"
#include "sqlite_service.h"
#include "stream_engine.h"

#define HTTP_RESPONSE_FMT                \
    "HTTP/1.1 200 OK\r\n"                \
    "Content-Type: application/json\r\n" \
    "Content-Length: %zu\r\n"            \
    "Access-Control-Allow-Origin: *\r\n" \
    "Connection: close\r\n"              \
    "\r\n"                               \
    "%s"

struct rest_server_s {
    uv_loop_t *loop;
    service_t *svc;
    int port;
    uv_tcp_t server;
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

// --- API Handlers ---

static char *handle_system_health(service_t *svc) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddNumberToObject(root, "uptime", (double)uv_now(service_get_loop(svc)) / 1000);

    cJSON *storage = cJSON_AddObjectToObject(root, "storage");
    cJSON_AddNumberToObject(storage, "queue_depth", 0);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static char *handle_contracts_get(service_t *svc) {
    const char *path = service_get_contract_path(svc);
    if (!path || access(path, F_OK) != 0) return strdup("{\"error\":\"contract_not_found\"}");
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

static char *handle_pipelines_get(service_t *svc) {
    stream_engine_t *engine = service_get_stream_engine(svc);
    if (!engine) return strdup("{\"error\":\"engine_not_ready\"}");
    return stream_engine_dump_json(engine);
}

static char *handle_devices_get(service_t *svc) {
    (void)svc;
    cJSON *root = cJSON_CreateArray();
    const char *mock_ids[] = {"meter_01", "pcs_01", "bms_01"};
    const char *mock_protos[] = {"Modbus_RTU", "Modbus_TCP", "CAN_Bus"};

    for (int i = 0; i < 3; i++) {
        cJSON *dev = cJSON_CreateObject();
        cJSON_AddStringToObject(dev, "id", mock_ids[i]);
        cJSON_AddStringToObject(dev, "protocol", mock_protos[i]);
        cJSON_AddStringToObject(dev, "status", (i == 2) ? "offline" : "online");
        cJSON_AddNumberToObject(dev, "last_active", (double)uv_now(service_get_loop(svc)));
        cJSON_AddNumberToObject(dev, "errors", 0);
        cJSON_AddItemToArray(root, dev);
    }

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static char *handle_params_list(service_t *svc) {
    (void)svc;
    cJSON *root = cJSON_CreateObject();
    cJSON *params = cJSON_AddArrayToObject(root, "params");

    // 工业级参数映射：从系统参数中心获取真实状态
    // 此处列出几个核心业务参数作为示例
    const char *keys[] = {"vpp_max_charge_w", "vpp_max_discharge_w", "grid_freq_threshold_hz"};
    float defaults[] = {5000.0f, 5000.0f, 0.5f};

    for (int i = 0; i < 3; i++) {
        cJSON *p = cJSON_CreateObject();
        cJSON_AddStringToObject(p, "key", keys[i]);
        cJSON_AddNumberToObject(p, "value", edge_param_get_float(keys[i], defaults[i]));
        cJSON_AddStringToObject(p, "type", "float");
        cJSON_AddItemToArray(params, p);
    }

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static char *handle_config_post(service_t *svc, const char *body) {
    (void)svc;
    if (!body) return strdup("{\"error\":\"missing_body\"}");

    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"error\":\"invalid_json\"}");

    cJSON *key = cJSON_GetObjectItem(root, "key");
    cJSON *val = cJSON_GetObjectItem(root, "value");

    if (cJSON_IsString(key) && cJSON_IsNumber(val)) {
        // 核心：执行带审计的参数设置
        edge_param_set_float(key->valuestring, (float)val->valuedouble);
        LOG_INFO(LOG_CAT_CORE, "Config updated: %s = %.3f", key->valuestring, val->valuedouble);
        cJSON_Delete(root);
        return strdup("{\"status\":\"ok\",\"message\":\"parameter_queued_for_update\"}");
    }

    cJSON_Delete(root);
    return strdup("{\"error\":\"bad_request_schema\"}");
}

static char *handle_history_query(service_t *svc, uint32_t tag_id, uint64_t start, uint64_t end) {
    (void)tag_id;
    sqlite_svc_t *db = service_get_storage(svc);
    if (!db) return strdup("{\"error\":\"storage_not_ready\"}");
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_AddArrayToObject(root, "data");
    query_ctx_t q_ctx = {.array = data, .count = 0};
    sqlite_svc_query(db, start, end, on_sqlite_row, &q_ctx);
    cJSON_AddNumberToObject(root, "count", q_ctx.count);
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static char *handle_data_latest(service_t *svc, uint32_t tag_id) {
    datahub_t *hub = service_get_datahub(svc);
    PipeMsg msg;
    cJSON *root = cJSON_CreateObject();
    if (datahub_read_shadow(hub, tag_id, &msg) == 0) {
        cJSON_AddNumberToObject(root, "tag_id", msg.tag_id);
        cJSON_AddNumberToObject(root, "value", msg.payload.val.f);
        cJSON_AddNumberToObject(root, "ts", (double)msg.ts);
    } else {
        cJSON_AddStringToObject(root, "status", "not_found");
    }
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

// --- Dispatcher ---

static uint64_t get_query_param_uint(const char *path, const char *key, uint64_t default_val) {
    char search_key[32];
    snprintf(search_key, sizeof(search_key), "%s=", key);
    char *p = strstr(path, search_key);
    if (!p) return default_val;
    return (uint64_t)atoll(p + strlen(search_key));
}

/**
 * @brief 下发 UI 描述符 (UI Descriptor)
 */
static char *handle_ui_descriptor_get(service_t *svc) {
    const char *contract_path = service_get_contract_path(svc);
    if (!contract_path) return strdup("{\"error\":\"no_path\"}");

    // 假设描述符文件与契约 YAML 在同一目录 (products/<name>/generated/ui_descriptor.json)
    char desc_path[256];
    strncpy(desc_path, contract_path, sizeof(desc_path));
    char *ext = strstr(desc_path, "contracts.yaml");
    if (ext) strcpy(ext, "ui_descriptor.json");

    if (access(desc_path, F_OK) != 0) return strdup("{\"error\":\"not_found\"}");

    FILE *f = fopen(desc_path, "r");
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)len + 1);
    fread(buf, 1, (size_t)len, f);
    fclose(f);
    buf[len] = '\0';
    return buf;
}

char *rest_dispatch(service_t *svc, const char *path, const char *body) {
    if (strcmp(path, "/api/v1/system/health") == 0) return handle_system_health(svc);
    if (strcmp(path, "/api/v1/contracts") == 0) return handle_contracts_get(svc);
    if (strcmp(path, "/api/v1/ui/descriptor") == 0) return handle_ui_descriptor_get(svc);
    if (strcmp(path, "/api/v1/pipelines") == 0) return handle_pipelines_get(svc);
    if (strcmp(path, "/api/v1/devices") == 0) return handle_devices_get(svc);

    if (strncmp(path, "/api/v1/data/query", 18) == 0) {
        uint32_t tag_id = (uint32_t)get_query_param_uint(path, "tag", 1001);
        uint64_t start = get_query_param_uint(path, "start", 0);
        uint64_t end = get_query_param_uint(path, "end", (uint64_t)time(NULL) * 1000);
        return handle_history_query(svc, tag_id, start, end);
    }

    if (strcmp(path, "/api/v1/system/config") == 0) {
        if (body) return handle_config_post(svc, body);
        return handle_params_list(svc);
    }
    if (strncmp(path, "/api/v1/data/latest/", 20) == 0)
        return handle_data_latest(svc, (uint32_t)atoi(path + 20));

    return strdup("{\"error\":\"not_found\"}");
}

// --- Network Stack ---

static void on_write(uv_write_t *req, int status) {
    (void)status;
    free(req->data);
    free(req);
}

static void on_alloc(uv_handle_t *h, size_t suggested, uv_buf_t *buf) {
    (void)h;
    buf->base = malloc(suggested);
    buf->len = suggested;
}

static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        rest_server_t *srv = client->data;
        char path[128] = {0}, method[16] = {0};
        sscanf(buf->base, "%15s %127s", method, path);
        char *body = strstr(buf->base, "\r\n\r\n");
        if (body) body += 4;

        char *json = rest_dispatch(srv->svc, path, body);
        size_t json_len = strlen(json);
        char *resp = malloc(json_len + 512);
        size_t resp_len = (size_t)sprintf(resp, HTTP_RESPONSE_FMT, json_len, json);

        uv_write_t *write_req = malloc(sizeof(uv_write_t));
        write_req->data = resp;
        uv_buf_t write_buf = uv_buf_init(resp, (unsigned int)resp_len);
        uv_write(write_req, client, &write_buf, 1, on_write);
        free(json);
    }
    if (nread < 0) uv_close((uv_handle_t *)client, (uv_close_cb)free);
    if (buf->base) free(buf->base);
}

static void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) return;
    uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(server->loop, client);
    client->data = server->data;
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        uv_read_start((uv_stream_t *)client, on_alloc, on_read);
    } else {
        uv_close((uv_handle_t *)client, (uv_close_cb)free);
    }
}

rest_server_t *rest_server_create(uv_loop_t *loop, service_t *svc, int port) {
    rest_server_t *srv = calloc(1, sizeof(rest_server_t));
    srv->loop = loop;
    srv->svc = svc;
    srv->port = port;
    uv_tcp_init(loop, &srv->server);
    srv->server.data = srv;
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    uv_tcp_bind(&srv->server, (const struct sockaddr *)&addr, 0);
    uv_listen((uv_stream_t *)&srv->server, 128, on_new_connection);
    return srv;
}

void rest_server_destroy(rest_server_t *srv) {
    if (!srv) return;
    uv_close((uv_handle_t *)&srv->server, (uv_close_cb)free);
}