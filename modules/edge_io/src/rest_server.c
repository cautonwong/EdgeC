#include "rest_server.h"
#include <uv.h>

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "arbiter.h"
#include "contract_data_types.h"
#include "datahub.h"
#include "edge_log.h"
#include "edge_params.h"
#include "edge_service.h"
#include "edge_version.h"
#include "sqlite_service.h"
#include "edge_app_mgr.h"
#include "stream_engine.h"
#include "stream_engine_priv.h"

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
    if (hub && hub->io_pool) {
        net_buf_pool_stats_t stats;
        net_buf_pool_audit(hub->io_pool, 0, &stats);
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
        cJSON_AddNumberToObject(dh, "total_writes", (double)hub->ring.hdr->total_writes);
        cJSON_AddNumberToObject(dh, "listener_count", hub->listener_count);
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
    stream_engine_t *engine = service_get_stream_engine(svc);
    if (engine) {
        char *engine_json = stream_engine_dump_json(engine);
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
    net_buf_t *tx = net_buf_alloc(hub->io_pool, 0);

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
    stream_engine_t *engine = service_get_stream_engine(svc);
    if (engine) {
        char *json = stream_engine_dump_json(engine);
        if (json) return json;
    }
    return strdup("{\"error\":\"engine_not_ready\"}");
}

static char *handle_apps_list(service_t *svc) {
    sqlite_svc_t *db = service_get_storage(svc);
    app_manifest_t apps[64];
    int count = db ? sqlite_svc_list_apps(db, apps, 64) : 0;

    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "id", apps[i].id);
        cJSON_AddStringToObject(item, "name", apps[i].name);
        cJSON_AddStringToObject(item, "version", apps[i].version);
        cJSON_AddStringToObject(item, "author", apps[i].author);
        cJSON_AddStringToObject(item, "wasm_path", apps[i].wasm_path);
        cJSON_AddNumberToObject(item, "trigger_tag", apps[i].trigger_tag);
        cJSON_AddStringToObject(item, "entry_func", apps[i].entry_func);
        cJSON_AddStringToObject(item, "fault_policy", apps[i].fault_policy == FAULT_POLICY_ISOLATE_AND_ALARM ? "isolate_and_alarm" : "auto_restart");
        cJSON_AddNumberToObject(item, "state", apps[i].state);
        cJSON_AddNumberToObject(item, "restart_count", apps[i].restart_count);
        cJSON_AddItemToArray(arr, item);
    }

    char *json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return json;
}

static char *handle_apps_start(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");
    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    if (!id_item || !id_item->valuestring) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_app_id\"}");
    }

    edge_app_mgr_t *mgr = (edge_app_mgr_t *)service_get_app_mgr(svc);
    int rc = -1;
    if (mgr) {
        rc = edge_app_mgr_start(mgr, id_item->valuestring);
    } else {
        sqlite_svc_t *db = service_get_storage(svc);
        if (db) {
            app_manifest_t m;
            if (sqlite_svc_get_app(db, id_item->valuestring, &m) == 0) {
                m.state = APP_STATE_RUNNING;
                rc = sqlite_svc_save_app(db, &m);
            }
        }
    }

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}") : strdup("{\"status\":\"error\",\"reason\":\"start_failed\"}");
}

static char *handle_apps_stop(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");
    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    if (!id_item || !id_item->valuestring) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_app_id\"}");
    }

    edge_app_mgr_t *mgr = (edge_app_mgr_t *)service_get_app_mgr(svc);
    int rc = -1;
    if (mgr) {
        rc = edge_app_mgr_stop(mgr, id_item->valuestring);
    } else {
        sqlite_svc_t *db = service_get_storage(svc);
        if (db) {
            app_manifest_t m;
            if (sqlite_svc_get_app(db, id_item->valuestring, &m) == 0) {
                m.state = APP_STATE_STOPPED;
                rc = sqlite_svc_save_app(db, &m);
            }
        }
    }

    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}") : strdup("{\"status\":\"error\",\"reason\":\"stop_failed\"}");
}

