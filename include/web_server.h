#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <uv.h>

// Opaque handle
typedef struct web_server_s web_server_t;

/**
 * @brief Init the Static Web Server (HMI).
 * 
 * @param loop The loop to run on (provided by Northbound Mgr).
 * @param port The port to listen on (e.g., 8080).
 */
web_server_t* web_server_create(uv_loop_t *loop, int port);

void web_server_destroy(web_server_t *srv);

#endif