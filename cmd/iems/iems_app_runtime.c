/**
 * @file iems_app_runtime.c
 * @brief Per-app app-host processes and their Host ABI servers.
 */

#include "iems_app_runtime.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <uv.h>

#include "edge_app_host.h"
#include "edge_launcher_port.h"
#include "edge_log.h"
#include "iems_app_host_ports.h"

typedef struct {
    iems_app_runtime_t *owner;
    char app_id[64];
    char wasm_path[256];
    uv_process_t process;
    pid_t launcher_pid;
    app_host_server_t *server;
    iems_app_host_ctx_t ports_ctx;
    app_host_ports_t ports;
    int stopping;
    int process_closed;
} app_runtime_t;

typedef struct {
    uv_timer_t timer;
    iems_app_runtime_t *rt;
    char app_id[64];
    char wasm_path[256];
    uint32_t heap_kb;
    uint32_t stack_kb;
} restart_timer_t;

struct iems_app_runtime_s {
    uv_loop_t *loop;
    iems_app_runtime_config_t cfg;
    char app_host_path[256];
    char apps_root[256];
    int use_launcher;
    int sigchld_active;
    uv_signal_t sigchld;
    app_runtime_t *apps[IEMS_APP_RUNTIME_MAX];
    restart_timer_t *restarts[IEMS_APP_RUNTIME_MAX];
};

static app_runtime_t *find_app(iems_app_runtime_t *rt, const char *app_id) {
    if (!rt || !app_id) return NULL;
    for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
        if (rt->apps[i] && strcmp(rt->apps[i]->app_id, app_id) == 0) return rt->apps[i];
    }
    return NULL;
}

static void maybe_free_app(app_runtime_t *app) {
    if (app->process_closed && !app->server) free(app);
}

static void on_abi_server_closed(void *ctx) {
    app_runtime_t *app = (app_runtime_t *)ctx;
    app->server = NULL;
    maybe_free_app(app);
}

static void on_process_close(uv_handle_t *handle) {
    app_runtime_t *app = (app_runtime_t *)handle->data;
    app->process_closed = 1;
    maybe_free_app(app);
}

static void finish_app(iems_app_runtime_t *rt, app_runtime_t *app, int64_t exit_status,
                       int term_signal, int via_uv) {
    if (!app->stopping) {
        LOG_WARN(LOG_CAT_CORE, "AppHost: %s exited unexpectedly (status=%lld signal=%d)",
                 app->app_id, (long long)exit_status, term_signal);
        if (rt->cfg.on_fault) {
            rt->cfg.on_fault(rt->cfg.on_fault_ctx, app->app_id, exit_status, term_signal);
        }
    }
    for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
        if (rt->apps[i] == app) rt->apps[i] = NULL;
    }
    if (app->server) app_host_server_destroy(app->server);
    if (via_uv) {
        uv_close((uv_handle_t *)&app->process, on_process_close);
    } else {
        app->process_closed = 1;
        maybe_free_app(app);
    }
}

static void on_process_exit(uv_process_t *process, int64_t exit_status, int term_signal) {
    app_runtime_t *app = (app_runtime_t *)process->data;
    finish_app(app->owner, app, exit_status, term_signal, 1);
}

/* A launcher-spawned app-host is a fork of this process, not a libuv handle.
 * libuv may reap it first while dispatching its own children; ECHILD means the
 * app is gone too and no exit status is left to read. */
static void on_sigchld(uv_signal_t *handle, int signum) {
    (void)signum;
    iems_app_runtime_t *rt = (iems_app_runtime_t *)handle->data;
    if (!rt) return;
    for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
        app_runtime_t *app = rt->apps[i];
        if (!app || app->launcher_pid <= 0) continue;
        int status = 0;
        pid_t got = waitpid(app->launcher_pid, &status, WNOHANG);
        if (got == 0) continue;
        int64_t exit_status = 0;
        int term_signal = 0;
        if (got == app->launcher_pid) {
            if (WIFEXITED(status)) exit_status = WEXITSTATUS(status);
            if (WIFSIGNALED(status)) term_signal = WTERMSIG(status);
        }
        app->launcher_pid = 0;
        finish_app(rt, app, exit_status, term_signal, 0);
    }
}

