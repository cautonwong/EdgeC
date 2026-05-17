#include "wasm_mgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "edge_log.h"
#include "wasm_export.h"

static datahub_t *g_hub = NULL;
static bool g_wasm_inited = false;

// --- Host Exports (ABI) ---
static int wasm_host_read(wasm_exec_env_t exec_env, uint32_t tag_id, uint32_t out_ptr_wasm) {
    if (!g_hub) return -1;
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    double *host_ptr = (double *)wasm_runtime_addr_app_to_native(inst, out_ptr_wasm);
    if (!host_ptr) return -1;
    pipe_msg_t msg;
    if (datahub_read_shadow(g_hub, tag_id, &msg) == 0) {
        if (msg.type == PIPEMSG_TYPE_SCALAR) {
            *host_ptr = msg.payload.val.f;
            return 0;
        }
    }
    return -1;
}

static int wasm_host_publish(wasm_exec_env_t exec_env, uint32_t tag_id, double value) {
    (void)exec_env;
    if (!g_hub) return -1;
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, tag_id, value, uv_hrtime());
    return datahub_publish(g_hub, &msg);
}

static void wasm_host_log(wasm_exec_env_t exec_env, uint32_t level, uint32_t msg_ptr_wasm) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    const char *host_msg = (const char *)wasm_runtime_addr_app_to_native(inst, msg_ptr_wasm);
    if (!host_msg) return;
    if (level == 0) LOG_INFO(LOG_CAT_CORE, "[WASM] %s", host_msg);
    else if (level == 1) LOG_INFO(LOG_CAT_CORE, "[WASM] %s", host_msg);
    else if (level == 2) LOG_WARN(LOG_CAT_CORE, "[WASM] %s", host_msg);
    else LOG_ERROR(LOG_CAT_CORE, "[WASM] %s", host_msg);
}

static NativeSymbol native_symbols[] = {
    {"ext_datahub_read", wasm_host_read, "(ii)i", NULL},
    {"ext_datahub_publish", wasm_host_publish, "(id)i", NULL},
    {"ext_log", wasm_host_log, "(ii)v", NULL}
};

struct wasm_app_s {
    wasm_module_t module;
    wasm_module_inst_t inst;
    uint8_t *wasm_buf;
    uint32_t timeout_ms;
    uint32_t stack_size;
    uint32_t heap_size;
};

static char *read_file_to_buffer(const char *path, uint32_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *size = (uint32_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(*size);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, *size, f) != *size) { free(buf); fclose(f); return NULL; }
    fclose(f);
    return buf;
}

int wasm_mgr_init(datahub_t *hub) {
    if (g_wasm_inited) {
        g_hub = hub;
        return 0;
    }
    g_hub = hub;
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
    init_args.native_module_name = "env";
    init_args.native_symbols = native_symbols;
    init_args.n_native_symbols = 3;
    if (!wasm_runtime_full_init(&init_args)) return -1;
    g_wasm_inited = true;
    LOG_INFO(LOG_CAT_CORE, "WASM: Secure Runtime initialized");
    return 0;
}

void wasm_mgr_cleanup(void) {
    if (g_wasm_inited) {
        wasm_runtime_destroy();
        g_wasm_inited = false;
    }
}

wasm_app_t *wasm_app_create(const char *wasm_path, uint32_t heap_size, uint32_t stack_size, uint32_t timeout_ms) {
    uint32_t wasm_size;
    uint8_t *buf = (uint8_t *)read_file_to_buffer(wasm_path, &wasm_size);
    if (!buf) return NULL;

    wasm_app_t *app = calloc(1, sizeof(wasm_app_t));
    app->wasm_buf = buf;
    app->heap_size = heap_size > 0 ? heap_size : 64 * 1024;
    app->stack_size = stack_size > 0 ? stack_size : 64 * 1024;
    app->timeout_ms = timeout_ms;

    char error_buf[128];
    app->module = wasm_runtime_load(buf, wasm_size, error_buf, sizeof(error_buf));
    if (!app->module) {
        LOG_ERROR(LOG_CAT_CORE, "WASM Load Failed: %s", error_buf);
        free(buf); free(app); return NULL;
    }

    app->inst = wasm_runtime_instantiate(app->module, app->stack_size, app->heap_size, error_buf, sizeof(error_buf));
    if (!app->inst) {
        LOG_ERROR(LOG_CAT_CORE, "WASM Instantiate Failed: %s", error_buf);
        wasm_runtime_unload(app->module); free(buf); free(app); return NULL;
    }

    return app;
}

typedef struct {
    wasm_module_inst_t inst;
    uint32_t timeout_ms;
    bool done;
    uv_mutex_t lock;
} wasm_watchdog_t;

static void _watchdog_thread(void *arg) {
    wasm_watchdog_t *wd = (wasm_watchdog_t *)arg;
    uint64_t start = uv_hrtime() / 1000000;
    
    while (true) {
        uv_mutex_lock(&wd->lock);
        if (wd->done) {
            uv_mutex_unlock(&wd->lock);
            break;
        }
        uint64_t now = uv_hrtime() / 1000000;
        if (now - start >= wd->timeout_ms) {
            LOG_ERROR(LOG_CAT_CORE, "WASM Watchdog: Timeout reached! Terminating instance %p", (void *)wd->inst);
            // [Nexus] 同时使用 set_exception 和 terminate 确保中断
            wasm_runtime_set_exception(wd->inst, "timeout-interrupted");
            wasm_runtime_terminate(wd->inst);
            uv_mutex_unlock(&wd->lock);
            break;
        }
        uv_mutex_unlock(&wd->lock);
        usleep(1000); // 1ms check interval for responsiveness
    }
}

int wasm_app_exec(wasm_app_t *app, const char *func_name, pipe_msg_t *msg) {
    (void)msg;
    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(app->inst, app->stack_size);
    if (!exec_env) return -1;

    wasm_function_inst_t func = wasm_runtime_lookup_function(app->inst, func_name);
    if (!func) { wasm_runtime_destroy_exec_env(exec_env); return -1; }

    // [Nexus] Watchdog 机制 (多线程监控)
    wasm_watchdog_t wd = {.inst = app->inst, .timeout_ms = app->timeout_ms, .done = false};
    uv_thread_t wd_thread;
    if (app->timeout_ms > 0) {
        uv_mutex_init(&wd.lock);
        uv_thread_create(&wd_thread, _watchdog_thread, &wd);
    }

    bool success = wasm_runtime_call_wasm(exec_env, func, 0, NULL);
    
    if (app->timeout_ms > 0) {
        uv_mutex_lock(&wd.lock);
        wd.done = true;
        uv_mutex_unlock(&wd.lock);
        uv_thread_join(&wd_thread);
        uv_mutex_destroy(&wd.lock);
    }

    wasm_runtime_destroy_exec_env(exec_env);
    return success ? 0 : -1;
}

void wasm_app_destroy(wasm_app_t *app) {
    if (!app) return;
    if (app->inst) wasm_runtime_deinstantiate(app->inst);
    if (app->module) wasm_runtime_unload(app->module);
    if (app->wasm_buf) free(app->wasm_buf);
    free(app);
}

// 兼容旧接口 (使用单例模拟)
static wasm_app_t *g_compat_app = NULL;
int wasm_mgr_exec(const char *path, const char *func_name, pipe_msg_t *msg) {
    if (!g_compat_app) g_compat_app = wasm_app_create(path, 0, 0, 100);
    return wasm_app_exec(g_compat_app, func_name, msg);
}
