#include <stdlib.h>

#include "arbiter.h"
#include "sqlite_service.h"

// Mock Arbiter
int arbiter_init(arbiter_t** arb, uv_loop_t* loop) {
    (void)loop;
    *arb = malloc(1);
    return 0;
}
void arbiter_shutdown(arbiter_t* arb) {
    free(arb);
}
int arbiter_submit_cmd(arbiter_t* arb, int bus_id, cmd_msg_t* cmd) {
    (void)arb;
    (void)bus_id;
    (void)cmd;
    return 0;
}