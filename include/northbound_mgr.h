#ifndef NORTHBOUND_MGR_H
#define NORTHBOUND_MGR_H

#include <uv.h>

#include "shmring.h"
#include "sqlite_service.h"

// Forward declarations

struct web_server_s;

struct rest_server_s;

typedef struct {
    uv_loop_t loop;

    uv_thread_t thread;

    // Core Links

    shm_ring_t *ring;

    sqlite_svc_t *writer;  // For control commands logging or settings

    // Sub-Servers
    struct web_server_s *web_srv;
    struct rest_server_s *rest_srv;

    uv_async_t stop_async;
    uv_async_t data_async;
    uv_timer_t poll_timer;
    uint64_t last_seq;

    int web_port;
    int rest_port;
    char web_root[256];

    void *user_data;
    int running;
} northbound_mgr_t;

/**
 * @brief Initialize the Northbound Manager.
 */
int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring);

/**
 * @brief Start the Northbound thread (Web:8080, REST:8081).
 */
int northbound_mgr_start(northbound_mgr_t *mgr);

/**
 * @brief Stop everything.
 */
void northbound_mgr_stop(northbound_mgr_t *mgr);

#endif
