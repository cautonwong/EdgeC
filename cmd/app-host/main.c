/**
 * @file main.c
 * @brief app-host: runs one wasm app and serves the Host ABI over an inherited fd.
 *
 * The product process spawns this binary with a socketpair fd. The app's
 * imports (env.point_resolve, point_read, point_publish, app_log, app_now)
 * forward to the product through the SDK client on that fd.
 */

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "edge_app_sdk.h"
#include "wasm_export.h"

static edge_app_sdk_t *g_sdk;
static volatile sig_atomic_t g_stop;

static void on_signal(int sig) {
    (void)sig;
    g_stop = 1;
}

static int error_index(const char *code) {
    static const char *codes[] = {
        "denied_not_granted", "denied_scope",     "denied_expired", "denied_revoked",
        "denied_policy",      "denied_quota",     "denied_package_mismatch",
        "denied_abi",         "invalid_manifest", "not_found",      "stale",
        "busy",               "invalid",          "internal",
    };
    if (code) {
        for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
            if (strcmp(code, codes[i]) == 0) return (int)i + 1;
        }
    }
    return 14;
}

static int abi_point_resolve(wasm_exec_env_t env, const char *name, uint32_t *handle,
                             uint32_t *access) {
    (void)env;
    if (!g_sdk || !name || !handle || !access) return 13;
    const char *access_text = NULL;
    const char *err = edge_app_point_resolve(g_sdk, name, handle, &access_text);
    if (err) return error_index(err);
    *access = access_text && strcmp(access_text, "publish") == 0 ? 1u : 0u;
    return 0;
}

static int abi_point_read(wasm_exec_env_t env, uint32_t handle, double *value, uint32_t *quality,
                          uint64_t *source_ts) {
    (void)env;
    if (!g_sdk || !value || !quality || !source_ts) return 13;
    edge_app_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    const char *err = edge_app_point_read(g_sdk, handle, &sample);
    if (err) return error_index(err);
    if (sample.value.kind == EDGE_APP_VALUE_INTEGER) {
        *value = (double)sample.value.integer;
    } else if (sample.value.kind == EDGE_APP_VALUE_BOOLEAN) {
        *value = sample.value.boolean ? 1.0 : 0.0;
    } else {
        *value = sample.value.number;
    }
    *quality = sample.quality && strcmp(sample.quality, "bad") == 0
                   ? 1u
                   : (sample.quality && strcmp(sample.quality, "uncertain") == 0 ? 2u : 0u);
    *source_ts = sample.source_ts;
    return 0;
}

static int abi_point_publish(wasm_exec_env_t env, uint32_t handle, double value, uint32_t quality,
                             uint64_t source_ts) {
    (void)env;
    if (!g_sdk) return 13;
    edge_app_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    sample.value.kind = EDGE_APP_VALUE_FLOAT;
    sample.value.number = value;
    sample.quality = quality == 1 ? "bad" : (quality == 2 ? "uncertain" : "good");
    sample.source_ts = source_ts;
    const char *err = edge_app_point_publish(g_sdk, handle, &sample);
    return err ? error_index(err) : 0;
}

static void abi_log(wasm_exec_env_t env, uint32_t level, const char *message) {
    (void)env;
    if (!g_sdk || !message) return;
    static const char *levels[] = {"debug", "info", "warn", "error"};
    edge_app_log(g_sdk, levels[level < 4 ? level : 3], message);
}

static uint64_t abi_now(wasm_exec_env_t env) {
    (void)env;
    uint64_t now = 0;
    if (g_sdk && edge_app_now(g_sdk, &now) == NULL) return now;
    return 0;
}

static NativeSymbol kSymbols[] = {
    {"point_resolve", abi_point_resolve, "(***)i", NULL},
    {"point_read", abi_point_read, "(i***)i", NULL},
    {"point_publish", abi_point_publish, "(iFiI)i", NULL},
    {"app_log", abi_log, "(i*)", NULL},
    {"app_now", abi_now, "()I", NULL},
};

