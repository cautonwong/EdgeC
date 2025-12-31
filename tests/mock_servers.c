#include <stdlib.h>

#include "rest_server.h"
#include "web_server.h"

// Mock Web Server
struct web_server_s {
    int dummy;
};
web_server_t *web_server_create(uv_loop_t *loop, int port) {
    (void)loop;
    (void)port;
    return malloc(sizeof(web_server_t));
}
void web_server_destroy(web_server_t *srv) {
    free(srv);
}

// Mock REST Server
struct rest_server_s {
    int dummy;
};
rest_server_t *rest_server_create(uv_loop_t *loop, service_t *svc, int port) {
    (void)loop;
    (void)svc;
    (void)port;
    return malloc(sizeof(rest_server_t));
}
void rest_server_destroy(rest_server_t *srv) {
    free(srv);
}
