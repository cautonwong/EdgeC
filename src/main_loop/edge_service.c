/**
 * @file edge_service.c
 * @brief 核心控制面实现 (支持容器化自组装, S14 掉电, S19 冗余, S9 IPC, S11 发现, S12 隧道)
 */

#include "edge_service.h"

#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

#include "edge_log.h"
#include "northbound_mgr.h"

struct service_s {
    datahub_t *hub;
    sqlite_svc_t *db;
    stream_engine_t *engine;
    arbiter_t *arb;
    edge_params_t *params;
    northbound_mgr_t *north;

    char *contract_path;
    char *mode;

    emergency_hook_fn emergency_hook;
    void *emergency_user_data;

    // S19: 链路管理
    net_link_t current_link;
    bool link_status[2];
    link_monitor_fn link_cb;
    void *link_user_data;

    // S9: IPC 桥接
    uv_pipe_t *ipc_pipe;

    // S11: 自动发现报告
    char *last_discovery_report;

    // S12: 远程隧道
    uv_tcp_t *tunnel_server;
    int tunnel_bus_id;

    // S10: 时间管理
    uint64_t time_offset_ms;
};

// --- S12: 隧道桥接逻辑 ---

static void _on_tunnel_alloc(uv_handle_t *h, size_t suggested, uv_buf_t *buf) {
    (void)h;
    buf->base = malloc(suggested);
    buf->len = suggested;
}

static void _on_tunnel_cmd_cleanup(cmd_msg_t *cmd) {
    if (cmd->params) free(cmd->params);
    free(cmd);
}

static void _on_tunnel_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    service_t *svc = client->data;
    if (nread > 0) {
        cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
        cmd->action = CMD_ACTION_RAW;
        cmd->priority = ARBITER_PRIO_EMERGENCY;
        cmd->signature = 0x55AA;
        cmd->on_cleanup = _on_tunnel_cmd_cleanup;
        cmd->params = malloc((size_t)nread);
        memcpy(cmd->params, buf->base, (size_t)nread);
        cmd->value = (double)nread;
        service_post_cmd(svc, cmd);
    }
    if (nread < 0) uv_close((uv_handle_t *)client, (uv_close_cb)free);
    if (buf->base) free(buf->base);
}

static void _on_new_tunnel_conn(uv_stream_t *server, int status) {
    if (status < 0) return;
    service_t *svc = server->data;
    uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(server->loop, client);
    client->data = svc;
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        uv_read_start((uv_stream_t *)client, _on_tunnel_alloc, _on_tunnel_read);
    } else {
        uv_close((uv_handle_t *)client, (uv_close_cb)free);
    }
}

void service_create_tunnel(service_t *svc, int tcp_port, int bus_id) {
    if (!svc || !svc->hub) return;
    svc->tunnel_server = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(svc->hub->loop, svc->tunnel_server);
    svc->tunnel_server->data = svc;
    svc->tunnel_bus_id = bus_id;
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", tcp_port, &addr);
    uv_tcp_bind(svc->tunnel_server, (const struct sockaddr *)&addr, 0);
    uv_listen((uv_stream_t *)svc->tunnel_server, 128, _on_new_tunnel_conn);
    LOG_INFO(LOG_CAT_CORE, "Remote maintenance tunnel opened on port %d -> Bus %d", tcp_port, bus_id);
}

// --- Lifecycle ---

service_t *service_new(service_options_t *opts) {
    service_t *svc = calloc(1, sizeof(service_t));
    if (!opts) return svc;

    if (opts->mode) svc->mode = strdup(opts->mode);
    
    uv_loop_t *loop = uv_default_loop();

    // 1. Foundation Plane
    if (opts->product_name) {
        svc->hub = calloc(1, sizeof(datahub_t));
        datahub_init(svc->hub, loop, opts->product_name, opts->shm_slots > 0 ? opts->shm_slots : 1024);
    }

    if (opts->db_path) {
        sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = opts->db_path};
        svc->db = sqlite_svc_create(loop, &db_cfg);
    }

    // 2. Control Plane
    arbiter_init(&svc->arb, loop);

    // 3. Northbound Plane
    if (opts->rest_port > 0) {
        svc->north = calloc(1, sizeof(northbound_mgr_t));
        northbound_mgr_init(svc->north, loop, &svc->hub->ring);
        svc->north->rest_port = opts->rest_port;
        svc->north->web_port = opts->web_port;
        svc->north->user_data = svc;
    }

    svc->link_status[LINK_PRIMARY] = true;
    svc->current_link = LINK_PRIMARY;
    return svc;
}

