#include "wasm_mgr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "wasm_export.h"

static datahub_t *g_hub = NULL;

// --- Host Exports (ABI) ---

/**
 * [WASM-ABI] 强制使用 WAMR 的原始注册方式以绕过签名校验不一致
 */

static bool ext_datahub_read_wrapper(wasm_exec_env_t exec_env, uint32_t tag_id, uint32_t out_ptr_wasm) {
    if (!g_hub) return false;
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    double *host_ptr = (double *)wasm_runtime_addr_app_to_native(inst, out_ptr_wasm);
    if (!host_ptr) return false;

    pipe_msg_t msg;
    if (datahub_read_shadow(g_hub, tag_id, &msg) == 0) {
        if (msg.type == PIPEMSG_TYPE_SCALAR) {
            *host_ptr = msg.payload.val.f;
            return true;
        }
    }
    return false;
}

static bool ext_datahub_publish_wrapper(wasm_exec_env_t exec_env, uint32_t tag_id, double value) {
    if (!g_hub) return false;
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, tag_id, value, uv_hrtime());
    return datahub_publish(g_hub, &msg) == 0;
}

static void ext_log_wrapper(wasm_exec_env_t exec_env, int level, uint32_t msg_ptr_wasm) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    const char *host_msg = (const char *)wasm_runtime_addr_app_to_native(inst, msg_ptr_wasm);
    if (!host_msg) return;

    if (level == 0) LOG_INFO(LOG_CAT_CORE, "[WASM] %s", host_msg);
    else if (level == 1) LOG_INFO(LOG_CAT_CORE, "[WASM] %s", host_msg);
    else if (level == 2) LOG_WARN(LOG_CAT_CORE, "[WASM] %s", host_msg);
    else LOG_ERROR(LOG_CAT_CORE, "[WASM] %s", host_msg);
}

// 采用更宽松的签名定义
static NativeSymbol native_symbols[] = {
    {"ext_datahub_read", ext_datahub_read_wrapper, "(ii)i", NULL},
    {"ext_datahub_publish", ext_datahub_publish_wrapper, "(id)i", NULL},
    {"ext_log", ext_log_wrapper, "(ii)v", NULL}
};

// --- Runtime Management ---

static char g_last_path[128] = {0};
static wasm_module_t g_module = NULL;
static wasm_module_inst_t g_inst = NULL;
static uint8_t *g_wasm_buf = NULL;

static void wasm_mgr_cleanup_module(void) {
    if (g_inst) wasm_runtime_deinstantiate(g_inst);
    if (g_module) wasm_runtime_unload(g_module);
    if (g_wasm_buf) free(g_wasm_buf);
    g_inst = NULL;
    g_module = NULL;
    g_wasm_buf = NULL;
    g_last_path[0] = '\0';
}

int wasm_mgr_init(datahub_t *hub) {
    g_hub = hub;
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_System_Allocator;

    if (!wasm_runtime_full_init(&init_args)) return -1;

    // 尝试不带命名空间的前缀注册，或者明确使用 "env"
    if (!wasm_runtime_register_natives("env", native_symbols, 3)) {
        return -1;
    }
    return 0;
}

void wasm_mgr_cleanup(void) {
    wasm_mgr_cleanup_module();
    wasm_runtime_destroy();
}

static char *read_file_to_buffer(const char *path, uint32_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *size = (uint32_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(*size);
    if (fread(buf, 1, *size, f) != *size) { free(buf); fclose(f); return NULL; }
    fclose(f);
    return buf;
}

int wasm_mgr_exec(const char *path, const char *func_name, pipe_msg_t *msg) {
    (void)msg;
    if (strcmp(g_last_path, path) != 0) {
        wasm_mgr_cleanup_module();
        uint32_t wasm_size;
        g_wasm_buf = (uint8_t*)read_file_to_buffer(path, &wasm_size);
        if (!g_wasm_buf) return -1;
        char error_buf[128];
        g_module = wasm_runtime_load(g_wasm_buf, wasm_size, error_buf, sizeof(error_buf));
        if (!g_module) { free(g_wasm_buf); g_wasm_buf = NULL; return -1; }
        g_inst = wasm_runtime_instantiate(g_module, 64 * 1024, 64 * 1024, error_buf, sizeof(error_buf));
        if (!g_inst) { wasm_runtime_unload(g_module); free(g_wasm_buf); return -1; }
        strncpy(g_last_path, path, 127);
    }

    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(g_inst, 64 * 1024);
    if (!exec_env) return -1;
    wasm_function_inst_t func = wasm_runtime_lookup_function(g_inst, func_name);
    if (!func) { wasm_runtime_destroy_exec_env(exec_env); return -1; }

    if (!wasm_runtime_call_wasm(exec_env, func, 0, NULL)) {
        wasm_runtime_destroy_exec_env(exec_env);
        return -1;
    }
    wasm_runtime_destroy_exec_env(exec_env);
    return 0;
}
