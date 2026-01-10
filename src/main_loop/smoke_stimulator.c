#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "edge_log.h"

typedef struct {
    uv_thread_t thread;
    datahub_t *hub;
    int running;
} smoke_stimulator_t;

static void _stimulator_thread(void *arg) {
    smoke_stimulator_t *s = arg;
    LOG_INFO(LOG_CAT_CORE, "[STIMULATOR] Direct DataHub injection started.");

    // 模拟 5 次正常电压，1 次过压
    for (int i = 0; i < 6; i++) {
        pipe_msg_t msg;
        double val = (i == 5) ? 275.0 : 230.0;
        pipe_msg_init_scalar(&msg, 1001, val, (uint64_t)time(NULL) * 1000);
        datahub_publish(s->hub, &msg);

        LOG_INFO(LOG_CAT_CORE, "[STIMULATOR] Injected Tag 1001 val %.1f", val);
        usleep(500 * 1000);
    }
    LOG_INFO(LOG_CAT_CORE, "[STIMULATOR] Injection sequence complete.");
    free(s);
}

void smoke_stimulator_start(datahub_t *hub) {
    smoke_stimulator_t *s = calloc(1, sizeof(smoke_stimulator_t));
    s->hub = hub;
    s->running = 1;
    uv_thread_create(&s->thread, _stimulator_thread, s);
}
