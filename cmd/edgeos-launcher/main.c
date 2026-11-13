#define _GNU_SOURCE

#include "edge_launcher_port.h"

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sched.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#define LAUNCH_REPORT_MAGIC 0x454C4E43u
#define LAUNCH_WIRE_BUFFER (EDGEC_LAUNCH_WIRE_MAX + 1u)

typedef struct {
    uint32_t magic;
    int32_t error_code;
    uint32_t degraded;
    uint32_t reserved;
} launch_report_t;

_Static_assert(sizeof(launch_report_t) == 16, "launch report is wire-stable");

static uint8_t g_wire[LAUNCH_WIRE_BUFFER];
static uint32_t g_degraded;

static int report(int fd, int error_code) {
    launch_report_t rep = {LAUNCH_REPORT_MAGIC, error_code, g_degraded, 0};
    return send(fd, &rep, sizeof(rep), MSG_NOSIGNAL) == (ssize_t)sizeof(rep) ? 0 : -1;
}

static void degrade(uint32_t bit, const char *step) {
    g_degraded |= bit;
    fprintf(stderr, "edgeos-launcher: degraded %s: %s\n", step, strerror(errno));
}

static int parse_wire(const uint8_t *buf, size_t len, launch_spec_t *spec,
                      char **args, char **env) {
    if (len < sizeof(*spec)) return EDGEC_LAUNCH_ERR_PROTOCOL;
    memcpy(spec, buf, sizeof(*spec));
    int rc = edge_launcher_validate_spec(spec);
    if (rc != EDGEC_LAUNCH_OK) return rc;

    size_t off = sizeof(*spec);
    for (uint32_t i = 0; i < spec->argv_count; i++) {
        if (off >= len) return EDGEC_LAUNCH_ERR_PROTOCOL;
        args[i] = (char *)(uintptr_t)(buf + off);
        size_t room = len - off;
        size_t slen = strnlen(args[i], room);
        if (slen == room || slen > EDGEC_LAUNCH_ARG_BYTES_MAX) return EDGEC_LAUNCH_ERR_PROTOCOL;
        off += slen + 1;
    }
    for (uint32_t i = 0; i < spec->env_count; i++) {
        if (off >= len) return EDGEC_LAUNCH_ERR_PROTOCOL;
        env[i] = (char *)(uintptr_t)(buf + off);
        size_t room = len - off;
        size_t slen = strnlen(env[i], room);
        if (slen == room || slen > EDGEC_LAUNCH_ARG_BYTES_MAX) return EDGEC_LAUNCH_ERR_PROTOCOL;
        if (edge_launcher_env_entry_valid(env[i]) != EDGEC_LAUNCH_OK) {
            return EDGEC_LAUNCH_ERR_ARG;
        }
        off += slen + 1;
    }
    return off == len ? EDGEC_LAUNCH_OK : EDGEC_LAUNCH_ERR_PROTOCOL;
}

static const char *apps_root(void) {
    const char *root = getenv("EDGEC_APPS_ROOT");
    return (root && root[0] == '/') ? root : "/var/edgeos/apps";
}

/* Fixed app_id -> app-host path. The caller never supplies an executable path. */
static int resolve_target(const launch_spec_t *spec, char *out, size_t out_size, int *out_fd) {
    char root_real[PATH_MAX];
    if (!realpath(apps_root(), root_real)) return EDGEC_LAUNCH_ERR_PATH;

    char target[PATH_MAX];
    if (edge_launcher_derive_app_host(root_real, spec->app_id, target, sizeof(target)) !=
        EDGEC_LAUNCH_OK) {
        return EDGEC_LAUNCH_ERR_PATH;
    }
    char real[PATH_MAX];
    if (!realpath(target, real)) return EDGEC_LAUNCH_ERR_PATH;

    char expected[PATH_MAX];
    int n = snprintf(expected, sizeof(expected), "%s/%s/app-host", root_real, spec->app_id);
    if (n < 0 || (size_t)n >= sizeof(expected) || strcmp(real, expected) != 0) {
        return EDGEC_LAUNCH_ERR_PATH;
    }

    /* shortcut: realpath+open races only if the app dir becomes writable; app
     * packages stay root-owned and read-only, so revisit with openat2 if that changes. */
    int fd = open(real, O_PATH | O_CLOEXEC);
    if (fd < 0) return EDGEC_LAUNCH_ERR_PATH;
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || (st.st_mode & (S_ISUID | S_ISGID))) {
        close(fd);
        return EDGEC_LAUNCH_ERR_PATH;
    }
    n = snprintf(out, out_size, "%s", real);
    if (n < 0 || (size_t)n >= out_size) {
        close(fd);
        return EDGEC_LAUNCH_ERR_PATH;
    }
    *out_fd = fd;
    return EDGEC_LAUNCH_OK;
}

