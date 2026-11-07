#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver_manager.h"
#include "edge_api.h"
#include "edge_log.h"
#include "iems_driver_adapter.h"
#include "iems_north_listener.h"
#include "io_executor.h"

static void _on_north_connect(uv_stream_t *server, int status) {
    if (status < 0) return;
    iems_north_accept_ctx_t *ctx = (iems_north_accept_ctx_t *)server->data;
    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t *)client) == 0 && ctx && ctx->orch) {
        int fd;
        uv_fileno((const uv_handle_t *)client, &fd);
        int new_fd = dup(fd);
        if (new_fd >= 0) {
            int flags = fcntl(new_fd, F_GETFL, 0);
            fcntl(new_fd, F_SETFL, flags | O_NONBLOCK);
            io_handle_t *h =
                io_executor_create_from_fd(server->loop, new_fd, datahub_get_io_pool(ctx->hub));
            if (h) {
                bus_orchestrator_attach_io(ctx->orch, ctx->bus_id, h);
                if (h->ops && h->ops->enable_read) h->ops->enable_read(h, true);
            } else {
                close(new_fd);
            }
        }
    }
    uv_close((uv_handle_t *)client, (uv_close_cb)free);
}

void iems_north_listener_start(iems_north_listener_t *l, uv_loop_t *loop, datahub_t *hub,
                               bus_orchestrator_t *orch, const char *first_sock,
                               bus_conn_state_t *bus_state) {
    if (!l || !loop || !hub || !orch) return;

    uint32_t conn_count = 0;
    const edge_connectivity_t *conns = iems_get_connectivity(&conn_count);
    if (!conns) return;

    int seen = 0;
    for (uint32_t i = 0; i < conn_count; i++) {
        const edge_connectivity_t *c = &conns[i];
        if (strcmp(c->category, "NORTHBOUND") != 0 || c->id < 0 || c->id >= IEMS_MAX_VBUS) continue;

        // first_sock 只覆盖第一条 NORTHBOUND(单总线是常态);其余条目用契约表
        // 自带的 simulation_path,否则第二条会 unlink 掉第一条正在监听的 socket
        bool from_env = (seen == 0 && first_sock && first_sock[0]);
        const char *path = NULL;
        if (from_env) {
            path = first_sock;
        } else if (c->simulation_path && c->simulation_path[0]) {
            path = c->simulation_path;
            if (seen > 0) {
                LOG_WARN(LOG_CAT_CORE,
                         "IEMS-INIT: NORTHBOUND bus %d uses contract path %s "
                         "(IEMS_NORTH_SOCK 只覆盖第一条北向总线)",
                         (int)c->id, path);
            }
        } else if (seen == 0) {
            path = "/tmp/iems_north_slave.sock";
        } else {
            // 共用同一个 socket 会让后面的 unlink 掉前面正在监听的文件,宁可不监听
            LOG_ERROR(LOG_CAT_CORE,
                      "IEMS-INIT: NORTHBOUND bus %d has no path in contract table; skipped "
                      "(refusing to share another bus's socket)",
                      (int)c->id);
            continue;
        }
        seen++;

        uv_pipe_t *srv = (uv_pipe_t *)malloc(sizeof(uv_pipe_t));
        uv_pipe_init(loop, srv, 0);
        unlink(path);

        int bind_rc = uv_pipe_bind(srv, path);
        int listen_rc = (bind_rc == 0) ? uv_listen((uv_stream_t *)srv, 128, _on_north_connect)
                                       : bind_rc;
        if (bind_rc == 0 && listen_rc == 0) {
            l->ctx[c->id].bus_id = c->id;
            l->ctx[c->id].hub = hub;
            l->ctx[c->id].orch = orch;
            srv->data = &l->ctx[c->id];
            l->srv[c->id] = srv;
            if (bus_state) bus_state[c->id] = BUS_STATE_LISTENING;
            printf("\033[32m[IEMS-INIT] Northbound Slave LISTENING at %s\033[0m\n", path);
        } else {
            LOG_ERROR(LOG_CAT_CORE,
                      "IEMS-INIT: Northbound Slave FAILED at %s (bind=%s, listen=%s)", path,
                      uv_strerror(bind_rc), uv_strerror(listen_rc));
            if (bind_rc == 0) unlink(path);
            uv_close((uv_handle_t *)srv, (uv_close_cb)free);
        }
    }
}

void iems_north_listener_close(iems_north_listener_t *l) {
    if (!l) return;
    for (int i = 0; i < IEMS_MAX_VBUS; i++) {
        if (l->srv[i]) {
            uv_close((uv_handle_t *)l->srv[i], (uv_close_cb)free);
            l->srv[i] = NULL;
        }
    }
}
