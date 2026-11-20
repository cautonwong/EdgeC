#include "iems_north_iec104.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"
#include "iems_driver_adapter.h"
#include "iems_southbound_adapter.h"
#include "io_executor.h"

/* ==================== TCP 监听 ==================== */

/* 链路错误只标记过期并停读：回调仍在句柄的读路径里，这里 destroy 会踩到自己。
 * 接管或 close 时再销毁；旧句柄的迟到事件按 active_io 过滤掉。 */
static void _on_client_link_event(io_handle_t *h, io_event_t evt, void *user_data) {
    iems_north_iec104_t *l = (iems_north_iec104_t *)user_data;
    if (!l || evt != IO_EVENT_ERROR) return;
    if (l->active_io != h) return;
    l->active_stale = true;
    io_handle_enable_read(h, false);
}

/* 驱动 t1 超时经从站端口调到这里：shutdown 让 io_executor 读路径收 EOF，
 * link_cb 随后把链路标成 stale。ctx 是监听器内嵌的 src，用来反推监听器。 */
static int _iec104_close_link(void *ctx) {
    if (!ctx) return -1;
    iems_north_iec104_t *l =
        (iems_north_iec104_t *)((char *)ctx - offsetof(iems_north_iec104_t, src));
    if (l->active_fd < 0) return -1;
    return shutdown(l->active_fd, SHUT_RDWR) == 0 ? 0 : -1;
}

static void _on_iec104_connect(uv_stream_t *server, int status) {
    if (status < 0) return;
    iems_north_iec104_t *l = (iems_north_iec104_t *)server->data;
    if (!l) return;

    uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
    if (!client) return;
    uv_tcp_init(server->loop, client);
    if (uv_accept(server, (uv_stream_t *)client) != 0) {
        uv_close((uv_handle_t *)client, (uv_close_cb)free);
        return;
    }

    /* 单客户端：活动链路还活着就拒绝第二路，避免覆盖引擎的 priv->io */
    if (l->active_io && !l->active_stale) {
        LOG_WARN(LOG_CAT_NB, "IEC104: 第二路客户端被拒绝（单客户端模式）");
        uv_close((uv_handle_t *)client, (uv_close_cb)free);
        return;
    }

    int fd = -1;
    if (uv_fileno((const uv_handle_t *)client, &fd) == 0 && fd >= 0) {
        int new_fd = dup(fd);
        if (new_fd >= 0) {
            int flags = fcntl(new_fd, F_GETFL, 0);
            if (flags >= 0) fcntl(new_fd, F_SETFL, flags | O_NONBLOCK);
            io_handle_t *h =
                io_executor_create_from_fd(server->loop, new_fd, datahub_get_io_pool(l->hub));
            if (!h) {
                close(new_fd);
            } else if (bus_orchestrator_attach_io(l->orch, l->bus_id, h) != 0) {
                io_handle_destroy(h);
            } else {
                io_handle_set_link_cb(h, _on_client_link_event, l);
                /* 先 attach 新句柄（引擎已改指新句柄），再换 active_io，最后销毁旧的：
                 * 反过来会留下悬垂的活动句柄 */
                io_handle_t *old = l->active_io;
                l->active_io = h;
                l->active_fd = new_fd;
                l->active_stale = false;
                if (old) io_handle_destroy(old);
            }
        }
    }
    uv_close((uv_handle_t *)client, (uv_close_cb)free);
}