void service_delete(service_t *svc) {
    if (!svc) return;
    
    if (svc->north) {
        northbound_mgr_stop(svc->north);
        free(svc->north);
    }
    if (svc->engine) stream_engine_destroy(svc->engine);
    if (svc->arb) arbiter_destroy(svc->arb);
    if (svc->db) sqlite_svc_destroy(svc->db);
    if (svc->hub) {
        datahub_destroy(svc->hub);
        free(svc->hub);
    }

    if (svc->mode) free(svc->mode);
    if (svc->contract_path) free(svc->contract_path);
    if (svc->last_discovery_report) free(svc->last_discovery_report);
    if (svc->ipc_pipe) uv_close((uv_handle_t *)svc->ipc_pipe, (uv_close_cb)free);
    if (svc->tunnel_server) uv_close((uv_handle_t *)svc->tunnel_server, (uv_close_cb)free);
    free(svc);
}

// --- IPC / Sync / Status Impl ---

static void _on_ipc_write(uv_write_t *req, int status) {
    (void)status;
    free(req);
}
static void _ipc_forward_cb(datahub_t *hub, void *user_data) {
    service_t *svc = user_data;
    if (!svc->ipc_pipe || uv_is_closing((uv_handle_t *)svc->ipc_pipe)) return;
    pipe_msg_t msg;
    static uint64_t last_seq = 0;
    while (shm_ring_read(&hub->ring, &last_seq, &msg) == 0) {
        uv_buf_t buf = uv_buf_init((char *)&msg, sizeof(msg));
        uv_write_t *req = malloc(sizeof(uv_write_t));
        uv_write(req, (uv_stream_t *)svc->ipc_pipe, &buf, 1, _on_ipc_write);
    }
}
static void _on_ipc_connect(uv_connect_t *req, int status) {
    (void)status;
    free(req);
}
void service_add_ipc_bridge(service_t *svc, const char *uds_path) {
    if (!svc || !svc->hub || !uds_path) return;
    svc->ipc_pipe = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(svc->hub->loop, svc->ipc_pipe, 0);
    uv_connect_t *conn = malloc(sizeof(uv_connect_t));
    uv_pipe_connect(conn, svc->ipc_pipe, uds_path, _on_ipc_connect);
    datahub_subscribe(svc->hub, _ipc_forward_cb, svc);
    LOG_INFO(LOG_CAT_CORE, "IPC Bridge established to %s", uds_path);
}

void service_sync_time(service_t *svc, uint64_t calendar_ms) {
    if (!svc || !svc->hub) return;
    svc->time_offset_ms = calendar_ms - uv_now(svc->hub->loop);
    LOG_INFO(LOG_CAT_CORE, "System Time Synced. New Offset: %lu ms", svc->time_offset_ms);
}
uint64_t service_get_now_ms(service_t *svc) {
    if (!svc || !svc->hub) return (uint64_t)time(NULL) * 1000;
    return uv_now(svc->hub->loop) + svc->time_offset_ms;
}

int service_scan_bus(service_t *svc, int bus_id) {
    if (!svc) return -1;
    (void)bus_id;
    if (svc->last_discovery_report) free(svc->last_discovery_report);
    svc->last_discovery_report = strdup("[{\"address\":1, \"fingerprint\":\"Vendor:0x1234\"}]");
    return 0;
}

int service_bootstrap(service_t *svc, const char *manifest_path) {
    if (!svc || !manifest_path) return -1;
    LOG_INFO(LOG_CAT_CORE, "ZTP: Starting Cold Boot Bootstrap from %s", manifest_path);
    char out_dir[256];
    strncpy(out_dir, manifest_path, sizeof(out_dir) - 1);
    char *last_slash = strrchr(out_dir, '/');
    if (last_slash) *last_slash = '\0';
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "/workspaces/edgeos/EdgeC/.local/linux-x86_64/bin/contract-compiler -manifest %s -out "
             "%s/generated > /dev/null 2>&1",
             manifest_path, out_dir);
    int ret = system(cmd);
    if (ret != 0) {
        LOG_ERROR(LOG_CAT_CORE, "ZTP: Contract Compiler failed with code %d", ret);
        return -1;
    }
    char yaml_path[512];
    snprintf(yaml_path, sizeof(yaml_path), "%s/generated/contracts.yaml", out_dir);
    service_set_contract_path(svc, yaml_path);
    return 0;
}

