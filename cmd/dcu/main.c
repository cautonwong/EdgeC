#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_service.h"

int main(int argc, char **argv) {
    service_options_t opts = {.web_port = 8080, .rest_port = 8081};
    strncpy(opts.config_path, "config/device_profile.json", 255);
    strncpy(opts.mode, "dcu", 31);

    int opt;
    while ((opt = getopt(argc, argv, "c:m:w:r:h")) != -1) {
        switch (opt) {
            case 'c':
                strncpy(opts.config_path, optarg, 255);
                break;
            case 'm':
                strncpy(opts.mode, optarg, 31);
                break;
            case 'w':
                opts.web_port = atoi(optarg);
                break;
            case 'r':
                opts.rest_port = atoi(optarg);
                break;
            case 'h':
                printf("EdgeOS DCU\n");
                return 0;
            default:
                return 1;
        }
    }

    service_t *svc = service_new(&opts);
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);

    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 2048, 512, NULL, NULL);  // 海量缓存

    arbiter_t *arbiter;
    arbiter_init(&arbiter, loop);

    datahub_t hub;
    datahub_init(&hub, loop, "/dcu_hub", 2048, NULL, NULL);

    persistence_mgr_t storage;
    sqlite_config_t db_cfg = {
        .type = PROD_DCU, .db_path = "edge_data.db", .batch_size = 500, .flush_interval_ms = 1000};
    storage.writer = sqlite_svc_create(loop, &db_cfg);
    persistence_mgr_init(&storage, loop, &hub.ring, 1000);

    northbound_mgr_t cloud;
    northbound_mgr_init(&cloud, loop, &hub.ring);
    cloud.web_port = opts.web_port;
    cloud.rest_port = opts.rest_port;
    cloud.user_data = svc;
    northbound_mgr_start(&cloud);

    int ret = service_run(svc, loop, &pool, arbiter, &hub, &storage, &cloud);

    northbound_mgr_stop(&cloud);
    persistence_mgr_close(&storage);
    arbiter_shutdown(arbiter);
    net_buf_pool_destroy(&pool);
    service_delete(svc);
    free(loop);
    return ret;
}
