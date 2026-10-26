/**
 * @file main.c
 * @brief IEMS Commercial Energy Storage Gateway Entrypoint (Composition Root)
 *
 * Conforms to ADR-0010 & Clean Architecture:
 * Entrypoint is purely an orchestration root responsible for config parsing,
 * signal registration, and running the libuv event loop.
 */

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "edge_license.h"
#include "iems_app.h"

static iems_app_t* g_app = NULL;

static void on_signal_exit(uv_signal_t* handle, int signum) {
    (void)signum;
    uv_stop(handle->loop);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // 1. SDK Tiering & License Engine
    const char* lic_key = getenv("EDGEC_LICENSE_KEY");
    if (license_engine_init(lic_key ? lic_key : "ENT-EVAL-KEY") < 0) {
        fprintf(stderr, "FATAL: License verification failed. Boot aborted.\n");
        return -1;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    uv_loop_t* loop = uv_default_loop();

    // 2. 信号处理
    static uv_signal_t s1, s2;
    uv_signal_init(loop, &s1);
    uv_signal_start(&s1, on_signal_exit, SIGINT);
    uv_signal_init(loop, &s2);
    uv_signal_start(&s2, on_signal_exit, SIGTERM);

    // 3. 配置组装与 Assembler 装配
    // socket 路径也在这里读一次,往下当参数传(深层模块不许 getenv)
    const char* north_sock = getenv("IEMS_NORTH_SOCK");
    if (north_sock && !north_sock[0]) north_sock = NULL;  // 空串当没设,别把空路径传下去
    const char* ui_sock = getenv("IEMS_UI_SOCK");
    if (ui_sock && !ui_sock[0]) ui_sock = NULL;
    iems_app_config_t cfg = {
        .db_path = getenv("EDGE_DB_PATH"),
        .shm_name = getenv("EDGE_SHM_NAME"),
        .wal_path = getenv("EDGE_WAL_PATH"),
        .snap_path = getenv("EDGE_SNAP_PATH"),
        .mqtt_url = getenv("MQTT_BROKER"),
        .rest_port = getenv("EDGE_REST_PORT") ? atoi(getenv("EDGE_REST_PORT")) : 9091,
        .north_sock = north_sock,
        .ui_sock = ui_sock
    };

    g_app = iems_app_assemble(loop, &cfg);
    if (!g_app) {
        fprintf(stderr, "FATAL: Failed to assemble IEMS application.\n");
        return -1;
    }

    // 4. 南向总线连接:组合根只发起这一次,配了南向却一台都连不上就是启动失败
    int configured = 0;
    int bus_count = iems_app_connect_buses(g_app, &configured);
    if (bus_count < 0 || (configured > 0 && bus_count == 0)) {
        if (bus_count < 0) {
            fprintf(stderr, "FATAL: bus connectivity contract unavailable. Boot aborted.\n");
        } else {
            fprintf(stderr,
                    "FATAL: %d southbound bus(es) configured, none connected. Boot aborted.\n",
                    configured);
        }
        iems_app_destroy(g_app);
        curl_global_cleanup();
        return -1;
    }

    // 5. 启动自律业务与服务
    if (iems_app_start(g_app) != 0) {
        fprintf(stderr, "FATAL: Failed to start IEMS application.\n");
        iems_app_destroy(g_app);
        return -1;
    }

    // 6. 运行事件循环
    iems_app_run(g_app);

    // 7. 安全释放与清理
    iems_app_destroy(g_app);
    curl_global_cleanup();
    return 0;
}