static uint8_t *read_file(const char *path, uint32_t *size_out) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "app-host: cannot open %s: %s\n", path, strerror(errno));
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    if (size <= 0 || size > 64 * 1024 * 1024) {
        fclose(file);
        return NULL;
    }
    uint8_t *buffer = malloc((size_t)size);
    if (!buffer || fread(buffer, 1, (size_t)size, file) != (size_t)size) {
        free(buffer);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *size_out = (uint32_t)size;
    return buffer;
}

static int run_app(const char *wasm_path, int fd, uint32_t heap_kb, uint32_t stack_kb) {
    uint32_t wasm_size = 0;
    uint8_t *wasm_bytes = read_file(wasm_path, &wasm_size);
    if (!wasm_bytes) return 3;

    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(init_args));
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
    init_args.native_module_name = "env";
    init_args.native_symbols = kSymbols;
    init_args.n_native_symbols = sizeof(kSymbols) / sizeof(kSymbols[0]);
    if (!wasm_runtime_full_init(&init_args)) {
        fprintf(stderr, "app-host: runtime init failed\n");
        free(wasm_bytes);
        return 2;
    }

    char error[160];
    wasm_module_t module = wasm_runtime_load(wasm_bytes, wasm_size, error, sizeof(error));
    if (!module) {
        fprintf(stderr, "app-host: load failed: %s\n", error);
        free(wasm_bytes);
        wasm_runtime_destroy();
        return 3;
    }
    wasm_module_inst_t inst = wasm_runtime_instantiate(module, stack_kb * 1024, heap_kb * 1024,
                                                       error, sizeof(error));
    if (!inst) {
        fprintf(stderr, "app-host: instantiate failed: %s\n", error);
        wasm_runtime_unload(module);
        free(wasm_bytes);
        wasm_runtime_destroy();
        return 4;
    }

    g_sdk = edge_app_sdk_create(fd);
    if (!g_sdk) {
        wasm_runtime_deinstantiate(inst);
        wasm_runtime_unload(module);
        free(wasm_bytes);
        wasm_runtime_destroy();
        return 5;
    }

    int rc = 0;
    if (wasm_runtime_lookup_function(inst, "app_init")) {
        if (!wasm_application_execute_func(inst, "app_init", 0, NULL)) {
            const char *exception = wasm_runtime_get_exception(inst);
            fprintf(stderr, "app-host: app_init failed: %s\n",
                    exception ? exception : "unknown exception");
            rc = 6;
        }
    }

    while (rc == 0 && !g_stop) {
        char byte;
        ssize_t n = read(fd, &byte, 1);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
    }

    if (wasm_runtime_lookup_function(inst, "app_stop")) {
        wasm_application_execute_func(inst, "app_stop", 0, NULL);
    }

    edge_app_sdk_destroy(g_sdk);
    g_sdk = NULL;
    wasm_runtime_deinstantiate(inst);
    wasm_runtime_unload(module);
    free(wasm_bytes);
    wasm_runtime_destroy();
    return rc;
}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    const char *wasm_path = NULL;
    int fd = -1;
    int close_fd = -1;
    uint32_t heap_kb = 64;
    uint32_t stack_kb = 64;

    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            fprintf(stderr, "app-host: missing value for %s\n", argv[i]);
            return 1;
        }
        if (strcmp(argv[i], "--fd") == 0) {
            fd = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--wasm") == 0) {
            wasm_path = argv[i + 1];
        } else if (strcmp(argv[i], "--close-fd") == 0) {
            close_fd = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--heap-kb") == 0) {
            heap_kb = (uint32_t)strtoul(argv[i + 1], NULL, 10);
        } else if (strcmp(argv[i], "--stack-kb") == 0) {
            stack_kb = (uint32_t)strtoul(argv[i + 1], NULL, 10);
        } else {
            fprintf(stderr, "app-host: unknown option %s\n", argv[i]);
            return 1;
        }
    }
    if (fd < 0 || !wasm_path || heap_kb == 0 || stack_kb == 0) {
        fprintf(stderr,
                "usage: app-host --fd N --wasm FILE [--heap-kb N] [--stack-kb N] [--close-fd N]\n");
        return 1;
    }
    if (close_fd >= 0 && close_fd != fd) close(close_fd);

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    return run_app(wasm_path, fd, heap_kb, stack_kb);
}