iems_app_runtime_t *iems_app_runtime_create(uv_loop_t *loop, const iems_app_runtime_config_t *cfg) {
    if (!loop || !cfg || !cfg->hub || !cfg->db) return NULL;
    iems_app_runtime_t *rt = calloc(1, sizeof(*rt));
    if (!rt) return NULL;
    rt->loop = loop;
    rt->cfg = *cfg;

    const char *path = cfg->app_host_path;
    if (!path || !path[0]) path = getenv("EDGEC_APP_HOST_PATH");
    if (!path || !path[0]) path = "/usr/libexec/edgeos/app-host";
    snprintf(rt->app_host_path, sizeof(rt->app_host_path), "%s", path);
    if (access(rt->app_host_path, X_OK) != 0) {
        snprintf(rt->app_host_path, sizeof(rt->app_host_path), "app-host");
    }

    const char *apps_root = getenv("EDGEC_APPS_ROOT");
    if (!apps_root || !apps_root[0]) apps_root = getenv("EDGE_APPS_DIR");
    if (!apps_root || !apps_root[0]) apps_root = "/var/edgeos/apps";
    snprintf(rt->apps_root, sizeof(rt->apps_root), "%s", apps_root);
    setenv("EDGEC_APPS_ROOT", rt->apps_root, 1);

    const char *launcher = getenv("EDGEC_LAUNCHER");
    rt->use_launcher = !(launcher && strcmp(launcher, "0") == 0);
    if (rt->use_launcher) {
        rt->sigchld.data = rt;
        if (uv_signal_init(loop, &rt->sigchld) != 0 ||
            uv_signal_start(&rt->sigchld, on_sigchld, SIGCHLD) != 0) {
            LOG_WARN(LOG_CAT_CORE, "AppHost: SIGCHLD watcher unavailable; launcher disabled");
            rt->use_launcher = 0;
        } else {
            rt->sigchld_active = 1;
        }
    }
    return rt;
}

void iems_app_runtime_stop(iems_app_runtime_t *rt, const char *app_id) {
    app_runtime_t *app = find_app(rt, app_id);
    if (!app) return;
    app->stopping = 1;
    if (app->launcher_pid > 0) {
        kill(app->launcher_pid, SIGTERM);
    } else {
        uv_process_kill(&app->process, SIGTERM);
    }
    if (app->server) app_host_server_destroy(app->server);
}

void iems_app_runtime_stop_all(iems_app_runtime_t *rt) {
    if (!rt) return;
    for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
        if (rt->apps[i]) iems_app_runtime_stop(rt, rt->apps[i]->app_id);
    }
}

int iems_app_runtime_is_running(iems_app_runtime_t *rt, const char *app_id) {
    return find_app(rt, app_id) != NULL;
}

int iems_app_runtime_pid(iems_app_runtime_t *rt, const char *app_id) {
    app_runtime_t *app = find_app(rt, app_id);
    if (!app) return -1;
    if (app->launcher_pid > 0) return (int)app->launcher_pid;
    return (int)app->process.pid;
}

static void on_timer_close(uv_handle_t *handle) {
    free(handle->data);
}

static void on_restart_timer(uv_timer_t *timer) {
    restart_timer_t *restart = (restart_timer_t *)timer->data;
    iems_app_runtime_t *rt = restart->rt;
    for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
        if (rt->restarts[i] == restart) rt->restarts[i] = NULL;
    }
    if (iems_app_runtime_start(rt, restart->app_id, restart->wasm_path, restart->heap_kb,
                               restart->stack_kb) == 0 &&
        rt->cfg.on_restarted) {
        rt->cfg.on_restarted(rt->cfg.on_restarted_ctx, restart->app_id);
    }
    uv_close((uv_handle_t *)timer, on_timer_close);
}

