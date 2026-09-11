/**
 * @file mcp_stdio.c
 * @brief Industrial MCP stdio transport for Claude Desktop & local CLI agents
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "edgeos/mcp_transport.h"
#include "edgeos/edge_mcp_dispatcher.h"

#define STDIO_BUF_SIZE 16384

typedef struct {
    uv_pipe_t stdin_pipe;
    char line_buf[STDIO_BUF_SIZE];
    size_t line_pos;
    bool is_running;
} stdio_priv_t;

static void stdio_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    (void)handle;
    buf->base = (char *)malloc(suggested_size);
    buf->len = (buf->base) ? suggested_size : 0;
}

static void process_line(mcp_transport_t *trans, const char *line) {
    if (!line || strlen(line) == 0) return;
    char *resp = edge_mcp_dispatch_json(trans->disp, line);
    if (resp) {
        /* In stdio transport, write response line directly to stdout followed by \n */
        fprintf(stdout, "%s\n", resp);
        fflush(stdout);
        free(resp);
    }
}

static void stdio_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    mcp_transport_t *trans = (mcp_transport_t *)stream->data;
    stdio_priv_t *priv = (stdio_priv_t *)trans->priv;

    if (nread > 0) {
        for (ssize_t i = 0; i < nread; i++) {
            char c = buf->base[i];
            if (c == '\n' || c == '\r') {
                if (priv->line_pos > 0) {
                    priv->line_buf[priv->line_pos] = '\0';
                    process_line(trans, priv->line_buf);
                    priv->line_pos = 0;
                }
            } else {
                if (priv->line_pos < sizeof(priv->line_buf) - 1) {
                    priv->line_buf[priv->line_pos++] = c;
                } else {
                    /* Buffer overflow protection */
                    priv->line_pos = 0;
                }
            }
        }
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "MCP stdio read error: %s\n", uv_strerror((int)nread));
        }
        uv_read_stop(stream);
    }

    if (buf->base) {
        free(buf->base);
    }
}

static int stdio_start(mcp_transport_t *trans) {
    stdio_priv_t *priv = (stdio_priv_t *)trans->priv;
    if (priv->is_running) return 0;

    uv_pipe_init(trans->loop, &priv->stdin_pipe, 0);
    priv->stdin_pipe.data = trans;
    uv_pipe_open(&priv->stdin_pipe, 0); /* 0 = STDIN_FILENO */

    int rc = uv_read_start((uv_stream_t *)&priv->stdin_pipe, stdio_alloc_cb, stdio_read_cb);
    if (rc == 0) {
        priv->is_running = true;
    }
    return rc;
}

static void stdio_stop(mcp_transport_t *trans) {
    stdio_priv_t *priv = (stdio_priv_t *)trans->priv;
    if (priv && priv->is_running) {
        uv_read_stop((uv_stream_t *)&priv->stdin_pipe);
        priv->is_running = false;
    }
}

static void stdio_destroy(mcp_transport_t *trans) {
    if (!trans) return;
    stdio_stop(trans);
    if (trans->priv) free(trans->priv);
    free(trans);
}

mcp_transport_t *mcp_transport_stdio_create(uv_loop_t *loop, edge_mcp_dispatcher_t *disp) {
    if (!loop || !disp) return NULL;
    mcp_transport_t *trans = (mcp_transport_t *)calloc(1, sizeof(mcp_transport_t));
    if (!trans) return NULL;

    stdio_priv_t *priv = (stdio_priv_t *)calloc(1, sizeof(stdio_priv_t));
    if (!priv) {
        free(trans);
        return NULL;
    }

    trans->disp = disp;
    trans->loop = loop;
    trans->priv = priv;
    trans->start = stdio_start;
    trans->stop = stdio_stop;
    trans->destroy = stdio_destroy;

    return trans;
}
