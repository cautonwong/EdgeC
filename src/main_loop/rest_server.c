#include "rest_server.h"

#include <cJSON/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "edge_params.h"
#include "edge_service.h"

struct rest_server_s {
    uv_loop_t *loop;
    service_t *svc;
    int port;
    uv_tcp_t server;
};

// --- REST Handlers based on web_ui_ia.md Contract ---

static char *handle_system_health(service_t *svc) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");

    // Observability: showing internal state
    cJSON *storage = cJSON_AddObjectToObject(root, "storage");
    cJSON_AddNumberToObject(storage, "queue_depth", 0);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static char *handle_data_latest(service_t *svc, uint32_t tag_id) {
    datahub_t *hub = service_get_datahub(svc);
    PipeMsg msg;
    cJSON *root = cJSON_CreateObject();

    // Principles 1 & Contract: Non-intrusive Shadow Read
    if (datahub_read_shadow(hub, tag_id, &msg) == 0) {
        cJSON_AddNumberToObject(root, "tag_id", msg.tag_id);
        cJSON_AddNumberToObject(root, "value", msg.payload.val.f);
        cJSON_AddNumberToObject(root, "ts", (double)msg.ts);
        cJSON_AddStringToObject(root, "quality", "good");
    } else {
        cJSON_AddStringToObject(root, "status", "not_found");
    }

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

static char *handle_params_list(service_t *svc) {
    (void)svc;
    cJSON *root = cJSON_CreateObject();
    cJSON *params = cJSON_AddArrayToObject(root, "params");

    // This would iterate through edge_params in a real app
    // Demoing the structure from IA
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "key", "max_charge_limit");
    cJSON_AddNumberToObject(p1, "value", edge_param_get_float("max_charge_limit", 3000.0));
    cJSON_AddItemToArray(params, p1);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

// --- Dispatcher and Lifecycle ---

rest_server_t *rest_server_create(uv_loop_t *loop, service_t *svc, int port) {
    rest_server_t *srv = calloc(1, sizeof(rest_server_t));
    if (!srv) return NULL;

    srv->loop = loop;
    srv->svc = svc;
    srv->port = port;

    uv_tcp_init(loop, &srv->server);
    srv->server.data = srv;

    printf("[REST] Server initialized on port %d (Frozen IA v1.0)\n", port);
    return srv;
}

void rest_server_destroy(rest_server_t *srv) {
    if (!srv) return;
    if (!uv_is_closing((uv_handle_t *)&srv->server)) {
        uv_close((uv_handle_t *)&srv->server, NULL);
    }
    free(srv);
}

char *rest_dispatch(service_t *svc, const char *path) {
    if (strcmp(path, "/api/v1/system/health") == 0) {
        return handle_system_health(svc);
    }
    if (strncmp(path, "/api/v1/data/latest/", 20) == 0) {
        uint32_t tag_id = (uint32_t)atoi(path + 20);
        return handle_data_latest(svc, tag_id);
    }
    if (strcmp(path, "/api/v1/system/config") == 0) {
        return handle_params_list(svc);
    }
    return strdup("{\"error\":\"not_found\"}");
}
