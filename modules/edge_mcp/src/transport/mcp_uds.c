/**
 * @file mcp_uds.c
 * @brief Industrial Shared MCP Bus with Multi-Client UDS Transport (AEP-510)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>
#include <cjson/cJSON.h>

#include "edgeos/mcp_transport.h"
#include "edgeos/edge_mcp_dispatcher.h"

#define MAX_UDS_CLIENTS 32
#define UDS_RECV_BUF_SIZE 65536

typedef struct uds_client_s {
    uv_pipe_t *pipe;
    mcp_transport_t *trans;
    char rx_buf[UDS_RECV_BUF_SIZE];
    size_t rx_len;
    bool active;
    bool closing;
} uds_client_t;

typedef struct {
    uv_pipe_t *server_pipe;
    char socket_path[256];
    uds_client_t clients[MAX_UDS_CLIENTS];
    bool is_running;
} uds_server_priv_t;

static void uds_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    (void)handle;
    buf->base = (char *)malloc(suggested_size);
    buf->len = (buf->base) ? suggested_size : 0;
}

static void on_client_close(uv_handle_t *handle) {
    if (!handle) return;
    if (handle->data) {
        uds_client_t *client = (uds_client_t *)handle->data;
        client->active = false;
        client->closing = false;
        client->rx_len = 0;
        client->pipe = NULL;
    }
    free(handle);
}

static void on_server_close(uv_handle_t *handle) {
    if (handle) {
        free(handle);
    }
}

static void on_uds_write(uv_write_t *req, int status) {
    (void)status;
    if (req) {
        if (req->data) {
            free(req->data);
        }
        free(req);
    }
}

static void uds_process_line(uds_client_t *client, const char *line) {
    if (!client || !client->active || client->closing || !client->pipe || !line || strlen(line) == 0) {
        return;
    }

    char *resp = edge_mcp_dispatch_json(client->trans->disp, line);
    if (!resp) {
        return;
    }

    size_t resp_len = strlen(resp);
    if (resp_len == 0) {
        free(resp);
        return;
    }

    bool has_newline = (resp[resp_len - 1] == '\n');
    size_t total_len = resp_len + (has_newline ? 0 : 1);

    char *send_buf = (char *)malloc(total_len + 1);
    if (!send_buf) {
        free(resp);
        return;
    }

    memcpy(send_buf, resp, resp_len);
    if (!has_newline) {
        send_buf[resp_len] = '\n';
    }
    send_buf[total_len] = '\0';
    free(resp);

    uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
    if (!req) {
        free(send_buf);
        return;
    }

    req->data = send_buf;
    uv_buf_t write_buf = uv_buf_init(send_buf, (unsigned int)total_len);
    int rc = uv_write(req, (uv_stream_t *)client->pipe, &write_buf, 1, on_uds_write);
    if (rc != 0) {
        free(send_buf);
        free(req);
    }
}

static void uds_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    uds_client_t *client = (uds_client_t *)stream->data;
    if (nread > 0) {
        if (client && client->active && !client->closing) {
            for (ssize_t i = 0; i < nread; i++) {
                char c = buf->base[i];
                if (c == '\n' || c == '\r') {
                    if (client->rx_len > 0) {
                        client->rx_buf[client->rx_len] = '\0';
                        uds_process_line(client, client->rx_buf);
                        client->rx_len = 0;
                    }
                } else {
                    if (client->rx_len < sizeof(client->rx_buf) - 1) {
                        client->rx_buf[client->rx_len++] = c;
                    } else {
                        /* Buffer overflow protection */
                        client->rx_len = 0;
                    }
                }
            }
        }
    } else if (nread < 0) {
        if (client && client->active && !client->closing && client->pipe) {
            client->closing = true;
            if (!uv_is_closing((uv_handle_t *)client->pipe)) {
                uv_close((uv_handle_t *)client->pipe, on_client_close);
            }
        }
    }

    if (buf->base) {
        free(buf->base);
    }
}

