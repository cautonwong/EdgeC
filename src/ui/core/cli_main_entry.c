#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "datahub.h"
#include "edge_log.h"
#include "edge_ui.h"

// 注入编译器生成的契约元数据和路由算法
extern const edge_tag_meta_t g_tag_metadata[];
extern const uint32_t g_tag_metadata_len;
extern int edge_tag_get_compact_index(uint32_t tag_id);

static int fetch_fds_from_server(const char *path, int *ring_fd, int *shadow_fd) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    *ring_fd = edge_fd_recv(sock);
    *shadow_fd = edge_fd_recv(sock);

    close(sock);
    return (*ring_fd >= 0) ? 0 : -1;
}

int main(int argc, char *argv[]) {
    const char *bus_name = "iems";
    if (argc > 1) bus_name = argv[1];

    char sock_path[128];
    snprintf(sock_path, sizeof(sock_path), "/tmp/%s_ui.sock", bus_name);

    int ring_fd = -1;
    int shadow_fd = -1;

    if (fetch_fds_from_server(sock_path, &ring_fd, &shadow_fd) != 0) {
        fprintf(stderr, "FATAL: Could not connect to DataHub server at %s\n", sock_path);
        return -1;
    }

    uv_loop_t *loop = uv_default_loop();
    datahub_t *hub = calloc(1, sizeof(datahub_t));

    // 关键修正：注入编译器生成的 map_fn (edge_tag_get_compact_index)
    // 这样 datahub_read_shadow 才能正确找到紧凑排列的索引
    datahub_init_from_fd(hub, loop, ring_fd, 1024 * 64, shadow_fd,
                         g_tag_metadata_len * sizeof(pipe_msg_t), g_tag_metadata_len,
                         g_tag_metadata, g_tag_metadata_len, NULL, edge_tag_get_compact_index,
                         NULL);

    printf("UI Dashboard Connected. Shadow Capacity: %u tags\n", hub->shadow_capacity);

    ui_service_t *ui = ui_service_create(hub);
    if (ui) {
        ui_service_run(ui);
        ui_service_destroy(ui);
    }

    datahub_destroy(hub);
    free(hub);

    return 0;
}
