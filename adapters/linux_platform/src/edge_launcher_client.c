#define _GNU_SOURCE

#include "edge_launcher_port.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define LAUNCH_REPORT_MAGIC 0x454C4E43u
#define LAUNCH_TIMEOUT_MS 5000

typedef struct {
    uint32_t magic;
    int32_t error_code;
    uint32_t degraded;
    uint32_t reserved;
} launch_report_t;

_Static_assert(sizeof(launch_report_t) == 16, "launch report is wire-stable");

static void report_and_exit(int fd, int error_code, uint32_t degraded) {
    launch_report_t rep = {LAUNCH_REPORT_MAGIC, error_code, degraded, 0};
    (void)send(fd, &rep, sizeof(rep), MSG_NOSIGNAL);
    _exit(127);
}

static int resolve_helper(char *out, size_t out_size) {
    const char *env = getenv("EDGEC_LAUNCHER_PATH");
    if (env && env[0] != '\0') {
        int n = snprintf(out, out_size, "%s", env);
        if (n < 0 || (size_t)n >= out_size || access(out, X_OK) != 0) return EDGEC_LAUNCH_ERR_ARG;
        return EDGEC_LAUNCH_OK;
    }
    const char *installed = "/usr/libexec/edgeos/edgeos-launcher";
    if (access(installed, X_OK) == 0) {
        int n = snprintf(out, out_size, "%s", installed);
        return (n > 0 && (size_t)n < out_size) ? EDGEC_LAUNCH_OK : EDGEC_LAUNCH_ERR_ARG;
    }
    /* Build tree: the helper sits next to the product binary. */
    char exe[512];
    ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len <= 0) return EDGEC_LAUNCH_ERR_ARG;
    exe[len] = '\0';
    char *slash = strrchr(exe, '/');
    if (!slash) return EDGEC_LAUNCH_ERR_ARG;
    *slash = '\0';
    int n = snprintf(out, out_size, "%s/edgeos-launcher", exe);
    if (n < 0 || (size_t)n >= out_size || access(out, X_OK) != 0) return EDGEC_LAUNCH_ERR_ARG;
    return EDGEC_LAUNCH_OK;
}

static int build_wire(const launch_spec_t *spec, const char *const args[],
                      const char *const env[], uint8_t *buf, size_t cap, size_t *out_len) {
    if (sizeof(*spec) > cap) return EDGEC_LAUNCH_ERR_ARG;
    if ((spec->argv_count > 0 && !args) || (spec->env_count > 0 && !env)) {
        return EDGEC_LAUNCH_ERR_ARG;
    }
    memcpy(buf, spec, sizeof(*spec));
    size_t off = sizeof(*spec);
    for (uint32_t i = 0; i < spec->argv_count; i++) {
        if (!args[i]) return EDGEC_LAUNCH_ERR_ARG;
        size_t len = strnlen(args[i], EDGEC_LAUNCH_ARG_BYTES_MAX + 1u);
        if (len > EDGEC_LAUNCH_ARG_BYTES_MAX || off + len + 1 > cap) return EDGEC_LAUNCH_ERR_ARG;
        memcpy(buf + off, args[i], len + 1);
        off += len + 1;
    }
    for (uint32_t i = 0; i < spec->env_count; i++) {
        if (edge_launcher_env_entry_valid(env[i]) != EDGEC_LAUNCH_OK) return EDGEC_LAUNCH_ERR_ARG;
        size_t len = strnlen(env[i], EDGEC_LAUNCH_ARG_BYTES_MAX + 1u);
        if (len > EDGEC_LAUNCH_ARG_BYTES_MAX || off + len + 1 > cap) return EDGEC_LAUNCH_ERR_ARG;
        memcpy(buf + off, env[i], len + 1);
        off += len + 1;
    }
    *out_len = off;
    return EDGEC_LAUNCH_OK;
}