int iems_app_runtime_restart_later(iems_app_runtime_t *rt, const char *app_id,
                                   const char *wasm_path, uint32_t delay_ms, uint32_t heap_kb,
                                   uint32_t stack_kb) {
    if (!rt || !app_id || !wasm_path) return -1;
    for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
        if (rt->restarts[i] && strcmp(rt->restarts[i]->app_id, app_id) == 0) return 0;
    }
    int slot = -1;
    for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
        if (!rt->restarts[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;

    restart_timer_t *restart = calloc(1, sizeof(*restart));
    if (!restart) return -1;
    restart->rt = rt;
    snprintf(restart->app_id, sizeof(restart->app_id), "%s", app_id);
    snprintf(restart->wasm_path, sizeof(restart->wasm_path), "%s", wasm_path);
    restart->heap_kb = heap_kb;
    restart->stack_kb = stack_kb;
    uv_timer_init(rt->loop, &restart->timer);
    restart->timer.data = restart;
    rt->restarts[slot] = restart;
    if (uv_timer_start(&restart->timer, on_restart_timer, delay_ms, 0) != 0) {
        rt->restarts[slot] = NULL;
        uv_close((uv_handle_t *)&restart->timer, on_timer_close);
        return -1;
    }
    return 0;
}

/* The launcher only execs <apps_root>/<app_id>/app-host. Seed that fixed path
 * from the platform binary on first start; a copy is the cross-filesystem
 * fallback when the platform root and the apps root differ. */
static int ensure_launcher_target(iems_app_runtime_t *rt, app_runtime_t *app) {
    char dir[384];
    char target[384];
    int n = snprintf(dir, sizeof(dir), "%s/%s", rt->apps_root, app->app_id);
    if (n < 0 || (size_t)n >= sizeof(dir)) return -1;
    n = snprintf(target, sizeof(target), "%s/app-host", dir);
    if (n < 0 || (size_t)n >= sizeof(target)) return -1;
    if (access(target, X_OK) == 0) return 0;

    const char *src = rt->app_host_path;
    if (access(src, X_OK) != 0) return -1;
    (void)chmod(dir, 0755);
    if (link(src, target) != 0) {
        FILE *in = fopen(src, "rb");
        FILE *out = in ? fopen(target, "wb") : NULL;
        if (in && out) {
            char buf[512];
            size_t got;
            while ((got = fread(buf, 1, sizeof(buf), in)) > 0) {
                if (fwrite(buf, 1, got, out) != got) break;
            }
        }
        if (in) fclose(in);
        if (out) fclose(out);
    }
    (void)chmod(target, 0755);
    (void)chmod(app->wasm_path, 0644);
    return access(target, X_OK) == 0 ? 0 : -1;
}

static int start_via_launcher(iems_app_runtime_t *rt, app_runtime_t *app, int abi_fd,
                              uint32_t heap_kb, uint32_t stack_kb) {
    if (ensure_launcher_target(rt, app) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppHost: launcher target unavailable for %s", app->app_id);
        return -1;
    }

    launch_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    snprintf(spec.app_id, sizeof(spec.app_id), "%s", app->app_id);
    uint32_t hash = 2166136261u;
    for (const char *p = app->app_id; *p; p++) {
        hash ^= (uint8_t)*p;
        hash *= 16777619u;
    }
    uint32_t span = EDGEC_LAUNCH_UID_MAX - EDGEC_LAUNCH_UID_MIN + 1u;
    spec.uid = EDGEC_LAUNCH_UID_MIN + hash % span;
    spec.gid = spec.uid;
    spec.seccomp_profile = EDGEC_LAUNCH_SECCOMP_MINIMAL;
    spec.pids_max = 32;
    spec.mem_max_bytes = (uint64_t)(heap_kb ? heap_kb : 64) * 1024ULL + (64ULL << 20);
    spec.fd_count = 1;

    char heap_text[16];
    char stack_text[16];
    snprintf(heap_text, sizeof(heap_text), "%u", heap_kb ? heap_kb : 64);
    snprintf(stack_text, sizeof(stack_text), "%u", stack_kb ? stack_kb : 64);
    const char *args[] = {"--fd",       "3",          "--wasm",   app->wasm_path,
                          "--heap-kb",  heap_text,   "--stack-kb", stack_text};
    spec.argv_count = 8;
    const int inherit_fds[] = {abi_fd};

    launch_result_t result;
    int rc = edge_launcher_spawn(&spec, args, NULL, inherit_fds, &result);
    if (rc != EDGEC_LAUNCH_OK) {
        LOG_WARN(LOG_CAT_CORE, "AppHost: launcher rejected %s: %d", app->app_id, rc);
        return -1;
    }
    if (result.degraded) {
        LOG_WARN(LOG_CAT_CORE, "AppHost: launcher degraded 0x%x for %s", result.degraded,
                 app->app_id);
    }
    app->launcher_pid = (pid_t)result.pid;
    return 0;
}

int iems_app_runtime_start(iems_app_runtime_t *rt, const char *app_id, const char *wasm_path,
                           uint32_t heap_kb, uint32_t stack_kb) {
    if (!rt || !app_id || !app_id[0] || !wasm_path || !wasm_path[0]) return -1;
    if (find_app(rt, app_id)) return 0;

    int slot = -1;
    for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
        if (!rt->apps[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;

    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppHost: socketpair failed for %s: %s", app_id, strerror(errno));
        return -1;
    }

    app_runtime_t *app = calloc(1, sizeof(*app));
    if (!app) {
        close(fds[0]);
        close(fds[1]);
        return -1;
    }
    app->owner = rt;
    snprintf(app->app_id, sizeof(app->app_id), "%s", app_id);
    snprintf(app->wasm_path, sizeof(app->wasm_path), "%s", wasm_path);
    app->ports_ctx = (iems_app_host_ctx_t){
        .app_id = app->app_id,
        .hub = rt->cfg.hub,
        .db = rt->cfg.db,
        .product_tags = rt->cfg.tags,
        .product_tag_count = rt->cfg.tag_count,
        .app_base = rt->cfg.app_base,
    };
    iems_app_host_ports_init(&app->ports, &app->ports_ctx);
    app->server = app_host_server_create_ex(rt->loop, fds[0], &app->ports,
                                            on_abi_server_closed, app);
    if (!app->server) {
        LOG_WARN(LOG_CAT_CORE, "AppHost: ABI server failed for %s", app_id);
        close(fds[1]);
        free(app);
        return -1;
    }
    /* libuv may clear CLOEXEC when it adopts the fd; the child also closes it
     * explicitly via --close-fd, so EOF arrives on stop either way. */
    fcntl(fds[0], F_SETFD, FD_CLOEXEC);

    if (rt->use_launcher) {
        int rc = start_via_launcher(rt, app, fds[1], heap_kb, stack_kb);
        close(fds[1]);
        if (rc != 0) {
            app->process_closed = 1;
            app_host_server_destroy(app->server);
            maybe_free_app(app);
            return -1;
        }
        rt->apps[slot] = app;
        return 0;
    }

    char fd_text[16];
    char server_fd_text[16];
    char heap_text[16];
    char stack_text[16];
    snprintf(fd_text, sizeof(fd_text), "%d", fds[1]);
    snprintf(server_fd_text, sizeof(server_fd_text), "%d", fds[0]);
    snprintf(heap_text, sizeof(heap_text), "%u", heap_kb ? heap_kb : 64);
    snprintf(stack_text, sizeof(stack_text), "%u", stack_kb ? stack_kb : 64);
    char *args[] = {"app-host",    "--fd",       fd_text,     "--wasm", app->wasm_path,
                    "--close-fd",  server_fd_text, "--heap-kb", heap_text, "--stack-kb",
                    stack_text,    NULL};

    uv_stdio_container_t stdio[3];
    memset(stdio, 0, sizeof(stdio));
    stdio[0].flags = UV_IGNORE;
    stdio[1].flags = UV_IGNORE;
    stdio[2].flags = UV_INHERIT_FD;
    stdio[2].data.fd = STDERR_FILENO;

    uv_process_options_t options;
    memset(&options, 0, sizeof(options));
    options.exit_cb = on_process_exit;
    options.file = rt->app_host_path;
    options.args = args;
    options.stdio = stdio;
    options.stdio_count = 3;

    app->process.data = app;
    int rc = uv_spawn(rt->loop, &app->process, &options);
    close(fds[1]);
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppHost: spawn %s failed: %s", rt->app_host_path,
                 uv_strerror(rc));
        app->process_closed = 1;
        app_host_server_destroy(app->server);
        maybe_free_app(app);
        return -1;
    }
    rt->apps[slot] = app;
    return 0;
}