static char *handle_apps_upload(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *name_item = cJSON_GetObjectItem(root, "name");
    cJSON *trigger_item = cJSON_GetObjectItem(root, "trigger_tag");
    cJSON *entry_item = cJSON_GetObjectItem(root, "entry_func");
    cJSON *payload_item = cJSON_GetObjectItem(root, "payload_hex");

    if (!name_item || !payload_item) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }

    const char *name = name_item->valuestring;
    const char *hex = payload_item->valuestring;
    size_t hex_len = strlen(hex);

    app_manifest_t manifest;
    cJSON *manifest_item = cJSON_GetObjectItem(root, "manifest");
    if (manifest_item && manifest_item->valuestring) {
        edge_app_mgr_parse_manifest_json(manifest_item->valuestring, &manifest);
    } else {
        edge_app_mgr_construct_default_manifest(name, &manifest);
        if (trigger_item) manifest.trigger_tag = (uint32_t)trigger_item->valueint;
        if (entry_item && entry_item->valuestring) snprintf(manifest.entry_func, sizeof(manifest.entry_func), "%s", entry_item->valuestring);
    }

    uint8_t *wasm_buf = malloc(hex_len / 2 + 1);
    size_t wasm_len = 0;
    if (wasm_buf) {
        for (size_t i = 0; i < hex_len; i += 2) {
            unsigned int b;
            sscanf(hex + i, "%02x", &b);
            wasm_buf[wasm_len++] = (uint8_t)b;
        }
    }

    edge_app_mgr_t *mgr = (edge_app_mgr_t *)service_get_app_mgr(svc);
    if (mgr) {
        int install_rc = edge_app_mgr_install(mgr, &manifest, wasm_buf, wasm_len);
        if (install_rc != 0) {
            free(wasm_buf);
            cJSON_Delete(root);
            return strdup("{\"status\":\"error\",\"reason\":\"install_failed\"}");
        }
        edge_app_mgr_start(mgr, manifest.id);
    } else {
        sqlite_svc_t *db = service_get_storage(svc);
        if (db) {
            manifest.state = APP_STATE_RUNNING;
            sqlite_svc_save_app(db, &manifest);
        }
    }

    if (wasm_buf) free(wasm_buf);

    stream_engine_t *engine = service_get_stream_engine(svc);
    if (engine) {
        pipeline_t *p = NULL;
        uv_mutex_lock(&engine->pipe_mutex);
        struct uv__queue *q;
        uv__queue_foreach(q, &engine->pipelines) {
            pipeline_t *curr = uv__queue_data(q, pipeline_t, node);
            if (strcmp(curr->id, manifest.id) == 0) {
                p = curr;
                break;
            }
        }
        uv_mutex_unlock(&engine->pipe_mutex);

        if (p) {
            pipeline_stage_t *st = stage_wasm_create(manifest.wasm_path, manifest.entry_func);
            if (st) {
                uv_mutex_lock(&engine->pipe_mutex);
                pipeline_stage_t *s = p->head;
                while (s) {
                    pipeline_stage_t *next_s = s->next[0];
                    if (s->cleanup_state) s->cleanup_state(s->priv_state);
                    if (s->cleanup_cfg) s->cleanup_cfg(s->priv_cfg);
                    free(s);
                    s = next_s;
                }
                p->head = NULL;
                p->trigger_tag = manifest.trigger_tag;
                uv_mutex_unlock(&engine->pipe_mutex);
                pipeline_add_stage(p, st->name, st->process, st->priv_state, st->cleanup_state, st->priv_cfg, st->cleanup_cfg);
                free(st);
            }
        } else {
            p = stream_engine_add_pipeline(engine, manifest.id, manifest.trigger_tag);
            if (p) {
                pipeline_stage_t *st = stage_wasm_create(manifest.wasm_path, manifest.entry_func);
                if (st) {
                    pipeline_add_stage(p, st->name, st->process, st->priv_state, st->cleanup_state, st->priv_cfg, st->cleanup_cfg);
                    free(st);
                }
            }
        }
    }

    cJSON_Delete(root);
    return strdup("{\"status\":\"ok\"}");
}