int edge_launcher_spawn(const launch_spec_t *spec, const char *const args[],
                        const char *const env[], const int inherit_fds[],
                        launch_result_t *result) {
    if (!result) return EDGEC_LAUNCH_ERR_ARG;
    memset(result, 0, sizeof(*result));
    int rc = edge_launcher_validate_spec(spec);
    if (rc != EDGEC_LAUNCH_OK) {
        result->error_code = rc;
        return rc;
    }
    if (spec->fd_count > 0 && !inherit_fds) {
        result->error_code = EDGEC_LAUNCH_ERR_ARG;
        return EDGEC_LAUNCH_ERR_ARG;
    }
    for (uint32_t i = 0; i < spec->fd_count; i++) {
        if (inherit_fds[i] < 3 || fcntl(inherit_fds[i], F_GETFD) < 0) {
            result->error_code = EDGEC_LAUNCH_ERR_ARG;
            return EDGEC_LAUNCH_ERR_ARG;
        }
    }

    char helper[512];
    rc = resolve_helper(helper, sizeof(helper));
    if (rc != EDGEC_LAUNCH_OK) {
        result->error_code = rc;
        return rc;
    }

    uint8_t wire[EDGEC_LAUNCH_WIRE_MAX];
    size_t wire_len = 0;
    rc = build_wire(spec, args, env, wire, sizeof(wire), &wire_len);
    if (rc != EDGEC_LAUNCH_OK) {
        result->error_code = rc;
        return rc;
    }

    int sv[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv) != 0) {
        result->error_code = EDGEC_LAUNCH_ERR_SYSTEM;
        return EDGEC_LAUNCH_ERR_SYSTEM;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(sv[0]);
        close(sv[1]);
        result->error_code = EDGEC_LAUNCH_ERR_SYSTEM;
        return EDGEC_LAUNCH_ERR_SYSTEM;
    }
    if (pid == 0) {
        close(sv[1]);
        /* Move pass-through fds aside before the fixed 3.. mapping so a later
         * dup2 cannot clobber a source that has not been copied yet. */
        int tmp[EDGEC_LAUNCH_FD_MAX];
        for (uint32_t i = 0; i < spec->fd_count; i++) {
            tmp[i] = fcntl(inherit_fds[i], F_DUPFD_CLOEXEC, 40);
            if (tmp[i] < 0) report_and_exit(sv[0], EDGEC_LAUNCH_ERR_SYSTEM, 0);
        }
        if (dup2(sv[0], EDGEC_LAUNCH_STATUS_FD) < 0) {
            report_and_exit(sv[0], EDGEC_LAUNCH_ERR_SYSTEM, 0);
        }
        if (sv[0] != EDGEC_LAUNCH_STATUS_FD) close(sv[0]);
        for (uint32_t i = 0; i < spec->fd_count; i++) {
            if (dup2(tmp[i], EDGEC_LAUNCH_FD_BASE + (int)i) < 0) {
                report_and_exit(EDGEC_LAUNCH_STATUS_FD, EDGEC_LAUNCH_ERR_SYSTEM, 0);
            }
            close(tmp[i]);
            (void)fcntl(EDGEC_LAUNCH_FD_BASE + (int)i, F_SETFD, 0);
        }
        (void)prctl(PR_SET_PDEATHSIG, SIGKILL);
        char fd_arg[16];
        snprintf(fd_arg, sizeof(fd_arg), "%d", EDGEC_LAUNCH_STATUS_FD);
        char *child_argv[] = {helper, fd_arg, NULL};
        execv(helper, child_argv);
        report_and_exit(EDGEC_LAUNCH_STATUS_FD, EDGEC_LAUNCH_ERR_EXEC, 0);
    }

    close(sv[0]);
    if (send(sv[1], wire, wire_len, MSG_NOSIGNAL) != (ssize_t)wire_len) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        close(sv[1]);
        result->error_code = EDGEC_LAUNCH_ERR_SYSTEM;
        return EDGEC_LAUNCH_ERR_SYSTEM;
    }

    int err = EDGEC_LAUNCH_OK;
    uint32_t degraded = 0;
    for (;;) {
        struct pollfd pfd = {.fd = sv[1], .events = POLLIN};
        int pr = poll(&pfd, 1, LAUNCH_TIMEOUT_MS);
        if (pr < 0) {
            if (errno == EINTR) continue;
            err = EDGEC_LAUNCH_ERR_SYSTEM;
            break;
        }
        if (pr == 0) {
            err = EDGEC_LAUNCH_ERR_TIMEOUT;
            break;
        }
        launch_report_t rep;
        ssize_t n = recv(sv[1], &rep, sizeof(rep), 0);
        if (n == 0) break; /* fd closed by exec: success */
        if (n != (ssize_t)sizeof(rep) || rep.magic != LAUNCH_REPORT_MAGIC) {
            err = EDGEC_LAUNCH_ERR_PROTOCOL;
            break;
        }
        degraded |= rep.degraded;
        if (rep.error_code != 0) {
            err = rep.error_code;
            break;
        }
    }
    close(sv[1]);

    if (err != EDGEC_LAUNCH_OK) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        result->pid = 0;
        result->error_code = err;
        result->degraded = degraded;
        return err;
    }
    result->pid = pid;
    result->error_code = EDGEC_LAUNCH_OK;
    result->degraded = degraded;
    return EDGEC_LAUNCH_OK;
}
