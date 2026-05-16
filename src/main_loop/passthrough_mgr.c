#include "passthrough_mgr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "edge_log.h"
#include "io_executor.h"

#define MAX_PASSTHROUGH_SERVERS 8

typedef struct {
    uv_tcp_t server;
    passthrough_mgr_t *mgr;
    int bus_id;
    bool session_active;
    
    uv_tcp_t client;
    io_handle_t *hijacked_io;
    void (*orig_cb)(io_handle_t *h, io_event_t evt, void *payload, void *user_data);
    void *orig_user_data;
} passthrough_server_t;

struct passthrough_mgr_s {
    uv_loop_t *loop;
    bus_orchestrator_t *orch;
    arbiter_t *arb;
    passthrough_server_t servers[MAX_PASSTHROUGH_SERVERS];
    int server_count;
};

static void _on_client_close(uv_handle_t *handle) {
    passthrough_server_t *srv = (passthrough_server_t *)handle->data;
    srv->session_active = false;
    
    // Restore IO handle
    if (srv->hijacked_io) {
        srv->hijacked_io->cb = srv->orig_cb;
        srv->hijacked_io->user_data = srv->orig_user_data;
        srv->hijacked_io = NULL;
    }
    
    // Release exclusive hold
    arbiter_set_exclusive(srv->mgr->arb, srv->bus_id, false);
    LOG_INFO(LOG_CAT_BUS, "Passthrough: Session closed on Bus %d. Regular polling resumed.", srv->bus_id);
}

static void _on_io_event(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    passthrough_server_t *srv = (passthrough_server_t *)user_data;
    if (evt == IO_EVENT_RX && srv->session_active) {
        net_buf_t *buf = (net_buf_t *)payload;
        uv_write_t *req = malloc(sizeof(uv_write_t));
        uv_buf_t uvb = uv_buf_init((char *)buf->b.data, buf->b.len);
        req->data = buf;
        uv_write(req, (uv_stream_t *)&srv->client, &uvb, 1, NULL); 
    }
}

static void _alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

static void _on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    passthrough_server_t *srv = (passthrough_server_t *)client->data;
    if (nread > 0) {
        if (srv->hijacked_io) {
            // Forward to IO
            net_buf_pool_t *pool = bus_orchestrator_get_pool(srv->mgr->orch);
            net_buf_t *nb = net_buf_alloc(pool, 0);
            if (nb) {
                size_t to_copy = (size_t)nread < (size_t)nb->b.size ? (size_t)nread : (size_t)nb->b.size;
                memcpy(nb->b.data, buf->base, to_copy);
                nb->b.len = (uint16_t)to_copy;
                srv->hijacked_io->ops->write_async(srv->hijacked_io, nb);
            }
        }
    } else if (nread < 0) {
        uv_close((uv_handle_t *)client, _on_client_close);
    }
    if (buf->base) free(buf->base);
}

static void _on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) return;
    passthrough_server_t *srv = (passthrough_server_t *)server->data;
    
    if (srv->session_active) {
        // Refuse multiple sessions
        uv_tcp_t tmp;
        uv_tcp_init(srv->mgr->loop, &tmp);
        uv_accept(server, (uv_stream_t *)&tmp);
        uv_close((uv_handle_t *)&tmp, NULL);
        return;
    }

    uv_tcp_init(srv->mgr->loop, &srv->client);
    srv->client.data = srv;
    if (uv_accept(server, (uv_stream_t *)&srv->client) == 0) {
        LOG_INFO(LOG_CAT_BUS, "Passthrough: New connection for Bus %d. Attempting exclusive hold...", srv->bus_id);
        
        // Acquire exclusive hold
        arbiter_set_exclusive(srv->mgr->arb, srv->bus_id, true);
        
        // Hijack IO
        srv->hijacked_io = bus_orchestrator_get_io(srv->mgr->orch, srv->bus_id);
        if (srv->hijacked_io) {
            srv->orig_cb = srv->hijacked_io->cb;
            srv->orig_user_data = srv->hijacked_io->user_data;
            srv->hijacked_io->cb = _on_io_event;
            srv->hijacked_io->user_data = srv;
            
            srv->session_active = true;
            uv_read_start((uv_stream_t *)&srv->client, _alloc_cb, _on_read);
            LOG_INFO(LOG_CAT_BUS, "Passthrough: Exclusive hold acquired. Tunnel active.");
        } else {
            LOG_ERROR(LOG_CAT_BUS, "Passthrough: Failed to hijack Bus %d - No IO handle attached.", srv->bus_id);
            uv_close((uv_handle_t *)&srv->client, _on_client_close);
        }
    } else {
        uv_close((uv_handle_t *)&srv->client, NULL);
    }
}

passthrough_mgr_t *passthrough_mgr_create(uv_loop_t *loop, bus_orchestrator_t *orch, arbiter_t *arb) {
    passthrough_mgr_t *mgr = calloc(1, sizeof(passthrough_mgr_t));
    mgr->loop = loop;
    mgr->orch = orch;
    mgr->arb = arb;
    return mgr;
}

int passthrough_mgr_start(passthrough_mgr_t *mgr, const passthrough_config_t *cfgs, int count) {
    if (!mgr || !cfgs) return -1;
    int started = 0;
    for (int i = 0; i < count && mgr->server_count < MAX_PASSTHROUGH_SERVERS; i++) {
        passthrough_server_t *srv = &mgr->servers[mgr->server_count];
        srv->mgr = mgr;
        srv->bus_id = cfgs[i].target_bus_id;
        
        uv_tcp_init(mgr->loop, &srv->server);
        srv->server.data = srv;
        
        struct sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", cfgs[i].port, &addr);
        
        if (uv_tcp_bind(&srv->server, (const struct sockaddr *)&addr, 0) == 0 &&
            uv_listen((uv_stream_t *)&srv->server, 5, _on_new_connection) == 0) {
            LOG_INFO(LOG_CAT_BUS, "Passthrough: Listening on port %d for Bus %d", cfgs[i].port, srv->bus_id);
            mgr->server_count++;
            started++;
        } else {
            LOG_ERROR(LOG_CAT_BUS, "Passthrough: Failed to start server on port %d", cfgs[i].port);
        }
    }
    return started;
}

void passthrough_mgr_destroy(passthrough_mgr_t *mgr) {
    if (!mgr) return;
    for (int i = 0; i < mgr->server_count; i++) {
        uv_close((uv_handle_t *)&mgr->servers[i].server, NULL);
        if (mgr->servers[i].session_active) {
            uv_close((uv_handle_t *)&mgr->servers[i].client, NULL);
        }
    }
    free(mgr);
}