static int write_text(const char *dir, const char *file, const char *text) {
    char path[PATH_MAX];
    int n = snprintf(path, sizeof(path), "%s/%s", dir, file);
    if (n < 0 || (size_t)n >= sizeof(path)) return -1;
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) return -1;
    size_t len = strlen(text);
    ssize_t wrote = write(fd, text, len);
    int saved = errno;
    close(fd);
    errno = saved;
    return wrote == (ssize_t)len ? 0 : -1;
}

static int mkdir_p(const char *path) {
    char tmp[PATH_MAX];
    int n = snprintf(tmp, sizeof(tmp), "%s", path);
    if (n < 0 || (size_t)n >= sizeof(tmp)) return -1;
    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
        *p = '/';
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
    return 0;
}

/* cgroup v2 limits; a platform without them is reported as degraded, not fatal. */
static void setup_cgroup(const launch_spec_t *spec) {
    const char *root = getenv("EDGEC_CGROUP_ROOT");
    if (!root || root[0] != '/') root = "/sys/fs/cgroup";

    char leaf[EDGEC_LAUNCH_PATH_MAX];
    if (spec->cgroup_path[0] != '\0') {
        memcpy(leaf, spec->cgroup_path, sizeof(leaf));
    } else {
        int n = snprintf(leaf, sizeof(leaf), "edgeos/%s", spec->app_id);
        if (n < 0 || (size_t)n >= sizeof(leaf)) {
            errno = ENAMETOOLONG;
            degrade(EDGEC_LAUNCH_DEGRADED_CGROUP, "cgroup path");
            return;
        }
    }

    char path[PATH_MAX];
    int n = snprintf(path, sizeof(path), "%s/%s", root, leaf);
    if (n < 0 || (size_t)n >= sizeof(path)) {
        errno = ENAMETOOLONG;
        degrade(EDGEC_LAUNCH_DEGRADED_CGROUP, "cgroup path");
        return;
    }
    if (mkdir_p(path) != 0) {
        degrade(EDGEC_LAUNCH_DEGRADED_CGROUP, "cgroup mkdir");
        return;
    }

    char value[32];
    snprintf(value, sizeof(value), "%llu\n", (unsigned long long)spec->mem_max_bytes);
    if (write_text(path, "memory.max", value) != 0) {
        degrade(EDGEC_LAUNCH_DEGRADED_CGROUP, "memory.max");
        return;
    }
    snprintf(value, sizeof(value), "%u\n", spec->pids_max);
    if (write_text(path, "pids.max", value) != 0) {
        degrade(EDGEC_LAUNCH_DEGRADED_CGROUP, "pids.max");
        return;
    }
    if (write_text(path, "memory.oom.group", "1\n") != 0) {
        degrade(EDGEC_LAUNCH_DEGRADED_CGROUP, "memory.oom.group");
        return;
    }
    snprintf(value, sizeof(value), "%d\n", (int)getpid());
    if (write_text(path, "cgroup.procs", value) != 0) {
        degrade(EDGEC_LAUNCH_DEGRADED_CGROUP, "cgroup.procs");
    }
}

/* Read-only root plus a writable data dir, inside a private mount namespace. */
static void setup_mounts(const launch_spec_t *spec) {
    struct stat st;
    if (spec->data_dir[0] != '\0') {
        if (stat(spec->data_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
            errno = ENOENT;
            degrade(EDGEC_LAUNCH_DEGRADED_MOUNT, "data dir");
            return;
        }
    }
    if (unshare(CLONE_NEWNS) != 0) {
        degrade(EDGEC_LAUNCH_DEGRADED_MOUNT, "mount namespace");
        return;
    }
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
        degrade(EDGEC_LAUNCH_DEGRADED_MOUNT, "mount propagation");
        return;
    }
    if (mount(NULL, "/", NULL, MS_BIND | MS_REMOUNT | MS_RDONLY, NULL) != 0) {
        degrade(EDGEC_LAUNCH_DEGRADED_MOUNT, "read-only root");
        return;
    }
    if (spec->data_dir[0] != '\0') {
        if (mount(spec->data_dir, spec->data_dir, NULL, MS_BIND, NULL) != 0) {
            degrade(EDGEC_LAUNCH_DEGRADED_MOUNT, "data bind");
            return;
        }
        if (mount(NULL, spec->data_dir, NULL, MS_BIND | MS_REMOUNT, NULL) != 0) {
            degrade(EDGEC_LAUNCH_DEGRADED_MOUNT, "data read-write");
        }
    }
}

