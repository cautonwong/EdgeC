#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "edge_log.h"
#include "driver_manager.h"
#include "edge_api.h"
#include "iems_driver_adapter.h"
#include "iems_southbound_adapter.h"
#include "io_executor.h"
#include "io_executor_linux.h"

// 摘要行里「分母 = 连上数 + 缺失数」必须恒成立,否则验收证据对不上;
// 所以每一条没连上的 SOUTHBOUND 条目都必须走这里登记
static void _note_missing(char *buf, size_t cap, int *count, int *listed, int bus_idx,
                          const char *label) {
    (*count)++;
    size_t used = strlen(buf);
    if (used + 1 >= cap) return;
    int n = snprintf(buf + used, cap - used, "%sbus %d %s", used ? ", " : "", bus_idx, label);
    if (n > 0 && used + (size_t)n < cap) {
        (*listed)++;
    } else {
        buf[used] = '\0';  // 这一条放不下就回滚,末尾统一补 "..."
    }
}

iems_bus_transport_t iems_southbound_transport_for(const char *type, const char *path) {
    if (!path) return IEMS_BUS_UDS;
    // 路径优先:静态连通表没有 TCP 类型,环境变量覆盖设备路径时 tcp:// 是比声明类型更强的信号
    if (strncmp(path, "tcp://", 6) == 0) return IEMS_BUS_TCP;
    if (!type) return IEMS_BUS_UDS;
    if (strcasecmp(type, "UART") == 0 && strncmp(path, "/dev/", 5) == 0) {
        return IEMS_BUS_UART;
    }
    if (strcasecmp(type, "CAN") == 0) {
        size_t len = strlen(path);
        int is_sock = len >= 5 && strcmp(path + len - 5, ".sock") == 0;
        if (!is_sock && strncmp(path, "/tmp/", 5) != 0) return IEMS_BUS_UNSUPPORTED;
    }
    return IEMS_BUS_UDS;
}

