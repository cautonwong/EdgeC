#include "rest_server.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "edge_params.h"
#include "edge_service.h"
#include "sqlite_service.h"
#include "contracts.h"

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

// --- SQLite 查询回调封装 ---
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
    return 0;  // 继续下一行
}

// --- REST Handlers ---

/**
 * @brief 历史数据点查 API (对齐 web_ui_ia.md 3.3)
 * 场景：Web UI 查询历史曲线
 */


static char *handle_system_health(service_t *svc) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddNumberToObject(root, "uptime", (double)uv_now(service_get_loop(svc)) / 1000);

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

char *rest_dispatch(service_t *svc, const char *path) {
    if (strcmp(path, "/api/v1/system/health") == 0) return handle_system_health(svc);

    // 路由 1: 实时影子点查
    if (strncmp(path, "/api/v1/data/latest/", 20) == 0) {
        uint32_t tag_id = (uint32_t)atoi(path + 20);
        return handle_data_latest(svc, tag_id);
    }

    // 路由 2: 历史曲线查询 (参数暂通过 QueryString 提取，此处演示核心分发)
    // 示例：/api/v1/data/query?tag=1001&start=0&end=...
    if (strncmp(path, "/api/v1/data/query", 18) == 0) {
        // TODO: This is a hardcoded test endpoint and should be removed.
        // The tag to be queried should come from the request.
        // return handle_history_query(svc, TAG_GRID_POWER, 0, (uint64_t)time(NULL) * 1000);
        return strdup("{\"error\":\"not_found\"}");
    }

    return strdup("{\"error\":\"not_found\"}");
}

// --- libuv TCP stack implementation ... ---

static void on_write(uv_write_t *req, int status) {
    (void)status;
    free(req->data);
    free(req);
}

static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        rest_server_t *srv = client->data;
        char path[128] = {0};
        sscanf(buf->base, "GET %127s HTTP/1.1", path);

        char *json = rest_dispatch(srv->svc, path);
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

static void on_alloc(uv_handle_t *h, size_t suggested, uv_buf_t *buf) {
    (void)h;
    buf->base = malloc(suggested);
    buf->len = suggested;
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