static int drop_privileges(const launch_spec_t *spec) {
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) return EDGEC_LAUNCH_ERR_SYSTEM;
    if (geteuid() != 0) {
        if (spec->uid != (uint32_t)geteuid() || spec->gid != (uint32_t)getegid()) {
            return EDGEC_LAUNCH_ERR_PRIVILEGE;
        }
        return EDGEC_LAUNCH_OK;
    }
    if (setgroups(0, NULL) != 0) return EDGEC_LAUNCH_ERR_SYSTEM;
    if (setgid((gid_t)spec->gid) != 0 || setuid((uid_t)spec->uid) != 0) {
        return EDGEC_LAUNCH_ERR_SYSTEM;
    }
    if (getuid() != (uid_t)spec->uid || geteuid() != (uid_t)spec->uid ||
        getgid() != (gid_t)spec->gid || getegid() != (gid_t)spec->gid) {
        return EDGEC_LAUNCH_ERR_SYSTEM;
    }
    return EDGEC_LAUNCH_OK;
}

#if defined(__x86_64__)
#define EDGE_AUDIT_ARCH AUDIT_ARCH_X86_64
#elif defined(__aarch64__)
#define EDGE_AUDIT_ARCH AUDIT_ARCH_AARCH64
#endif

#define ALLOW_SYSCALL(nr)                                                                    \
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (uint32_t)(nr), 0, 1),                               \
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)

/*
 * Minimal allowlist for app-host: memory, clocks, fds, threads and the Host
 * ABI channel. socket/mount/ptrace/bpf/keyctl are absent, so they return EPERM.
 * execve/execveat stay allowed because the filter must permit the launcher's
 * own fexecve; app-host runs unprivileged after that point.
 * clone is allowed for worker threads, but namespace-creating flags are denied
 * so an unprivileged user namespace cannot undo the mount sandbox.
 */
static int install_seccomp_minimal(void) {
#ifdef EDGE_AUDIT_ARCH
    static const struct sock_filter filter[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (uint32_t)offsetof(struct seccomp_data, arch)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (uint32_t)EDGE_AUDIT_ARCH, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA)),
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (uint32_t)offsetof(struct seccomp_data, nr)),

        /* clone: reject CLONE_NEWUSER/CLONE_NEWNS/CLONE_NEWNET/CLONE_NEWPID/... */
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (uint32_t)__NR_clone, 0, 4),
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (uint32_t)offsetof(struct seccomp_data, args[0])),
        BPF_STMT(BPF_ALU | BPF_AND | BPF_K,
                 (uint32_t)(CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWNET | CLONE_NEWPID |
                            CLONE_NEWIPC | CLONE_NEWUTS | CLONE_NEWCGROUP)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA)),

        ALLOW_SYSCALL(__NR_read),
        ALLOW_SYSCALL(__NR_write),
        ALLOW_SYSCALL(__NR_readv),
        ALLOW_SYSCALL(__NR_writev),
        ALLOW_SYSCALL(__NR_pread64),
        ALLOW_SYSCALL(__NR_pwrite64),
        ALLOW_SYSCALL(__NR_close),
        ALLOW_SYSCALL(__NR_lseek),
        ALLOW_SYSCALL(__NR_fstat),
        ALLOW_SYSCALL(__NR_newfstatat),
        ALLOW_SYSCALL(__NR_statx),
#ifdef __NR_access
        ALLOW_SYSCALL(__NR_access),
#endif
        ALLOW_SYSCALL(__NR_faccessat),
#ifdef __NR_faccessat2
        ALLOW_SYSCALL(__NR_faccessat2),
#endif
#ifdef __NR_open
        ALLOW_SYSCALL(__NR_open),
#endif
        ALLOW_SYSCALL(__NR_openat),
#ifdef __NR_openat2
        ALLOW_SYSCALL(__NR_openat2),
