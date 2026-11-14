#define _GNU_SOURCE

#include <cmocka.h>

#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>

#include "edge_launcher_port.h"

static char g_tmp[PATH_MAX];
static char g_apps_root[PATH_MAX];
static char g_cgroup_root[PATH_MAX];
static char g_apps[2][64] = {"demo", "probe"};
static int g_root;

/* ---- app-host fixture probes ------------------------------------------- */

static int launcher_probe(const char *mode, const char *arg) {
    if (strcmp(mode, "socket") == 0) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd >= 0) {
            close(fd);
            return 3;
        }
        return errno == EPERM ? 0 : 4;
    }
    if (strcmp(mode, "root-ro") == 0) {
        struct statvfs st;
        if (statvfs("/", &st) != 0) return 5;
        if ((st.f_flag & ST_RDONLY) == 0) return 4;
        char path[64];
        snprintf(path, sizeof(path), "/tmp/edgec-launcher-probe-%d", (int)getpid());
        int fd = open(path, O_WRONLY | O_CREAT, 0644);
        if (fd >= 0) {
            close(fd);
            return 4;
        }
        return errno == EROFS ? 0 : 6;
    }
    if (strcmp(mode, "sleep") == 0) {
        sleep(arg ? (unsigned)atoi(arg) : 30u);
        return 0;
    }
    return 6;
}

/* ---- fixture helpers ---------------------------------------------------- */

static int copy_file(const char *src, const char *dst) {
    int in = open(src, O_RDONLY);
    if (in < 0) return -1;
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (out < 0) {
        close(in);
        return -1;
    }
    char buf[8192];
    for (;;) {
        ssize_t n = read(in, buf, sizeof(buf));
        if (n < 0) {
            close(in);
            close(out);
            return -1;
        }
        if (n == 0) break;
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(out, buf + off, (size_t)(n - off));
            if (w <= 0) {
                close(in);
                close(out);
                return -1;
            }
            off += w;
        }
    }
    fchmod(out, 0755);
    close(in);
    close(out);
    return 0;
}

static int read_text(const char *path, char *buf, size_t cap) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, buf, cap - 1);
    close(fd);
    if (n < 0) return -1;
    buf[n] = '\0';
    return 0;
}

static int touch_empty(const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    close(fd);
    return 0;
}

static int make_cgroup_leaf(const char *app_id) {
    char dir[PATH_MAX];
    snprintf(dir, sizeof(dir), "%s/edgeos/%s", g_cgroup_root, app_id);
    char parent[PATH_MAX];
    snprintf(parent, sizeof(parent), "%s/edgeos", g_cgroup_root);
    mkdir(parent, 0755);
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) return -1;
    static const char *files[] = {"cgroup.procs", "memory.max", "pids.max", "memory.oom.group"};
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir, files[i]);
        if (touch_empty(path) != 0) return -1;
    }
    return 0;
}

static int remove_entry(const char *path, const struct stat *sb, int flag, struct FTW *ftw) {
    (void)sb;
    (void)flag;
    (void)ftw;
    return remove(path);
}

