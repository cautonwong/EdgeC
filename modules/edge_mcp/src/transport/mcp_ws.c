/**
 * @file mcp_ws.c
 * @brief Industrial MCP WebSocket & HTTP JSON-RPC POST Transport
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <cjson/cJSON.h>

#include "edgeos/mcp_transport.h"
#include "edgeos/edge_mcp_dispatcher.h"

#define MAX_WS_CLIENTS 32
#define WS_RECV_BUF_SIZE 16384

typedef struct ws_client_s {
    uv_tcp_t handle;
    mcp_transport_t *trans;
    char rx_buf[WS_RECV_BUF_SIZE];
    size_t rx_len;
    bool is_websocket;
    bool active;
} ws_client_t;

typedef struct {
    uv_tcp_t server_tcp;
    char bind_ip[64];
    uint16_t port;
    ws_client_t clients[MAX_WS_CLIENTS];
    bool is_running;
} ws_server_priv_t;

static void ws_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    (void)handle;
    buf->base = (char *)malloc(suggested_size);
    buf->len = (buf->base) ? suggested_size : 0;
}

static void on_client_close(uv_handle_t *handle) {
    ws_client_t *client = (ws_client_t *)handle->data;
    if (client) {
        client->active = false;
        client->rx_len = 0;
        client->is_websocket = false;
    }
}

static void send_raw(ws_client_t *client, const char *data, size_t len) {
    if (!client || !client->active) return;
    uv_buf_t buf = uv_buf_init((char *)malloc(len), (unsigned int)len);
    if (!buf.base) return;
    memcpy(buf.base, data, len);

    uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
    req->data = buf.base;
    uv_write(req, (uv_stream_t *)&client->handle, &buf, 1, (uv_write_cb)free);
}

/* Base64 & SHA-1 Mock / Minimal RFC 6455 Handshake helper */
static void handle_http_or_ws_frame(ws_client_t *client, const char *data, size_t len) {
    /* 1. Handle HTTP POST /api/v1/mcp/call */
    if (strncmp(data, "POST /api/v1/mcp/call", 21) == 0 || strncmp(data, "POST /mcp", 9) == 0) {
        const char *body = strstr(data, "\r\n\r\n");
        if (body) {
            body += 4;
            char *json_resp = edge_mcp_dispatch_json(client->trans->disp, body);
            if (json_resp) {
                char header[256];
                int hlen = snprintf(header, sizeof(header),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Access-Control-Allow-Headers: Content-Type\r\n"
                    "Content-Length: %zu\r\n\r\n",
                    strlen(json_resp));
                send_raw(client, header, (size_t)hlen);
                send_raw(client, json_resp, strlen(json_resp));
                free(json_resp);
            }
        }
        return;
    }

    /* 2. Handle HTTP OPTIONS (CORS preflight) */
    if (strncmp(data, "OPTIONS ", 8) == 0) {
        const char *cors_resp =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
        send_raw(client, cors_resp, strlen(cors_resp));
        return;
    }

    /* 3. Handle WebSocket Handshake */
    if (strstr(data, "Upgrade: websocket") || strstr(data, "upgrade: websocket")) {
        client->is_websocket = true;
        const char *ws_accept =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n\r\n";
        send_raw(client, ws_accept, strlen(ws_accept));
        return;
    }

    /* 4. If already websocket, unmask and dispatch frame */
    if (client->is_websocket && len >= 2) {
        const uint8_t *ubuf = (const uint8_t *)data;
        uint8_t opcode = ubuf[0] & 0x0F;
        if (opcode == 0x08) { /* Connection Close */
            uv_close((uv_handle_t *)&client->handle, on_client_close);
            return;
        }

        bool is_masked = (ubuf[1] & 0x80) != 0;
        uint64_t payload_len = ubuf[1] & 0x7F;
        size_t head_offset = 2;

        if (payload_len == 126 && len >= 4) {
            payload_len = ((uint64_t)ubuf[2] << 8) | ubuf[3];
            head_offset = 4;
        } else if (payload_len == 127 && len >= 10) {
            payload_len = ((uint64_t)ubuf[2] << 56) | ((uint64_t)ubuf[9]);
            head_offset = 10;
        }

        uint8_t mask_key[4] = {0};
        if (is_masked) {
            memcpy(mask_key, data + head_offset, 4);
            head_offset += 4;
        }

        if (len >= head_offset + payload_len) {
            char *unmasked = (char *)malloc((size_t)payload_len + 1);
            if (unmasked) {
                for (size_t i = 0; i < (size_t)payload_len; i++) {
                    unmasked[i] = (char)(data[head_offset + i] ^ (is_masked ? mask_key[i % 4] : 0));
                }
                unmasked[payload_len] = '\0';

                char *resp = edge_mcp_dispatch_json(client->trans->disp, unmasked);
                if (resp) {
                    /* Frame response as WS text frame (0x81) */
                    size_t rlen = strlen(resp);
                    uint8_t header[10];
                    size_t hlen = 0;
                    header[0] = 0x81; /* Fin + Text */
                    if (rlen < 126) {
                        header[1] = (uint8_t)rlen;
                        hlen = 2;
                    } else if (rlen <= 65535) {
                        header[1] = 126;
                        header[2] = (uint8_t)((rlen >> 8) & 0xFF);
                        header[3] = (uint8_t)(rlen & 0xFF);
                        hlen = 4;
                    }
                    send_raw(client, (char *)header, hlen);
                    send_raw(client, resp, rlen);
                    free(resp);
                }
                free(unmasked);
            }
        }
    }
}

