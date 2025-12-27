#include "edge_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

static void on_signal(uv_signal_t *handle, int signum) {
    (void)signum;
    printf("\n[Service] Signal received, stopping...\n");
    service_stop((service_t *)handle->data);
}

// Process commands from REST/Other threads
static void on_cmd_async(uv_async_t *handle) {
    service_t *svc = (service_t *)handle->data;
    service_cmd_t cmd;

    while (true) {
        bool has_cmd = false;
        uv_mutex_lock(&svc->cmd_lock);
        if (svc->cmd_head != svc->cmd_tail) {
            cmd = svc->cmd_queue[svc->cmd_tail];
            svc->cmd_tail = (svc->cmd_tail + 1) % 16;
            has_cmd = true;
        }
        uv_mutex_unlock(&svc->cmd_lock);

        if (!has_cmd) break;

        // Dispatch Command
        switch (cmd.type) {
            case SVC_CMD_READ_ON_DEMAND:
                printf("[Service] S5: On-Demand Read for Tag %u\n", cmd.tag_id);
                // Trigger Arbiter high-prio request
                break;
            case SVC_CMD_WRITE:
                printf("[Service] S6: Control Write for Tag %u = %f\n", cmd.tag_id, cmd.value);
                break;
            case SVC_CMD_OTA_START:
                printf("[Service] S7: OTA Started (Exclusive Lock)\n");
                break;
            case SVC_CMD_OTA_END:
                printf("[Service] S7: OTA Finished (Release Lock)\n");
                break;
        }
    }
}

int service_start(service_t *svc) {
    if (!svc || !svc->loop) return -1;

    printf("[Service] Container starting in mode: %s\n", svc->opts.mode);

    // Initialize Bridge
    uv_mutex_init(&svc->cmd_lock);
    uv_async_init(svc->loop, &svc->cmd_async, on_cmd_async);
    svc->cmd_async.data = svc;
    svc->cmd_head = svc->cmd_tail = 0;

    // Signals
    static uv_signal_t sig_int, sig_term;
    uv_signal_init(svc->loop, &sig_int);
    sig_int.data = svc;
    uv_signal_start(&sig_int, on_signal, SIGINT);

    uv_signal_init(svc->loop, &sig_term);
    sig_term.data = svc;
    uv_signal_start(&sig_term, on_signal, SIGTERM);

    return uv_run(svc->loop, UV_RUN_DEFAULT);
}

void service_stop(service_t *svc) {
    if (!svc || svc->shutting_down) return;
    svc->shutting_down = 1;

    printf("[Service] Container gracefully shutting down...\n");
    
    // Cleanup Bridge
    uv_close((uv_handle_t*)&svc->cmd_async, NULL);
    uv_mutex_destroy(&svc->cmd_lock);

    uv_stop(svc->loop);
}

int service_post_cmd(service_t *svc, service_cmd_t *cmd) {
    if (!svc || !cmd) return -1;
    
    uv_mutex_lock(&svc->cmd_lock);
    int next = (svc->cmd_head + 1) % 16;
    if (next == svc->cmd_tail) {
        uv_mutex_unlock(&svc->cmd_lock);
        return -1; // Queue full
    }
    svc->cmd_queue[svc->cmd_head] = *cmd;
    svc->cmd_head = next;
    uv_mutex_unlock(&svc->cmd_lock);
    
    return uv_async_send(&svc->cmd_async);
}