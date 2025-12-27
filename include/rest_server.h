#ifndef REST_SERVER_H
#define REST_SERVER_H

#include <uv.h>
#include "shmring.h"

// Opaque handle
typedef struct rest_server_s rest_server_t;

/**
 * @brief Init the REST API Server (Data/Control).
 * 
 * @param loop The loop to run on (provided by Northbound Mgr).
 * @param ring The data source.
 * @param port The port to listen on (e.g., 8081).
 */
rest_server_t* rest_server_create(uv_loop_t *loop, shm_ring_t *ring, int port);

void rest_server_destroy(rest_server_t *srv);

#endif