int iems_north_iec104_init(iems_north_iec104_t *l, uv_loop_t *loop, datahub_t *hub,
                           bus_orchestrator_t *orch, iems_node_t *node, sqlite_svc_t *db,
                           const char *host, int port) {
    if (!l || !loop || !hub || !orch || !node) return -1;
    memset(l, 0, sizeof(*l));
    l->hub = hub;
    l->orch = orch;
    l->bus_id = -1;
    l->active_fd = -1;
    l->src = (iems_iec104_source_ctx_t){.hub = hub, .node = node, .db = db, .north_bus_id = -1};

    uint32_t conn_count = 0;
    const edge_connectivity_t *conns = iems_get_connectivity(&conn_count);
    if (!conns) return -1;

    for (uint32_t i = 0; i < conn_count; i++) {
        const edge_connectivity_t *c = &conns[i];
        if (strcmp(c->category, "NORTHBOUND") != 0 || !c->type || strcmp(c->type, "IEC104") != 0) {
            continue;
        }
        if (c->id < 0 || c->id >= IEMS_MAX_VBUS) {
            LOG_ERROR(LOG_CAT_CORE, "IEMS-INIT: IEC104 northbound bus id %d out of range", c->id);
            return -1;
        }
        if (bus_orchestrator_setup_slave(orch, c->id, "IEC104", 1) != 0) {
            LOG_ERROR(LOG_CAT_CORE, "IEMS-INIT: IEC104 northbound bus %d setup failed", c->id);
            return -1;
        }
        /* 基端口来自产品适配器；这里只补协议定时器需要的断链能力。
         * session_manager_set_slave_source 按值拷贝，栈上合成即可。 */
        edge_slave_source_ops_t ops = *iems_north_iec104_source_ops();
        ops.close_link = _iec104_close_link;
        bus_orchestrator_set_slave_source(orch, c->id, &ops, &l->src);
        if (l->bus_id >= 0) {
            LOG_WARN(LOG_CAT_CORE,
                     "IEMS-INIT: IEC104 northbound bus %d shares port with bus %d; "
                     "only the first one listens on TCP",
                     c->id, l->bus_id);
            continue;
        }
        l->bus_id = c->id;
        l->src.north_bus_id = c->id;
    }

    if (l->bus_id < 0) {
        LOG_WARN(LOG_CAT_CORE, "IEMS-INIT: contract table has no IEC104 NORTHBOUND entry");
        return 0;
    }

    if (!host || !host[0]) host = "0.0.0.0";
    if (port < 0) port = 2404;

    l->srv = calloc(1, sizeof(uv_tcp_t));
    if (!l->srv) return -1;
    uv_tcp_init(loop, l->srv);
    l->srv->data = l;

    struct sockaddr_in addr;
    int rc = uv_ip4_addr(host, port, &addr);
    if (rc == 0) rc = uv_tcp_bind(l->srv, (const struct sockaddr *)&addr, 0);
    if (rc == 0) rc = uv_listen((uv_stream_t *)l->srv, 128, _on_iec104_connect);
    if (rc != 0) {
        LOG_ERROR(LOG_CAT_CORE, "IEMS-INIT: IEC104 listen on %s:%d failed: %s", host, port,
                  uv_strerror(rc));
        uv_close((uv_handle_t *)l->srv, (uv_close_cb)free);
        l->srv = NULL;
        return -1;
    }

    struct sockaddr_storage bound;
    int bound_len = sizeof(bound);
    if (uv_tcp_getsockname(l->srv, (struct sockaddr *)&bound, &bound_len) == 0 &&
        bound.ss_family == AF_INET) {
        l->port = ntohs(((struct sockaddr_in *)&bound)->sin_port);
    }
    LOG_INFO(LOG_CAT_CORE, "IEMS-INIT: IEC104 northbound LISTENING at %s:%d (bus %d)", host,
             l->port, l->bus_id);
    return 0;
}

void iems_north_iec104_close(iems_north_iec104_t *l) {
    if (!l) return;
    /* uv_run 之前销毁活动句柄；置空保证重复 close 幂等 */
    if (l->active_io) {
        io_handle_destroy(l->active_io);
        l->active_io = NULL;
        l->active_fd = -1;
        l->active_stale = false;
    }
    if (!l->srv) return;
    if (!uv_is_closing((uv_handle_t *)l->srv)) {
        uv_close((uv_handle_t *)l->srv, (uv_close_cb)free);
    }
    l->srv = NULL;
}
