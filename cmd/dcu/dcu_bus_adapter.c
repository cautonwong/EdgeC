#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "dcu_bus_adapter.h"
#include "edge_log.h"
#include "io_executor.h"

void dcu_bus_attach_channels(uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch,
                             driver_manager_t *drv_mgr, dcu_inventory_t *inv) {
    if (!loop || !hub || !orch || !drv_mgr || !inv) return;

    for (int i = 0; i < 32; i++) {
        // 协议来自档案，不按通道号猜；空通道不建主站、不连 socket。
        const char *proto = dcu_inventory_channel_protocol(inv, (uint8_t)i);
        if (!proto) continue;
        /* [ADR-002] 驱动不再需要点位表注入；这里只确认协议确实注册过 */
        if (!driver_manager_find(drv_mgr, proto)) {
            LOG_ERROR(LOG_CAT_SCHED, "❌ channel %d: protocol '%s' has no registered driver", i,
                      proto);
            continue;
        }
        bus_orchestrator_setup_master(orch, i, proto, 0);
        char path[64];
        snprintf(path, sizeof(path), "/tmp/rs485_%d.sock", i);
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) >= 0) {
            int flags = fcntl(fd, F_GETFL, 0);
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            io_handle_t *h = io_executor_create_from_fd(loop, fd, datahub_get_io_pool(hub));
            bus_orchestrator_attach_io(orch, i, h);
        } else {
            close(fd);
        }
    }
}
