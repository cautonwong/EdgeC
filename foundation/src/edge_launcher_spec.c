#include "edge_launcher_port.h"

#include <stdio.h>
#include <string.h>

/* app ids are path components: no '/', no "..", bounded. */
static int valid_app_id(const char *id, size_t cap) {
    if (!id || id[0] == '\0') return 0;
    if (!memchr(id, '\0', cap)) return 0;
    size_t len = strlen(id);
    if (len >= EDGEC_LAUNCH_APP_ID_MAX) return 0;
    if (strcmp(id, ".") == 0) return 0;
    for (size_t i = 0; i < len; i++) {
        char c = id[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.') {
            continue;
        }
        return 0;
    }
    return strstr(id, "..") == NULL;
}

static int no_dotdot_components(const char *path) {
    const char *p = path;
    while (*p) {
        const char *slash = strchr(p, '/');
        size_t len = slash ? (size_t)(slash - p) : strlen(p);
        if (len == 2 && p[0] == '.' && p[1] == '.') return 0;
        if (!slash) break;
        p = slash + 1;
    }
    return 1;
}

static int valid_absolute_path(const char *path, size_t cap) {
    if (!memchr(path, '\0', cap)) return 0;
    if (path[0] != '/' || strlen(path) >= cap) return 0;
    return no_dotdot_components(path);
}

static int valid_relative_path(const char *path, size_t cap) {
    if (!memchr(path, '\0', cap)) return 0;
    size_t len = strlen(path);
    if (len == 0 || len >= cap || path[0] == '/') return 0;
    if (!no_dotdot_components(path)) return 0;
    for (size_t i = 0; i < len; i++) {
        char c = path[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.' || c == '/') {
            continue;
        }
        return 0;
    }
    return strstr(path, "//") == NULL;
}

int edge_launcher_validate_spec(const launch_spec_t *spec) {
    if (!spec) return EDGEC_LAUNCH_ERR_ARG;
    if (!valid_app_id(spec->app_id, sizeof(spec->app_id))) return EDGEC_LAUNCH_ERR_APP_ID;
    if (spec->uid < EDGEC_LAUNCH_UID_MIN || spec->uid > EDGEC_LAUNCH_UID_MAX ||
        spec->gid < EDGEC_LAUNCH_UID_MIN || spec->gid > EDGEC_LAUNCH_UID_MAX) {
        return EDGEC_LAUNCH_ERR_UID;
    }
    if (spec->argv_count > EDGEC_LAUNCH_ARG_MAX || spec->env_count > EDGEC_LAUNCH_ARG_MAX ||
        spec->fd_count > EDGEC_LAUNCH_FD_MAX) {
        return EDGEC_LAUNCH_ERR_ARG;
    }
    if (spec->seccomp_profile > EDGEC_LAUNCH_SECCOMP_MINIMAL) return EDGEC_LAUNCH_ERR_ARG;
    if (spec->mem_max_bytes == 0 || spec->pids_max == 0) return EDGEC_LAUNCH_ERR_ARG;
    if (spec->flags != 0 || spec->reserved != 0) return EDGEC_LAUNCH_ERR_ARG;
    if (spec->cgroup_path[0] != '\0' &&
        !valid_relative_path(spec->cgroup_path, sizeof(spec->cgroup_path))) {
        return EDGEC_LAUNCH_ERR_PATH;
    }
    if (spec->data_dir[0] != '\0' && !valid_absolute_path(spec->data_dir, sizeof(spec->data_dir))) {
        return EDGEC_LAUNCH_ERR_PATH;
    }
    return EDGEC_LAUNCH_OK;
}

int edge_launcher_env_entry_valid(const char *entry) {
    static const char *deny_prefix[] = {
        "LD_", "GLIBC_", "MALLOC_", "PYTHON", "PERL5", "LOCPATH", "NLSPATH",
        "GCONV_PATH", "RESOLV_HOST_CONF", NULL
    };
    static const char *deny_exact[] = {
        "PATH", "HOME", "TMPDIR", "IFS", "ENV", "BASH_ENV", "SHELLOPTS",
        "RUBYOPT", "NODE_OPTIONS", "TZDIR", "HOSTALIASES", NULL
    };
    if (!entry) return EDGEC_LAUNCH_ERR_ARG;
    const char *eq = strchr(entry, '=');
    if (!eq || eq == entry) return EDGEC_LAUNCH_ERR_ARG;
    if (!((entry[0] >= 'A' && entry[0] <= 'Z') || (entry[0] >= 'a' && entry[0] <= 'z') ||
          entry[0] == '_')) {
        return EDGEC_LAUNCH_ERR_ARG;
    }
    for (const char *p = entry; p < eq; p++) {
        char c = *p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '_') {
            continue;
        }
        return EDGEC_LAUNCH_ERR_ARG;
    }
    for (size_t i = 0; deny_prefix[i]; i++) {
        if (strncmp(entry, deny_prefix[i], strlen(deny_prefix[i])) == 0) return EDGEC_LAUNCH_ERR_ARG;
    }
    for (size_t i = 0; deny_exact[i]; i++) {
        if ((size_t)(eq - entry) == strlen(deny_exact[i]) &&
            strncmp(entry, deny_exact[i], (size_t)(eq - entry)) == 0) {
            return EDGEC_LAUNCH_ERR_ARG;
        }
    }
    return EDGEC_LAUNCH_OK;
}

int edge_launcher_derive_app_host(const char *apps_root, const char *app_id,
                                  char *out, size_t out_size) {
    if (!apps_root || !app_id || !out || out_size == 0) return EDGEC_LAUNCH_ERR_ARG;
    if (!valid_app_id(app_id, EDGEC_LAUNCH_APP_ID_MAX)) return EDGEC_LAUNCH_ERR_APP_ID;
    if (apps_root[0] != '/') return EDGEC_LAUNCH_ERR_PATH;
    int n = snprintf(out, out_size, "%s/%s/app-host", apps_root, app_id);
    if (n < 0 || (size_t)n >= out_size) return EDGEC_LAUNCH_ERR_PATH;
    return EDGEC_LAUNCH_OK;
}

const char *edge_launcher_strerror(int code) {
    switch (code) {
    case EDGEC_LAUNCH_OK: return "ok";
    case EDGEC_LAUNCH_ERR_ARG: return "invalid argument";
    case EDGEC_LAUNCH_ERR_APP_ID: return "invalid app_id";
    case EDGEC_LAUNCH_ERR_UID: return "uid/gid outside 20000-29999";
    case EDGEC_LAUNCH_ERR_PATH: return "app path rejected";
    case EDGEC_LAUNCH_ERR_PRIVILEGE: return "insufficient privilege";
    case EDGEC_LAUNCH_ERR_SYSTEM: return "sandbox setup failed";
    case EDGEC_LAUNCH_ERR_EXEC: return "exec failed";
    case EDGEC_LAUNCH_ERR_TIMEOUT: return "launcher timed out";
    case EDGEC_LAUNCH_ERR_PROTOCOL: return "launcher protocol error";
    default: return "unknown launcher error";
    }
}