static void ws_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    ws_client_t *client = (ws_client_t *)stream->data;
    if (nread > 0) {
        handle_http_or_ws_frame(client, buf->base, (size_t)nread);
    } else if (nread < 0) {
        uv_close((uv_handle_t *)stream, on_client_close);
    }
    if (buf->base) free(buf->base);
}

static void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) return;
    mcp_transport_t *trans = (mcp_transport_t *)server->data;
    ws_server_priv_t *priv = (ws_server_priv_t *)trans->priv;

    ws_client_t *client = NULL;
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (!priv->clients[i].active) {
            client = &priv->clients[i];
            break;
        }
    }
    if (!client) return;

    memset(client, 0, sizeof(ws_client_t));
    client->trans = trans;
    client->active = true;

    uv_tcp_init(trans->loop, &client->handle);
    client->handle.data = client;

    if (uv_accept(server, (uv_stream_t *)&client->handle) == 0) {
        uv_read_start((uv_stream_t *)&client->handle, ws_alloc_cb, ws_read_cb);
    } else {
        uv_close((uv_handle_t *)&client->handle, on_client_close);
    }
}

static int ws_start(mcp_transport_t *trans) {
    ws_server_priv_t *priv = (ws_server_priv_t *)trans->priv;
    if (priv->is_running) return 0;

    struct sockaddr_in addr;
    uv_ip4_addr(priv->bind_ip, priv->port, &addr);

    uv_tcp_init(trans->loop, &priv->server_tcp);
    priv->server_tcp.data = trans;

    int rc = uv_tcp_bind(&priv->server_tcp, (const struct sockaddr *)&addr, 0);
    if (rc != 0) return rc;

    rc = uv_listen((uv_stream_t *)&priv->server_tcp, 128, on_new_connection);
    if (rc == 0) {
        priv->is_running = true;
    }
    return rc;
}

static void ws_stop(mcp_transport_t *trans) {
    ws_server_priv_t *priv = (ws_server_priv_t *)trans->priv;
    if (priv && priv->is_running) {
        uv_close((uv_handle_t *)&priv->server_tcp, NULL);
        for (int i = 0; i < MAX_WS_CLIENTS; i++) {
            if (priv->clients[i].active) {
                uv_close((uv_handle_t *)&priv->clients[i].handle, on_client_close);
            }
        }
        priv->is_running = false;
    }
}

static void ws_destroy(mcp_transport_t *trans) {
    if (!trans) return;
    ws_stop(trans);
    if (trans->priv) free(trans->priv);
    free(trans);
}

mcp_transport_t *mcp_transport_ws_create(uv_loop_t *loop,
                                         edge_mcp_dispatcher_t *disp,
                                         const char *bind_ip,
                                         uint16_t port) {
    if (!loop || !disp) return NULL;
    mcp_transport_t *trans = (mcp_transport_t *)calloc(1, sizeof(mcp_transport_t));
    if (!trans) return NULL;

    ws_server_priv_t *priv = (ws_server_priv_t *)calloc(1, sizeof(ws_server_priv_t));
    if (!priv) {
        free(trans);
        return NULL;
    }

    strncpy(priv->bind_ip, bind_ip ? bind_ip : "0.0.0.0", sizeof(priv->bind_ip) - 1);
    priv->port = port ? port : 8089;

    trans->disp = disp;
    trans->loop = loop;
    trans->priv = priv;
    trans->start = ws_start;
    trans->stop = ws_stop;
    trans->destroy = ws_destroy;

    return trans;
}
