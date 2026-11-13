#ifndef EDGE_LAUNCHER_PORT_H
#define EDGE_LAUNCHER_PORT_H

/**
 * @file edge_launcher_port.h
 * @brief Privileged launcher port (T7): fixed app_id -> app-host path, sandbox
 *        setup in a short-lived helper, no supervisor.
 *
 * The caller (product process) never passes an executable path. The helper
 * derives `<EDGEC_APPS_ROOT>/<app_id>/app-host` (default apps root
 * /var/edgeos/apps) and re-validates it before exec. The helper checks the
 * socketpair peer's uid with SO_PEERCRED and only serves a root product process
 * (the product is the TCB; a setuid helper is still refused to non-root
 * callers). It applies setgid/setuid, no_new_privs, cgroup v2 limits, a
 * read-only root mount namespace, and an optional seccomp allowlist. It exits
 * into app-host with the same pid, so the product process remains the parent
 * and no supervisor exists.
 *
 * Environment (trusted, set by the product process):
 *   EDGEC_LAUNCHER_PATH  helper binary; default /usr/libexec/edgeos/edgeos-launcher
 *   EDGEC_APPS_ROOT      app package root; default /var/edgeos/apps
 *   EDGEC_CGROUP_ROOT    cgroup v2 root; default /sys/fs/cgroup
 */

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGEC_LAUNCH_APP_ID_MAX 64u
#define EDGEC_LAUNCH_PATH_MAX 128u
#define EDGEC_LAUNCH_ARG_MAX 64u
#define EDGEC_LAUNCH_FD_MAX 16u
#define EDGEC_LAUNCH_ARG_BYTES_MAX 4096u
#define EDGEC_LAUNCH_WIRE_MAX 65536u
#define EDGEC_LAUNCH_FD_BASE 3
#define EDGEC_LAUNCH_STATUS_FD 31

#define EDGEC_LAUNCH_UID_MIN 20000u
#define EDGEC_LAUNCH_UID_MAX 29999u

/** Negative codes; 0 is success. */
typedef enum {
    EDGEC_LAUNCH_OK = 0,
    EDGEC_LAUNCH_ERR_ARG = -1,
    EDGEC_LAUNCH_ERR_APP_ID = -2,
    EDGEC_LAUNCH_ERR_UID = -3,
    EDGEC_LAUNCH_ERR_PATH = -4,
    EDGEC_LAUNCH_ERR_PRIVILEGE = -5,
    EDGEC_LAUNCH_ERR_SYSTEM = -6,
    EDGEC_LAUNCH_ERR_EXEC = -7,
    EDGEC_LAUNCH_ERR_TIMEOUT = -8,
    EDGEC_LAUNCH_ERR_PROTOCOL = -9
} edge_launch_error_t;

/** Bits reported when the target platform cannot honor a hardening step. */
#define EDGEC_LAUNCH_DEGRADED_CGROUP (1u << 0)
#define EDGEC_LAUNCH_DEGRADED_MOUNT (1u << 1)
#define EDGEC_LAUNCH_DEGRADED_SECCOMP (1u << 2)

typedef enum {
    EDGEC_LAUNCH_SECCOMP_OFF = 0,
    EDGEC_LAUNCH_SECCOMP_MINIMAL = 1
} edge_launch_seccomp_t;

/**
 * Launch request. Wire-stable: the client and helper exchange this struct
 * verbatim, so fields keep fixed widths and no pointers are stored here.
 * argv_count/env_count/fd_count describe arrays passed to
 * edge_launcher_spawn(): arguments (without argv[0]), "NAME=value" env entries,
 * and file descriptors to map to 3..3+fd_count-1 in app-host.
 */
typedef struct {
    char app_id[EDGEC_LAUNCH_APP_ID_MAX];
    char cgroup_path[EDGEC_LAUNCH_PATH_MAX]; /**< leaf under cgroup root; empty -> edgeos/<app_id> */
    char data_dir[EDGEC_LAUNCH_PATH_MAX];    /**< empty -> no data bind mount */
    uint32_t uid;
    uint32_t gid;
    uint32_t seccomp_profile; /**< edge_launch_seccomp_t */
    uint32_t argv_count;
    uint32_t env_count;
    uint32_t fd_count;
    uint32_t pids_max;
    uint64_t mem_max_bytes;
    uint32_t flags; /**< reserved, must be 0 */
    uint32_t reserved;
} launch_spec_t;

typedef struct {
    pid_t pid; /**< app-host pid; 0 on failure */
    int32_t error_code;
    uint32_t degraded; /**< EDGEC_LAUNCH_DEGRADED_* bits actually applied */
} launch_result_t;

/**
 * Spawn app-host through the privileged helper.
 * Returns 0 and fills result->pid on success; returns a negative
 * edge_launch_error_t on failure and fills result->error_code.
 */
int edge_launcher_spawn(const launch_spec_t *spec,
                        const char *const args[],
                        const char *const env[],
                        const int inherit_fds[],
                        launch_result_t *result);

/** Pure validation of spec fields, shared by client and helper. */
int edge_launcher_validate_spec(const launch_spec_t *spec);

/** Reject env entries that would load code into app-host (LD_*, PYTHON*, ...). */
int edge_launcher_env_entry_valid(const char *entry);

/** Derive `<apps_root>/<app_id>/app-host`; fails on a bad app_id or overflow. */
int edge_launcher_derive_app_host(const char *apps_root, const char *app_id,
                                  char *out, size_t out_size);

const char *edge_launcher_strerror(int code);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_LAUNCHER_PORT_H */
