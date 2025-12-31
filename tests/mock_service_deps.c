#include <stdlib.h>

#include "arbiter.h"
#include "sqlite_service.h"

// Mock Arbiter
int arbiter_init(arbiter_t **arb, uv_loop_t *loop) {
    (void)loop;
    *arb = malloc(1);
    return 0;
}
void arbiter_shutdown(arbiter_t *arb) {
    free(arb);
}
int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    (void)arb;
    (void)bus_id;
    (void)cmd;
    return 0;
}

// Mock SQLite
sqlite_svc_t *sqlite_svc_create(uv_loop_t *loop, sqlite_config_t *cfg) {
    (void)loop;
    (void)cfg;
    return malloc(1);
}
void sqlite_svc_destroy(sqlite_svc_t *svc) {
    free(svc);
}