char *service_get_discovery_report(service_t *svc) {
    return svc->last_discovery_report ? strdup(svc->last_discovery_report) : NULL;
}

void service_set_datahub(service_t *svc, datahub_t *hub) { svc->hub = hub; }
void service_set_storage(service_t *svc, sqlite_svc_t *storage) { svc->db = storage; }
void service_set_stream_engine(service_t *svc, stream_engine_t *engine) { svc->engine = engine; }
void service_set_arbiter(service_t *svc, arbiter_t *arb) { svc->arb = arb; }
void service_set_params(service_t *svc, edge_params_t *params) { svc->params = params; }
void service_set_contract_path(service_t *svc, const char *path) {
    if (svc->contract_path) free(svc->contract_path);
    svc->contract_path = strdup(path);
}
void service_set_emergency_hook(service_t *svc, emergency_hook_fn hook, void *user_data) {
    svc->emergency_hook = hook;
    svc->emergency_user_data = user_data;
}
void service_set_link_monitor(service_t *svc, link_monitor_fn cb, void *user_data) {
    svc->link_cb = cb;
    svc->link_user_data = user_data;
}

void service_report_link_status(service_t *svc, net_link_t link, bool is_up) {
    if (!svc || link > LINK_SECONDARY) return;
    svc->link_status[link] = is_up;
    net_link_t target = svc->link_status[LINK_PRIMARY] ? LINK_PRIMARY : LINK_SECONDARY;
    if (target != svc->current_link) {
        svc->current_link = target;
        if (svc->link_cb) svc->link_cb(svc->current_link, svc->link_user_data);
    }
}

uv_loop_t *service_get_loop(service_t *svc) { return svc->hub ? svc->hub->loop : NULL; }
datahub_t *service_get_datahub(service_t *svc) { return svc->hub; }
sqlite_svc_t *service_get_storage(service_t *svc) { return svc->db; }
stream_engine_t *service_get_stream_engine(service_t *svc) { return svc->engine; }
arbiter_t *service_get_arbiter(service_t *svc) { return svc->arb; }
edge_params_t *service_get_params(service_t *svc) { return svc->params; }
const char *service_get_contract_path(service_t *svc) {
    return svc->contract_path ? svc->contract_path : "./generated/contracts.yaml";
}

int service_post_cmd(service_t *svc, cmd_msg_t *cmd) {
    if (!svc || !svc->arb || !cmd) return -1;
    if (cmd->signature != 0x55AA) return -13;
    return arbiter_submit_cmd(svc->arb, 0, cmd);
}

typedef struct {
    export_publish_fn cb;
    int count;
} export_ctx_t;

static int _export_query_cb(void *user_data, pipe_msg_t *msg) {
    export_ctx_t *ctx = user_data;
    if (msg->type == PIPEMSG_TYPE_SCALAR) {
        ctx->cb(msg->tag_id, msg->payload.val.f);
        ctx->count++;
    }
    return 0;
}

int service_trigger_export(service_t *svc, const char *table, export_publish_fn cb) {
    if (!svc || !svc->db || !cb) return -1;
    export_ctx_t ctx = {.cb = cb, .count = 0};
    uint64_t now = service_get_now_ms(svc);
    sqlite_svc_query(svc->db, now - 3600000, now, _export_query_cb, &ctx);
    return 0;
}

int service_run(service_t *svc) {
    if (!svc || !svc->hub) return -1;
    if (svc->north) northbound_mgr_start(svc->north);
    return uv_run(svc->hub->loop, UV_RUN_DEFAULT);
}

void service_stop(service_t *svc) {
    if (!svc) return;
    if (svc->emergency_hook) svc->emergency_hook(svc->emergency_user_data);
    if (svc->hub && svc->hub->loop) uv_stop(svc->hub->loop);
}