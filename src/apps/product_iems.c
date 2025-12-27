#include "edge_service.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief IEMS 产品二开入口
 * 这里负责实例化所有组件
 */
int run_iems_product(service_options_t *opts) {
    service_t svc;
    memset(&svc, 0, sizeof(svc));
    svc.opts = *opts;
    svc.loop = uv_default_loop();

    printf("[App] Initializing IEMS Product Components...\n");

    // 1. 初始化内存池
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 512, 512, NULL, NULL);
    svc.pool = &pool;

    // 2. 初始化配置
    svc.config = profile_load(opts->config_path);

    // 3. 初始化仲裁器
    svc.arbiter = arbiter_create(svc.loop);

    // 4. 初始化数据中心
    datahub_t hub;
    datahub_init(&hub, svc.loop, "/iems_hub", 512, NULL, &svc);
    svc.data_hub = &hub;

    // 5. 初始化持久化 (SQLite)
    persistence_mgr_t storage;
    persistence_mgr_init(&storage, svc.loop, &hub.ring, 1000);
    svc.storage = &storage;

    // 6. 初始化北向网关
    northbound_mgr_init(&svc.cloud, svc.loop, &hub.ring);
    svc.cloud.web_port = opts->web_port;
    svc.cloud.rest_port = opts->rest_port;
    northbound_mgr_start(&svc.cloud);

    // --- 启动容器 ---
    int ret = service_start(&svc);

    // --- 清理逻辑 ---
    printf("[App] Cleaning up IEMS Product Components...\n");
    northbound_mgr_stop(&svc.cloud);
    persistence_mgr_close(&storage);
    arbiter_destroy(svc.arbiter);
    net_buf_pool_destroy(&pool);
    profile_free(svc.config);

    return ret;
}