static int setup(void **state) {
    (void)state;
    snprintf(g_tmp, sizeof(g_tmp), "/tmp/edgec-launcher-XXXXXX");
    if (!mkdtemp(g_tmp)) return -1;
    chmod(g_tmp, 0755);

    snprintf(g_apps_root, sizeof(g_apps_root), "%s/apps", g_tmp);
    snprintf(g_cgroup_root, sizeof(g_cgroup_root), "%s/cgroup", g_tmp);
    mkdir(g_apps_root, 0755);
    mkdir(g_cgroup_root, 0755);
    if (setenv("EDGEC_APPS_ROOT", g_apps_root, 1) != 0) return -1;
    if (setenv("EDGEC_CGROUP_ROOT", g_cgroup_root, 1) != 0) return -1;

    char helper[PATH_MAX];
    snprintf(helper, sizeof(helper), "%s/edgeos-launcher", g_tmp);
    const char *env_helper = getenv("EDGEC_LAUNCHER_PATH");
    if (!env_helper || env_helper[0] == '\0') {
        char exe[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
        if (len <= 0) return -1;
        exe[len] = '\0';
        char *slash = strrchr(exe, '/');
        if (!slash) return -1;
        *slash = '\0';
        snprintf(helper, sizeof(helper), "%s/../cmd/edgeos-launcher/edgeos-launcher", exe);
        if (setenv("EDGEC_LAUNCHER_PATH", helper, 1) != 0) return -1;
    }
    if (access(getenv("EDGEC_LAUNCHER_PATH"), X_OK) != 0) {
        fprintf(stderr, "launcher helper not found: %s\n", getenv("EDGEC_LAUNCHER_PATH"));
        return -1;
    }

    char exe[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len <= 0) return -1;
    exe[len] = '\0';
    char sleep_bin[PATH_MAX];
    snprintf(sleep_bin, sizeof(sleep_bin), "%s", access("/bin/sleep", X_OK) == 0 ? "/bin/sleep"
                                                                               : "/usr/bin/sleep");

    for (size_t i = 0; i < sizeof(g_apps) / sizeof(g_apps[0]); i++) {
        char dir[PATH_MAX];
        snprintf(dir, sizeof(dir), "%s/%s", g_apps_root, g_apps[i]);
        if (mkdir(dir, 0755) != 0 && errno != EEXIST) return -1;
        char target[PATH_MAX];
        snprintf(target, sizeof(target), "%s/app-host", dir);
        if (copy_file(strcmp(g_apps[i], "demo") == 0 ? sleep_bin : exe, target) != 0) return -1;
        if (make_cgroup_leaf(g_apps[i]) != 0) return -1;
    }

    char evil[PATH_MAX];
    snprintf(evil, sizeof(evil), "%s/evil", g_apps_root);
    mkdir(evil, 0755);
    char link[PATH_MAX];
    snprintf(link, sizeof(link), "%s/app-host", evil);
    if (symlink("/bin/true", link) != 0) return -1;

    char cross[PATH_MAX];
    snprintf(cross, sizeof(cross), "%s/cross", g_apps_root);
    mkdir(cross, 0755);
    char cross_link[PATH_MAX];
    snprintf(cross_link, sizeof(cross_link), "%s/app-host", cross);
    char demo_target[PATH_MAX];
    snprintf(demo_target, sizeof(demo_target), "%s/demo/app-host", g_apps_root);
    if (symlink(demo_target, cross_link) != 0) return -1;

    g_root = geteuid() == 0;
    return 0;
}

static int teardown(void **state) {
    (void)state;
    if (g_tmp[0] != '\0') nftw(g_tmp, remove_entry, 16, FTW_DEPTH | FTW_PHYS);
    return 0;
}

/* ---- tests -------------------------------------------------------------- */

static launch_spec_t base_spec(const char *app_id) {
    launch_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    snprintf(spec.app_id, sizeof(spec.app_id), "%s", app_id);
    spec.uid = EDGEC_LAUNCH_UID_MIN;
    spec.gid = EDGEC_LAUNCH_UID_MIN;
    spec.seccomp_profile = EDGEC_LAUNCH_SECCOMP_OFF;
    spec.pids_max = 8;
    spec.mem_max_bytes = 4u * 1024u * 1024u;
    snprintf(spec.cgroup_path, sizeof(spec.cgroup_path), "edgeos/%s", app_id);
    return spec;
}

static void test_rejects_bad_app_id(void **state) {
    (void)state;
    static const char *bad[] = {"../etc", "a/b", "", "..", ".", "a..b", "bad id"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        launch_spec_t spec = base_spec("demo");
        snprintf(spec.app_id, sizeof(spec.app_id), "%s", bad[i]);
        launch_result_t result;
        assert_int_equal(edge_launcher_spawn(&spec, NULL, NULL, NULL, &result),
                         EDGEC_LAUNCH_ERR_APP_ID);
        assert_int_equal(result.error_code, EDGEC_LAUNCH_ERR_APP_ID);
        assert_int_equal(result.pid, 0);
    }
}

static void test_rejects_uid_outside_pool(void **state) {
    (void)state;
    launch_spec_t spec = base_spec("demo");
    launch_result_t result;
    spec.uid = EDGEC_LAUNCH_UID_MIN - 1;
    assert_int_equal(edge_launcher_spawn(&spec, NULL, NULL, NULL, &result), EDGEC_LAUNCH_ERR_UID);
    spec = base_spec("demo");
    spec.gid = EDGEC_LAUNCH_UID_MAX + 1;
    assert_int_equal(edge_launcher_spawn(&spec, NULL, NULL, NULL, &result), EDGEC_LAUNCH_ERR_UID);
}

static void test_rejects_dangerous_env(void **state) {
    (void)state;
    assert_int_equal(edge_launcher_env_entry_valid("LD_PRELOAD=/tmp/x.so"), EDGEC_LAUNCH_ERR_ARG);
    assert_int_equal(edge_launcher_env_entry_valid("PATH=/tmp"), EDGEC_LAUNCH_ERR_ARG);
    assert_int_equal(edge_launcher_env_entry_valid("PYTHONPATH=/tmp"), EDGEC_LAUNCH_ERR_ARG);
    assert_int_equal(edge_launcher_env_entry_valid("NOEQ"), EDGEC_LAUNCH_ERR_ARG);
    assert_int_equal(edge_launcher_env_entry_valid("1BAD=x"), EDGEC_LAUNCH_ERR_ARG);
    assert_int_equal(edge_launcher_env_entry_valid("APP_MODE=fast"), EDGEC_LAUNCH_OK);
}

static void test_rejects_missing_and_escaped_target(void **state) {
    (void)state;
    launch_result_t result;
    launch_spec_t spec = base_spec("missing");
    assert_int_equal(edge_launcher_spawn(&spec, NULL, NULL, NULL, &result), EDGEC_LAUNCH_ERR_PATH);

    /* <apps>/evil/app-host is a symlink outside the app root. */
    spec = base_spec("evil");
    assert_int_equal(edge_launcher_spawn(&spec, NULL, NULL, NULL, &result), EDGEC_LAUNCH_ERR_PATH);
    assert_int_equal(result.pid, 0);

    /* Another app's app-host is still the wrong fixed path. */
    spec = base_spec("cross");
    assert_int_equal(edge_launcher_spawn(&spec, NULL, NULL, NULL, &result), EDGEC_LAUNCH_ERR_PATH);
}

static void test_nonroot_caller_refused(void **state) {
    (void)state;
    if (g_root) {
        skip();
        return;
    }
    launch_spec_t spec = base_spec("probe");
    launch_result_t result;
    assert_int_equal(edge_launcher_spawn(&spec, NULL, NULL, NULL, &result),
                     EDGEC_LAUNCH_ERR_PRIVILEGE);
}

static void test_derives_fixed_target(void **state) {
    (void)state;
    char path[PATH_MAX];
    assert_int_equal(edge_launcher_derive_app_host("/var/edgeos/apps", "demo-app", path, sizeof(path)),
                     EDGEC_LAUNCH_OK);
    assert_string_equal(path, "/var/edgeos/apps/demo-app/app-host");
    assert_int_equal(edge_launcher_derive_app_host("/var/edgeos/apps", "../x", path, sizeof(path)),
                     EDGEC_LAUNCH_ERR_APP_ID);
}

static void test_sandbox_runtime_flags(void **state) {
    (void)state;
    if (!g_root) {
        skip();
        return;
    }
    launch_spec_t spec = base_spec("probe");
    spec.seccomp_profile = EDGEC_LAUNCH_SECCOMP_MINIMAL;
    static const char *args[] = {"--launcher-probe", "sleep", "30"};
    spec.argv_count = 3;
    launch_result_t result;
    assert_int_equal(edge_launcher_spawn(&spec, args, NULL, NULL, &result), EDGEC_LAUNCH_OK);
    assert_true(result.pid > 0);

    char status_path[64];
    snprintf(status_path, sizeof(status_path), "/proc/%d/status", (int)result.pid);
    char status[8192];
    assert_int_equal(read_text(status_path, status, sizeof(status)), 0);

    char *uid_line = strstr(status, "\nUid:");
    assert_non_null(uid_line);
    unsigned ru = 0, eu = 0, su = 0, fu = 0;
    assert_int_equal(sscanf(uid_line + 1, "Uid: %u %u %u %u", &ru, &eu, &su, &fu), 4);
    assert_int_equal(ru, EDGEC_LAUNCH_UID_MIN);
    assert_int_equal(eu, EDGEC_LAUNCH_UID_MIN);

    char *nnp_line = strstr(status, "\nNoNewPrivs:");
    assert_non_null(nnp_line);
    int nnp = 0;
    assert_int_equal(sscanf(nnp_line + 1, "NoNewPrivs: %d", &nnp), 1);
    assert_int_equal(nnp, 1);

    if ((result.degraded & EDGEC_LAUNCH_DEGRADED_SECCOMP) == 0) {
        char *seccomp_line = strstr(status, "\nSeccomp:");
        assert_non_null(seccomp_line);
        int seccomp = 0;
        assert_int_equal(sscanf(seccomp_line + 1, "Seccomp: %d", &seccomp), 1);
        assert_int_equal(seccomp, 2);
    }

    char cg_path[PATH_MAX];
    char cg_value[64];
    snprintf(cg_path, sizeof(cg_path), "%s/edgeos/probe/memory.max", g_cgroup_root);
    assert_int_equal(read_text(cg_path, cg_value, sizeof(cg_value)), 0);
    assert_int_equal(atoi(cg_value), (int)spec.mem_max_bytes);
    snprintf(cg_path, sizeof(cg_path), "%s/edgeos/probe/pids.max", g_cgroup_root);
    assert_int_equal(read_text(cg_path, cg_value, sizeof(cg_value)), 0);
    assert_int_equal(atoi(cg_value), (int)spec.pids_max);
    snprintf(cg_path, sizeof(cg_path), "%s/edgeos/probe/cgroup.procs", g_cgroup_root);
    assert_int_equal(read_text(cg_path, cg_value, sizeof(cg_value)), 0);
    assert_int_equal(atoi(cg_value), (int)result.pid);
    assert_int_equal(result.degraded & EDGEC_LAUNCH_DEGRADED_CGROUP, 0);

    kill(result.pid, SIGKILL);
    waitpid(result.pid, NULL, 0);
}

static int run_probe(const char *mode, launch_result_t *result) {
    launch_spec_t spec = base_spec("probe");
    spec.seccomp_profile = EDGEC_LAUNCH_SECCOMP_MINIMAL;
    const char *args[] = {"--launcher-probe", mode};
    spec.argv_count = 2;
    if (edge_launcher_spawn(&spec, args, NULL, NULL, result) != EDGEC_LAUNCH_OK) return -1;
    int status = 0;
    if (waitpid(result->pid, &status, 0) != result->pid) return -1;
    if (!WIFEXITED(status)) return -1;
    return WEXITSTATUS(status);
}

static void test_seccomp_denies_socket(void **state) {
    (void)state;
    if (!g_root) {
        skip();
        return;
    }
    launch_result_t result;
    int rc = run_probe("socket", &result);
    assert_int_equal(rc, 0);
    assert_int_equal(result.degraded & EDGEC_LAUNCH_DEGRADED_SECCOMP, 0);
}

static void test_readonly_root(void **state) {
    (void)state;
    if (!g_root) {
        skip();
        return;
    }
    launch_result_t result;
    int rc = run_probe("root-ro", &result);
    if (result.degraded & EDGEC_LAUNCH_DEGRADED_MOUNT) {
        skip();
        return;
    }
    assert_int_equal(rc, 0);
}

static void test_product_death_kills_app_host(void **state) {
    (void)state;
    if (!g_root) {
        skip();
        return;
    }
    int pipefd[2];
    assert_int_equal(pipe(pipefd), 0);

    pid_t product = fork();
    assert_true(product >= 0);
    if (product == 0) {
        close(pipefd[0]);
        launch_spec_t spec = base_spec("demo");
        spec.seccomp_profile = EDGEC_LAUNCH_SECCOMP_OFF;
        const char *args[] = {"60"};
        spec.argv_count = 1;
        launch_result_t result;
        int rc = edge_launcher_spawn(&spec, args, NULL, NULL, &result);
        int report = rc == EDGEC_LAUNCH_OK ? (int)result.pid : -1;
        ssize_t w = write(pipefd[1], &report, sizeof(report));
        (void)w;
        for (;;) pause();
    }

    close(pipefd[1]);
    int app_host = -1;
    assert_int_equal(read(pipefd[0], &app_host, sizeof(app_host)), (ssize_t)sizeof(app_host));
    close(pipefd[0]);
    assert_true(app_host > 0);

    char status_path[64];
    snprintf(status_path, sizeof(status_path), "/proc/%d", app_host);
    assert_int_equal(access(status_path, F_OK), 0);

    kill(product, SIGKILL);
    waitpid(product, NULL, 0);

    int alive = 1;
    for (int i = 0; i < 200 && alive; i++) {
        if (access(status_path, F_OK) != 0) alive = 0;
        else usleep(10 * 1000);
    }
    if (alive) kill(app_host, SIGKILL);
    assert_int_equal(alive, 0);
}

int main(int argc, char **argv) {
    if (argc >= 3 && strcmp(argv[1], "--launcher-probe") == 0) {
        return launcher_probe(argv[2], argc >= 4 ? argv[3] : NULL);
    }
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_rejects_bad_app_id),
        cmocka_unit_test(test_rejects_uid_outside_pool),
        cmocka_unit_test(test_rejects_dangerous_env),
        cmocka_unit_test(test_rejects_missing_and_escaped_target),
        cmocka_unit_test(test_nonroot_caller_refused),
        cmocka_unit_test(test_derives_fixed_target),
        cmocka_unit_test(test_sandbox_runtime_flags),
        cmocka_unit_test(test_seccomp_denies_socket),
        cmocka_unit_test(test_readonly_root),
        cmocka_unit_test(test_product_death_kills_app_host),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