#endif
        ALLOW_SYSCALL(__NR_getcwd),
        ALLOW_SYSCALL(__NR_getdents64),
        ALLOW_SYSCALL(__NR_ioctl),
        ALLOW_SYSCALL(__NR_fcntl),
        ALLOW_SYSCALL(__NR_dup),
        ALLOW_SYSCALL(__NR_dup3),
        ALLOW_SYSCALL(__NR_pipe2),
        ALLOW_SYSCALL(__NR_mmap),
        ALLOW_SYSCALL(__NR_mprotect),
        ALLOW_SYSCALL(__NR_munmap),
        ALLOW_SYSCALL(__NR_mremap),
        ALLOW_SYSCALL(__NR_brk),
        ALLOW_SYSCALL(__NR_madvise),
        ALLOW_SYSCALL(__NR_mlock),
        ALLOW_SYSCALL(__NR_munlock),
        ALLOW_SYSCALL(__NR_membarrier),
        ALLOW_SYSCALL(__NR_memfd_create),
        ALLOW_SYSCALL(__NR_fadvise64),
        ALLOW_SYSCALL(__NR_rt_sigaction),
        ALLOW_SYSCALL(__NR_rt_sigprocmask),
        ALLOW_SYSCALL(__NR_rt_sigreturn),
        ALLOW_SYSCALL(__NR_rt_sigtimedwait),
        ALLOW_SYSCALL(__NR_sigaltstack),
        ALLOW_SYSCALL(__NR_set_robust_list),
        ALLOW_SYSCALL(__NR_get_robust_list),
        ALLOW_SYSCALL(__NR_rseq),
        ALLOW_SYSCALL(__NR_set_tid_address),
        ALLOW_SYSCALL(__NR_futex),
        ALLOW_SYSCALL(__NR_sched_yield),
        ALLOW_SYSCALL(__NR_sched_getaffinity),
#ifdef __NR_clone3
        /* clone3 takes flags through a pointer that BPF cannot inspect; deny. */
#endif
        ALLOW_SYSCALL(__NR_clone),
        ALLOW_SYSCALL(__NR_execve),
        ALLOW_SYSCALL(__NR_execveat),
        ALLOW_SYSCALL(__NR_clock_gettime),
        ALLOW_SYSCALL(__NR_clock_nanosleep),
        ALLOW_SYSCALL(__NR_nanosleep),
        ALLOW_SYSCALL(__NR_gettimeofday),
        ALLOW_SYSCALL(__NR_getrandom),
        ALLOW_SYSCALL(__NR_uname),
        ALLOW_SYSCALL(__NR_sysinfo),
        ALLOW_SYSCALL(__NR_statfs),
        ALLOW_SYSCALL(__NR_fstatfs),
        ALLOW_SYSCALL(__NR_getpid),
        ALLOW_SYSCALL(__NR_gettid),
        ALLOW_SYSCALL(__NR_getuid),
        ALLOW_SYSCALL(__NR_geteuid),
        ALLOW_SYSCALL(__NR_getgid),
        ALLOW_SYSCALL(__NR_getegid),
        ALLOW_SYSCALL(__NR_getppid),
        ALLOW_SYSCALL(__NR_tgkill),
        ALLOW_SYSCALL(__NR_exit),
        ALLOW_SYSCALL(__NR_exit_group),
        ALLOW_SYSCALL(__NR_prlimit64),
        ALLOW_SYSCALL(__NR_getrlimit),
        ALLOW_SYSCALL(__NR_epoll_create1),
        ALLOW_SYSCALL(__NR_epoll_ctl),
        ALLOW_SYSCALL(__NR_epoll_pwait),
        ALLOW_SYSCALL(__NR_eventfd2),
        ALLOW_SYSCALL(__NR_timerfd_create),
        ALLOW_SYSCALL(__NR_timerfd_settime),
        ALLOW_SYSCALL(__NR_timerfd_gettime),
        ALLOW_SYSCALL(__NR_ppoll),
        ALLOW_SYSCALL(__NR_pselect6),
#ifdef __NR_poll
        ALLOW_SYSCALL(__NR_poll),
#endif
        ALLOW_SYSCALL(__NR_sendto),
        ALLOW_SYSCALL(__NR_recvfrom),
        ALLOW_SYSCALL(__NR_sendmsg),
        ALLOW_SYSCALL(__NR_recvmsg),
#ifdef __NR_arch_prctl
        ALLOW_SYSCALL(__NR_arch_prctl),
#endif
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA)),
    };
    struct sock_fprog prog = {
        .len = (unsigned short)(sizeof(filter) / sizeof(filter[0])),
        .filter = (struct sock_filter *)(uintptr_t)filter,
    };
    return prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
#else
    errno = ENOSYS;
    return -1;
#endif
}

