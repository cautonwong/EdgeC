#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "edge_log.h"
#include "smartbox_uds_ipc.h"

#define UDS_PATH "/tmp/smart_box_ipc.sock"

void smartbox_uds_send(uint8_t msg_type, const uint8_t *payload, uint8_t len) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_ERROR(LOG_CAT_CORE, "UDS IPC: Failed to create socket");
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UDS_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_WARN(LOG_CAT_CORE, "UDS IPC: Go Gateway not listening on %s", UDS_PATH);
        close(fd);
        return;
    }

    uint8_t frame_buf[260];
    frame_buf[0] = 0x53;  // Magic 'S'
    frame_buf[1] = 0x42;  // Magic 'B'
    frame_buf[2] = msg_type;
    frame_buf[3] = len;

    if (len > 0 && payload != NULL) {
        memcpy(&frame_buf[4], payload, len);
    }

    uint8_t checksum = 0;
    for (int i = 0; i < 4 + len; i++) {
        checksum ^= frame_buf[i];
    }
    frame_buf[4 + len] = checksum;

    if (write(fd, frame_buf, (size_t)(5 + len)) < 0) {
        LOG_ERROR(LOG_CAT_CORE, "UDS IPC: Failed to write to socket");
    }
    close(fd);
    LOG_INFO(LOG_CAT_CORE, "UDS IPC: Sent frame to Go Gateway. MsgType=%d, Len=%d", msg_type, len);
}