int iems_southbound_connect(uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch,
                            bus_conn_state_t *bus_state, int *configured) {
    if (configured) *configured = 0;
    if (!loop || !hub || !orch || !bus_state) return -1;

    uint32_t conn_count = 0;
    const edge_connectivity_t *conns = iems_get_connectivity(&conn_count);
    if (!conns) return -1;  // 契约表都拿不到 = 装配坏了,交给组合根 abort

    int connected = 0;
    int declared = 0;
    int missing_count = 0;
    int missing_listed = 0;
    char missing[512];
    missing[0] = '\0';
    for (uint32_t i = 0; i < conn_count; i++) {
        const edge_connectivity_t *c = &conns[i];
        if (strcmp(c->category, "SOUTHBOUND") != 0) continue;
        if (configured) (*configured)++;
        declared++;
        const char *label = c->label ? c->label : "?";
        if (c->id < 0 || c->id >= IEMS_MAX_VBUS) {
            LOG_WARN(LOG_CAT_CORE, "Startup: SOUTHBOUND %s has out-of-range id %d, ignored", label,
                     c->id);
            _note_missing(missing, sizeof(missing), &missing_count, &missing_listed, (int)c->id,
                          label);
            continue;
        }

        int bus_idx = (int)c->id;
        if (bus_state[bus_idx] == BUS_STATE_CONNECTED) {
            connected++;
            continue;
        }
        if (bus_state[bus_idx] != BUS_STATE_DISCONNECTED) {
            _note_missing(missing, sizeof(missing), &missing_count, &missing_listed, bus_idx,
                          label);
            continue;
        }

        const char *env_path = NULL;
        {
            char env_key[32];
            snprintf(env_key, sizeof(env_key), "OMNIS_BUS_%d_DEV", bus_idx);
            env_path = getenv(env_key);
        }
        const char *path = env_path ? env_path : edge_resolve_bus_path(c->id);

        iems_bus_transport_t transport = iems_southbound_transport_for(c->type, path);
        if (transport == IEMS_BUS_UNSUPPORTED) {
            LOG_WARN(LOG_CAT_CORE,
                     "Startup: southbound bus %d (%s) transport %s not implemented, not connected",
                     bus_idx, label, c->type ? c->type : "?");
            _note_missing(missing, sizeof(missing), &missing_count, &missing_listed, bus_idx,
                          label);
            continue;
        }
        if (transport == IEMS_BUS_TCP) {
            char host[64];
            unsigned port = 0;
            if (sscanf(path + 6, "%63[^:]:%u", host, &port) != 2 || port == 0 || port > 65535) {
                LOG_WARN(LOG_CAT_CORE, "Startup: southbound bus %d (%s) has invalid TCP address %s",
                         bus_idx, label, path);
            } else {
                io_handle_t *h = io_executor_create_tcp(loop, host, (uint16_t)port,
                                                        datahub_get_io_pool(hub));
                if (h) {
                    bus_orchestrator_attach_io(orch, c->id, h);
                    io_handle_enable_read(h, true);
                    bus_state[bus_idx] = BUS_STATE_CONNECTED;
                    connected++;
                    printf("\033[32m[PHYSICS-SUCCESS] Bus %d CONNECTED via TCP: %s:%u\033[0m\n",
                           bus_idx, host, port);
                }
            }
        } else if (transport == IEMS_BUS_UART) {
            // 过渡配置:真实设备的波特率尚未进契约表,先按总线用环境变量覆盖
            uint32_t baud = 9600;
            char baud_key[32];
            snprintf(baud_key, sizeof(baud_key), "OMNIS_BUS_%d_BAUD", bus_idx);
            const char *baud_env = getenv(baud_key);
            if (baud_env) {
                unsigned long parsed = strtoul(baud_env, NULL, 10);
                if (parsed != 0) baud = (uint32_t)parsed;
            }
            io_handle_t *h = io_executor_create_uart(loop, path, baud, datahub_get_io_pool(hub));
            if (h) {
                bus_orchestrator_attach_io(orch, c->id, h);
                io_handle_enable_read(h, true);
                bus_state[bus_idx] = BUS_STATE_CONNECTED;
                connected++;
                printf("\033[32m[PHYSICS-SUCCESS] Bus %d CONNECTED via UART: %s\033[0m\n", bus_idx,
                       path);
            }
        } else {
            const char *search_paths[3];
            search_paths[0] = env_path;
            search_paths[1] = path;
            char fallback_path[64];
            snprintf(fallback_path, sizeof(fallback_path), "/tmp/vbus%d.sock", bus_idx);
            search_paths[2] = fallback_path;

            for (int p_idx = 0; p_idx < 3; p_idx++) {
                const char *cur_path = search_paths[p_idx];
                if (!cur_path || cur_path[0] == '\0') continue;

                int fd = socket(AF_UNIX, SOCK_STREAM, 0);
                if (fd < 0) continue;

                struct sockaddr_un addr;
                memset(&addr, 0, sizeof(addr));
                addr.sun_family = AF_UNIX;
                strncpy(addr.sun_path, cur_path, sizeof(addr.sun_path) - 1);

                if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                    io_handle_t *h = io_executor_create_from_fd(loop, fd, datahub_get_io_pool(hub));
                    if (h) {
                        bus_orchestrator_attach_io(orch, c->id, h);
                        io_handle_enable_read(h, true);
                        bus_state[bus_idx] = BUS_STATE_CONNECTED;
                        connected++;
                        printf("\033[32m[PHYSICS-SUCCESS] Bus %d CONNECTED via PATH: %s\033[0m\n",
                               bus_idx, cur_path);
                        break;
                    } else {
                        close(fd);
                    }
                } else {
                    close(fd);
                }
            }
        }

        if (bus_state[bus_idx] != BUS_STATE_CONNECTED) {
            LOG_WARN(LOG_CAT_CORE, "Startup: southbound bus %d (%s) NOT connected, no retry",
                     bus_idx, label);
            _note_missing(missing, sizeof(missing), &missing_count, &missing_listed, bus_idx,
                          label);
        }
    }
    if (missing_count > missing_listed && strlen(missing) + 6 < sizeof(missing)) {
        strcat(missing, missing_listed ? ", ..." : "...");
    }
    // 运维要看的是「缺哪路」,不是「有几路」;格式固定便于 grep 与验收证据比对
    if (missing_count > 0) {
        LOG_INFO(LOG_CAT_CORE, "[IEMS-INIT] southbound: %d/%d connected (%s missing)", connected,
                 declared, missing);
    } else {
        LOG_INFO(LOG_CAT_CORE, "[IEMS-INIT] southbound: %d/%d connected", connected, declared);
    }
    return connected;
}
