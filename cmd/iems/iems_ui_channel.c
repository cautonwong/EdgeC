#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "edge_log.h"
#include "edge_slave_plane.h"
#include "iems_ui_channel.h"

static void on_ui_connect(uv_stream_t *server, int status) {
    if (status < 0) return;
    iems_ui_channel_t *ch = (iems_ui_channel_t *)server->data;
    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t *)client) == 0 && ch && ch->hub) {
        int cfd;
        uv_fileno((const uv_handle_t *)client, &cfd);
        edge_fd_send(cfd, datahub_get_ring_fd(ch->hub));
        edge_fd_send(cfd, datahub_get_shadow_fd(ch->hub));
    }
    uv_close((uv_handle_t *)client, (uv_close_cb)free);
}

void iems_ui_channel_start(iems_ui_channel_t *ch, uv_loop_t *loop, datahub_t *hub,
                           const char *path) {
    if (!ch || !loop || !path) return;

    ch->hub = hub;
    ch->srv = (uv_pipe_t *)malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, ch->srv, 0);
    ch->srv->data = ch;
    unlink(path);
    int bind_rc = uv_pipe_bind(ch->srv, path);
    int listen_rc = (bind_rc == 0) ? uv_listen((uv_stream_t *)ch->srv, 128, on_ui_connect) : 0;
    if (bind_rc == 0 && listen_rc == 0) {
        printf("\033[32m[IEMS-INIT] UI Companion Socket listening at %s\033[0m\n", path);
        return;
    }

    // 路径太长/目录不存在/被占:UI 面板会看不到数据,得让现场直接看见原因
    LOG_ERROR(LOG_CAT_CORE, "IEMS-INIT: UI Companion Socket FAILED at %s (bind=%s)", path,
              uv_strerror(bind_rc));
    if (bind_rc == 0) {
        LOG_ERROR(LOG_CAT_CORE, "IEMS-INIT: UI listen FAILED: %s", uv_strerror(listen_rc));
        unlink(path);  // bind 成功、listen 失败会留下没人听的 socket 文件
    }
    uv_close((uv_handle_t *)ch->srv, (uv_close_cb)free);
    ch->srv = NULL;
}

void iems_ui_channel_close(iems_ui_channel_t *ch) {
    if (!ch || !ch->srv) return;
    uv_close((uv_handle_t *)ch->srv, (uv_close_cb)free);
    ch->srv = NULL;
}