static void close_unneeded_fds(int status_fd, int target_fd, uint32_t fd_count) {
    long max_fd = sysconf(_SC_OPEN_MAX);
    if (max_fd < 0 || max_fd > 65536) max_fd = 65536;
    for (int fd = 3; fd < (int)max_fd; fd++) {
        if (fd == status_fd || fd == target_fd) continue;
        if (fd >= EDGEC_LAUNCH_FD_BASE && fd < EDGEC_LAUNCH_FD_BASE + (int)fd_count) continue;
        close(fd);
    }
}

/* The socketpair is created by the product process, so its peer credentials
 * identify the caller even if a setuid install lets another user exec us. */
static int caller_is_root(int fd) {
    struct ucred cred;
    socklen_t len = sizeof(cred);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) != 0) return 0;
    return cred.uid == 0;
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    int status_fd = atoi(argv[1]);
    if (status_fd < 3 || fcntl(status_fd, F_GETFD) < 0) return 2;

    ssize_t n = recv(status_fd, g_wire, sizeof(g_wire), 0);
    if (n <= 0 || (size_t)n > EDGEC_LAUNCH_WIRE_MAX) {
        report(status_fd, EDGEC_LAUNCH_ERR_PROTOCOL);
        return 1;
    }

    launch_spec_t spec;
    char *args[EDGEC_LAUNCH_ARG_MAX];
    char *env[EDGEC_LAUNCH_ARG_MAX];
    memset(args, 0, sizeof(args));
    memset(env, 0, sizeof(env));
    int rc = parse_wire(g_wire, (size_t)n, &spec, args, env);
    if (rc != EDGEC_LAUNCH_OK) {
        report(status_fd, rc);
        return 1;
    }

    char target[PATH_MAX];
    int target_fd = -1;
    rc = resolve_target(&spec, target, sizeof(target), &target_fd);
    if (rc != EDGEC_LAUNCH_OK) {
        report(status_fd, rc);
        return 1;
    }

    (void)prctl(PR_SET_PDEATHSIG, SIGKILL);
    if (!caller_is_root(status_fd)) {
        report(status_fd, EDGEC_LAUNCH_ERR_PRIVILEGE);
        return 1;
    }
    /* cgroup first: the read-only root below must not hide a test/root fs cgroup. */
    setup_cgroup(&spec);
    setup_mounts(&spec);

    rc = drop_privileges(&spec);
    if (rc != EDGEC_LAUNCH_OK) {
        report(status_fd, rc);
        return 1;
    }
    /* setuid() clears PDEATHSIG; set it again so the product stays the supervisor. */
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0) {
        report(status_fd, EDGEC_LAUNCH_ERR_SYSTEM);
        return 1;
    }

    if (spec.seccomp_profile == EDGEC_LAUNCH_SECCOMP_MINIMAL) {
        if (install_seccomp_minimal() != 0) {
            degrade(EDGEC_LAUNCH_DEGRADED_SECCOMP, "seccomp");
        }
    }

    (void)fcntl(status_fd, F_SETFD, FD_CLOEXEC);
    close_unneeded_fds(status_fd, target_fd, spec.fd_count);

    char *exec_argv[EDGEC_LAUNCH_ARG_MAX + 2];
    exec_argv[0] = (char *)"app-host";
    for (uint32_t i = 0; i < spec.argv_count; i++) exec_argv[i + 1] = args[i];
    exec_argv[spec.argv_count + 1] = NULL;

    char *exec_env[EDGEC_LAUNCH_ARG_MAX + 4];
    uint32_t env_count = 0;
    for (uint32_t i = 0; i < spec.env_count; i++) exec_env[env_count++] = env[i];
    exec_env[env_count++] = (char *)"PATH=/usr/bin:/bin";
    char home[EDGEC_LAUNCH_PATH_MAX + 8];
    char tmpdir[EDGEC_LAUNCH_PATH_MAX + 8];
    if (spec.data_dir[0] != '\0') {
        snprintf(home, sizeof(home), "HOME=%s", spec.data_dir);
        snprintf(tmpdir, sizeof(tmpdir), "TMPDIR=%s", spec.data_dir);
        exec_env[env_count++] = home;
        exec_env[env_count++] = tmpdir;
    }
    exec_env[env_count] = NULL;

    if (report(status_fd, EDGEC_LAUNCH_OK) < 0) return 1;
    fexecve(target_fd, exec_argv, exec_env);
    report(status_fd, EDGEC_LAUNCH_ERR_EXEC);
    return 127;
}