char *rest_dispatch(service_t *svc, const char *method, const char *path, const char *body) {
    if (!svc) {
        fprintf(stderr, "[REST] ERROR: Dispatch called with NULL service context!\n");
        return strdup("{\"error\":\"no_context\"}");
    }

    if (strcmp(method, "POST") == 0) {
        if (strncmp(path, "/api/v1/actor/set", 17) == 0) return handle_data_write(svc, body);
        if (strncmp(path, "/api/v1/io/raw", 14) == 0) return handle_io_raw(svc, body);
        if (strncmp(path, "/api/v1/apps/upload", 19) == 0) return handle_apps_upload(svc, body);
        if (strncmp(path, "/api/v1/apps/start", 18) == 0) return handle_apps_start(svc, body);
        if (strncmp(path, "/api/v1/apps/stop", 17) == 0) return handle_apps_stop(svc, body);
    }

    if (strncmp(path, "/api/v1/system/health", 21) == 0) return handle_system_health(svc);
    if (strncmp(path, "/api/v1/system/stats", 20) == 0) return handle_system_stats(svc);
    if (strncmp(path, "/api/v1/contracts", 17) == 0) return handle_contracts_get(svc);
    if (strncmp(path, "/api/v1/devices", 15) == 0) return handle_devices_get(svc);
    if (strncmp(path, "/api/v1/io/raw/latest", 21) == 0) return handle_io_raw_latest();
    if (strncmp(path, "/api/v1/stream/topology", 23) == 0) return handle_stream_topology(svc);
    if (strncmp(path, "/api/v1/stream/snapshot", 23) == 0) return handle_buffer_snapshot(svc);
    if (strncmp(path, "/api/v1/apps/list", 17) == 0) return handle_apps_list(svc);
    if (strncmp(path, "/api/v1/data/latest/", 20) == 0)
        return handle_data_latest(svc, (uint32_t)atoi(path + 20));
    return strdup("{\"error\":\"not_found\"}");
}

static void on_write(uv_write_t *req, int status) {
    (void)status;
    free(req->data);
    free(req);
}
static void on_alloc(uv_handle_t *h, size_t suggested, uv_buf_t *buf) {
    (void)h;
    buf->base = malloc(suggested);
    buf->len = (unsigned int)suggested;
}

static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        rest_server_t *srv = client->data;
        char path[128] = {0}, method[16] = {0};
        sscanf(buf->base, "%15s %127s", method, path);
        char *body = strstr(buf->base, "\r\n\r\n");
        if (body) body += 4;
        char *json = rest_dispatch(srv->svc, method, path, body);
        size_t json_len = strlen(json);
        char *resp = malloc(json_len + 512);
        size_t resp_len = (size_t)sprintf(resp, HTTP_RESPONSE_FMT, json_len, json);
        uv_write_t *write_req = malloc(sizeof(uv_write_t));
        write_req->data = resp;
        uv_buf_t write_buf = uv_buf_init(resp, (unsigned int)resp_len);
        uv_write(write_req, client, &write_buf, 1, on_write);
        free(json);
    }
    if (nread < 0) {
        if (!uv_is_closing((uv_handle_t *)client)) {
            uv_close((uv_handle_t *)client, (uv_close_cb)free);
        }
    }
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
    int rc = uv_tcp_bind(&srv->server, (const struct sockaddr *)&addr, 0);
    if (rc != 0) {
        fprintf(stderr, "[REST] Failed to bind to port %d: %s\n", port, uv_strerror(rc));
        return srv;
    }
    rc = uv_listen((uv_stream_t *)&srv->server, 128, on_new_connection);
    if (rc != 0) {
        fprintf(stderr, "[REST] Failed to listen on port %d: %s\n", port, uv_strerror(rc));
    } else {
        fprintf(stderr, "[REST] Server listening on port %d\n", port);
    }
    return srv;
}

static void _on_rest_server_close(uv_handle_t *h) {
    rest_server_t *srv = (rest_server_t *)h->data;
    free(srv);
}
void rest_server_destroy(rest_server_t *srv) {
    if (srv && !uv_is_closing((uv_handle_t *)&srv->server)) {
        uv_close((uv_handle_t *)&srv->server, _on_rest_server_close);
    }
}