static void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) return;
    mcp_transport_t *trans = (mcp_transport_t *)server->data;
    if (!trans || !trans->priv) return;
    uds_server_priv_t *priv = (uds_server_priv_t *)trans->priv;

    uds_client_t *client = NULL;
    for (int i = 0; i < MAX_UDS_CLIENTS; i++) {
        if (!priv->clients[i].active && !priv->clients[i].closing) {
            client = &priv->clients[i];
            break;
        }
    }

    if (!client) {
        /* Pool is full (32 concurrent clients): accept and close cleanly */
        uv_pipe_t *reject_pipe = (uv_pipe_t *)malloc(sizeof(uv_pipe_t));
        if (reject_pipe) {
            uv_pipe_init(trans->loop, reject_pipe, 0);
            if (uv_accept(server, (uv_stream_t *)reject_pipe) == 0) {
                uv_close((uv_handle_t *)reject_pipe, (uv_close_cb)free);
            } else {
                uv_close((uv_handle_t *)reject_pipe, (uv_close_cb)free);
            }
        }
        return;
    }

    client->pipe = (uv_pipe_t *)malloc(sizeof(uv_pipe_t));
    if (!client->pipe) return;

    memset(client->rx_buf, 0, sizeof(client->rx_buf));
    client->trans = trans;
    client->active = true;
    client->closing = false;
    client->rx_len = 0;

    uv_pipe_init(trans->loop, client->pipe, 0);
    client->pipe->data = client;

    if (uv_accept(server, (uv_stream_t *)client->pipe) == 0) {
        uv_read_start((uv_stream_t *)client->pipe, uds_alloc_cb, uds_read_cb);
    } else {
        client->closing = true;
        uv_close((uv_handle_t *)client->pipe, on_client_close);
    }
}

static int uds_start(mcp_transport_t *trans) {
    uds_server_priv_t *priv = (uds_server_priv_t *)trans->priv;
    if (priv->is_running) return 0;

    /* Remove existing stale socket file if present before binding */
    unlink(priv->socket_path);

    priv->server_pipe = (uv_pipe_t *)malloc(sizeof(uv_pipe_t));
    if (!priv->server_pipe) return -1;

    uv_pipe_init(trans->loop, priv->server_pipe, 0);
    priv->server_pipe->data = trans;

    int rc = uv_pipe_bind(priv->server_pipe, priv->socket_path);
    if (rc != 0) {
        free(priv->server_pipe);
        priv->server_pipe = NULL;
        return rc;
    }

    rc = uv_listen((uv_stream_t *)priv->server_pipe, 128, on_new_connection);
    if (rc == 0) {
        priv->is_running = true;
    } else {
        free(priv->server_pipe);
        priv->server_pipe = NULL;
    }
    return rc;
}

static void uds_stop(mcp_transport_t *trans) {
    uds_server_priv_t *priv = (uds_server_priv_t *)trans->priv;
    if (priv && priv->is_running) {
        if (priv->server_pipe) {
            if (!uv_is_closing((uv_handle_t *)priv->server_pipe)) {
                uv_close((uv_handle_t *)priv->server_pipe, on_server_close);
            }
            priv->server_pipe = NULL;
        }
        for (int i = 0; i < MAX_UDS_CLIENTS; i++) {
            if (priv->clients[i].active && priv->clients[i].pipe) {
                if (!uv_is_closing((uv_handle_t *)priv->clients[i].pipe)) {
                    priv->clients[i].closing = true;
                    uv_close((uv_handle_t *)priv->clients[i].pipe, on_client_close);
                }
            }
        }
        unlink(priv->socket_path);
        priv->is_running = false;
    }
}

static void uds_destroy(mcp_transport_t *trans) {
    if (!trans) return;
    uds_stop(trans);
    uds_server_priv_t *priv = (uds_server_priv_t *)trans->priv;
    if (priv) {
        for (int i = 0; i < MAX_UDS_CLIENTS; i++) {
            if (priv->clients[i].pipe) {
                priv->clients[i].pipe->data = NULL;
            }
        }
        unlink(priv->socket_path);
        free(priv);
    }
    free(trans);
}

mcp_transport_t *mcp_transport_uds_create(uv_loop_t *loop,
                                         edge_mcp_dispatcher_t *disp,
                                         const char *socket_path) {
    if (!loop || !disp || !socket_path || strlen(socket_path) == 0) return NULL;
    mcp_transport_t *trans = (mcp_transport_t *)calloc(1, sizeof(mcp_transport_t));
    if (!trans) return NULL;

    uds_server_priv_t *priv = (uds_server_priv_t *)calloc(1, sizeof(uds_server_priv_t));
    if (!priv) {
        free(trans);
        return NULL;
    }

    strncpy(priv->socket_path, socket_path, sizeof(priv->socket_path) - 1);

    trans->disp = disp;
    trans->loop = loop;
    trans->priv = priv;
    trans->start = uds_start;
    trans->stop = uds_stop;
    trans->destroy = uds_destroy;

    return trans;
}