const char *iems_app_runtime_app_host_path(const iems_app_runtime_t *rt) {
    return rt ? rt->app_host_path : NULL;
}

void iems_app_runtime_destroy(iems_app_runtime_t *rt) {
    if (!rt) return;
    iems_app_runtime_stop_all(rt);
    if (rt->sigchld_active) {
        uv_signal_stop(&rt->sigchld);
        uv_close((uv_handle_t *)&rt->sigchld, NULL);
        rt->sigchld_active = 0;
    }
    for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
        if (rt->restarts[i]) {
            uv_timer_stop(&rt->restarts[i]->timer);
            uv_close((uv_handle_t *)&rt->restarts[i]->timer, on_timer_close);
            rt->restarts[i] = NULL;
        }
    }
    for (int attempt = 0; attempt < 200; attempt++) {
        uv_run(rt->loop, UV_RUN_NOWAIT);
        int running = 0;
        for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
            if (rt->apps[i]) running = 1;
        }
        if (!running) break;
        if (attempt == 50) {
            for (int i = 0; i < IEMS_APP_RUNTIME_MAX; i++) {
                if (!rt->apps[i]) continue;
                if (rt->apps[i]->launcher_pid > 0) {
                    kill(rt->apps[i]->launcher_pid, SIGKILL);
                } else {
                    uv_process_kill(&rt->apps[i]->process, SIGKILL);
                }
            }
        }
        usleep(10000);
    }
    free(rt);
}
