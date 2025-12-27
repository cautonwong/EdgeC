#include "edge_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static service_t *g_svc_ptr = NULL;

static void on_signal(uv_signal_t *handle, int signum) {
    (void)signum;
    printf("[Service] Signal received, stopping...\n");
    service_stop((service_t *)handle->data);
}

int service_start(service_t *svc) {
    if (!svc || !svc->loop) return -1;
    g_svc_ptr = svc;

    printf("[Service] Container starting in mode: %s\n", svc->opts.mode);

    // 注册系统信号处理 (使用 libuv)
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

    // 具体的组件销毁由 App 负责，或者容器协助关闭 UV handles
    uv_stop(svc->loop);
}

