#include <errno.h>
#include <locale.h>
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

extern const edge_tag_meta_t g_tag_metadata[];
extern const uint32_t g_tag_metadata_len;
extern int edge_tag_get_compact_index(uint32_t tag_id);

static int fetch_fds(const char *path, int *rfd, int *sfd) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    printf(" [UI] Connected to discovery socket. Receiving FDs...\n");
    *rfd = edge_fd_recv(sock);
    *sfd = edge_fd_recv(sock);
    printf(" [UI] Recv FDs -> Ring: %d, Shadow: %d\n", *rfd, *sfd);

    close(sock);
    return (*rfd >= 0) ? 0 : -1;
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    const char *bus_name = "iems";
    char sock_path[128];
    snprintf(sock_path, sizeof(sock_path), "/tmp/%s_ui.sock", bus_name);

    printf("\n--- IEMS UI DASHBOARD STARTUP ---\n");
    printf(" [1/3] Searching for DataHub [%s] at %s\n", bus_name, sock_path);

    int rfd = -1, sfd = -1;
    int retry = 0;
    while (rfd < 0 && retry < 20) {
        if (fetch_fds(sock_path, &rfd, &sfd) != 0) {
            printf("  ... still waiting for engine (try %d)\n", ++retry);
            usleep(500000);
            continue;
        }
        break;
    }

    if (rfd < 0) {
        fprintf(stderr, " ❌ FATAL: Discovery failed after 10s. Is the engine alive?\n");
        return -1;
    }

    printf(" [2/3] Initializing Local DataHub Client...\n");
    uv_loop_t *loop = uv_default_loop();
    datahub_t *hub = calloc(1, sizeof(datahub_t));

    if (datahub_init_from_fd(hub, loop, rfd, 1024 * 64, sfd,
                             g_tag_metadata_len * sizeof(pipe_msg_t), g_tag_metadata_len,
                             g_tag_metadata, g_tag_metadata_len, NULL, edge_tag_get_compact_index,
                             NULL) != 0) {
        fprintf(stderr, " ❌ FATAL: DataHub init failed.\n");
        return -1;
    }

    printf(" [3/3] Launching TUI Framework...\n");
    ui_service_t *ui = ui_service_create(hub);
    if (ui) {
        ui_service_run(ui);
        ui_service_destroy(ui);
    }

    datahub_destroy(hub);
    free(hub);
    return 0;
}