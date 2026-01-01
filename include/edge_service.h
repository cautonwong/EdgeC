#ifndef EDGE_SERVICE_H
#define EDGE_SERVICE_H

#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "edge_params.h"
#include "net_buf.h"
#include "northbound_mgr.h"
#include "persistence_mgr.h"
#include "profile_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct service_s service_t;

typedef struct {
    char config_path[256];
    char mode[32];
    int web_port;
    int rest_port;
} service_options_t;

service_t *service_new(service_options_t *opts);

void service_set_loop(service_t *svc, uv_loop_t *loop);
void service_set_datahub(service_t *svc, datahub_t *hub);
void service_set_storage(service_t *svc, sqlite_svc_t *storage);
uv_loop_t *service_get_loop(service_t *svc);

shm_ring_t *service_get_ring(service_t *svc);
shm_ring_t *service_get_shmring(service_t *svc);

arbiter_t *service_get_arbiter(service_t *svc);
datahub_t *service_get_datahub(service_t *svc);
sqlite_svc_t *service_get_storage(service_t *svc);
edge_params_t *service_get_params(service_t *svc);  // [新增]

typedef struct device_profile_s device_profile_t;
device_profile_t *service_get_devices(service_t *svc);

void service_set_contract_path(service_t *svc, const char *path);
const char *service_get_contract_path(service_t *svc);

typedef struct stream_engine_s stream_engine_t;
void service_set_stream_engine(service_t *svc, stream_engine_t *engine);
stream_engine_t *service_get_stream_engine(service_t *svc);

int service_run(service_t *svc, uv_loop_t *loop, net_buf_pool_t *pool, arbiter_t *arbiter,
                datahub_t *hub, persistence_mgr_t *storage, northbound_mgr_t *cloud);

int service_post_cmd(service_t *svc, cmd_msg_t *cmd);
void service_delete(service_t *svc);

#ifdef __cplusplus
}
#endif

#endif