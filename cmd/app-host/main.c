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
#include "no_exec_filter.h"
#include "wasm_export.h"

static edge_app_sdk_t *g_sdk;
static wasm_module_inst_t g_inst;
static int g_has_event_export;
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
        /* Ext ABI only: not in the frozen WIT error enum, appended after it. */
        if (strcmp(code, "not_set") == 0) return 15;
        if (strcmp(code, "not_supported") == 0) return 16;
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

static int abi_point_subscribe(wasm_exec_env_t env, uint32_t handle, double deadband,
                               uint32_t min_interval_ms, uint32_t max_interval_ms,
                               uint32_t *subscription) {
    (void)env;
    if (!g_sdk || !subscription || !g_has_event_export) return 13;
    const char *err = edge_app_point_subscribe(g_sdk, handle, deadband, min_interval_ms,
                                               max_interval_ms, subscription);
    return err ? error_index(err) : 0;
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

/* Ext ABI: (key_ptr, key_len, out_ptr, cap) -> JSON text plus a status code. */
#define APP_HOST_SETTING_KEY_MAX 64

static int abi_setting_get(wasm_exec_env_t env, const char *key, uint32_t key_len, char *out,
                           uint32_t cap) {
    (void)env;
    if (!g_sdk || !key || !out) return 13;
    if (key_len == 0) return 13;
    if (key_len >= APP_HOST_SETTING_KEY_MAX) return 10;
    char name[APP_HOST_SETTING_KEY_MAX];
    memcpy(name, key, key_len);
    name[key_len] = '\0';
    const char *err = edge_app_setting_get(g_sdk, name, out, cap);
    return err ? error_index(err) : 0;
}

static int abi_setting_watch(wasm_exec_env_t env) {
    (void)env;
    if (!g_sdk) return 13;
    const char *err = edge_app_setting_watch(g_sdk);
    return err ? error_index(err) : 0;
}

static NativeSymbol kSymbols[] = {
    {"point_resolve", abi_point_resolve, "(***)i", NULL},
    {"point_read", abi_point_read, "(i***)i", NULL},
    {"point_subscribe", abi_point_subscribe, "(iFii*)i", NULL},
    {"point_publish", abi_point_publish, "(iFiI)i", NULL},
    {"app_log", abi_log, "(i*)", NULL},
    {"app_now", abi_now, "()I", NULL},
    {"setting_get", abi_setting_get, "(*i*i)i", NULL},
    {"setting_watch", abi_setting_watch, "()i", NULL},
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

/* v1 export convention: app_on_event(ptr, len), UTF-8 JSON in guest memory. */
static int child_on_event(void *ctx, const char *event_json) {
    (void)ctx;
    if (!g_inst || !event_json || !g_has_event_export) return -1;
    size_t len = strlen(event_json);
    if (len >= UINT32_MAX) return -1;
    void *native = NULL;
    uint64_t ptr = wasm_runtime_module_malloc(g_inst, (uint64_t)len + 1, &native);
    if (ptr == 0 || native == NULL) return -1;
    memcpy(native, event_json, len + 1);

    /* This WAMR takes execute_func argv as decimal strings, one per wasm arg. */
    char ptr_text[16];
    char len_text[16];
    snprintf(ptr_text, sizeof ptr_text, "%u", (unsigned)ptr);
    snprintf(len_text, sizeof len_text, "%u", (unsigned)len);
    char *argv[2] = {ptr_text, len_text};
    bool ok = wasm_application_execute_func(g_inst, "app_on_event", 2, argv);
    wasm_runtime_module_free(g_inst, ptr);
    if (!ok) {
        wasm_runtime_clear_exception(g_inst);
        return -1;
    }
    return 0;
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

    g_inst = inst;
    g_has_event_export = wasm_runtime_lookup_function(inst, "app_on_event") != NULL;

    g_sdk = edge_app_sdk_create(fd);
    if (!g_sdk) {
        wasm_runtime_deinstantiate(inst);
        wasm_runtime_unload(module);
        free(wasm_bytes);
        wasm_runtime_destroy();
        return 5;
    }
    edge_app_sdk_set_exports(g_sdk, NULL, child_on_event, NULL, NULL);

    int rc = 0;
    if (wasm_runtime_lookup_function(inst, "app_init")) {
        if (!wasm_application_execute_func(inst, "app_init", 0, NULL)) {
            const char *exception = wasm_runtime_get_exception(inst);
            fprintf(stderr, "app-host: app_init failed: %s\n",
                    exception ? exception : "unknown exception");
            rc = 6;
        }
    }

    /* The event frame can exceed the stack-buffer budget; keep both buffers on
     * the heap for the life of the child loop. */
    char *frame = malloc(4096);
    char *response = malloc(256);
    if (!frame || !response) {
        fprintf(stderr, "app-host: out of memory for frame buffers\n");
        rc = 7;
        goto frame_out;
    }
    while (rc == 0 && !g_stop && edge_app_sdk_next_frame(g_sdk, frame, 4096) == 0) {
        if (edge_app_sdk_dispatch(g_sdk, frame, response, 256) != 0) break;
        if (edge_app_sdk_send_frame(g_sdk, response) != 0) break;
    }
frame_out:
    free(frame);
    free(response);

    if (wasm_runtime_lookup_function(inst, "app_stop")) {
        wasm_application_execute_func(inst, "app_stop", 0, NULL);
    }

    edge_app_sdk_destroy(g_sdk);
    g_sdk = NULL;
    wasm_runtime_deinstantiate(inst);
    g_inst = NULL;
    wasm_runtime_unload(module);
    free(wasm_bytes);
    wasm_runtime_destroy();
    return rc;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGPIPE, SIG_IGN);
    if (app_host_install_no_exec_filter() != 0) {
        const char *allow_degraded = getenv("EDGEC_APP_HOST_ALLOW_DEGRADED");
        if (!allow_degraded || strcmp(allow_degraded, "1") != 0) {
            fprintf(stderr, "app-host: failed to install no-exec filter: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }
        fprintf(stderr, "app-host: no-exec seccomp filter unavailable (degraded): %s\n",
                strerror(errno));
    }

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
