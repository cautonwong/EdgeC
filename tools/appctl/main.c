/**
 * @file main.c
 * @brief appctl: local management client for the EdgeC app platform.
 *
 * Speaks the rest_uds_server frame protocol on /tmp/edgeos_appctl.sock
 * (EDGEC_APPCTL_SOCK overrides). Commands map one-to-one to REST routes.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#define CTL_MAGIC "CTL1"
#define CTL_MAX_BODY (8u * 1024u * 1024u)
#define CTL_DEFAULT_SOCK "/tmp/edgeos_appctl.sock"
#define CTL_MAX_INPUT (64u * 1024u * 1024u)

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} sb_t;

static int sb_reserve(sb_t *sb, size_t extra) {
    size_t need = sb->len + extra + 1;
    if (need <= sb->cap) return 0;
    size_t cap = sb->cap ? sb->cap : 256;
    while (cap < need) cap *= 2;
    char *grown = realloc(sb->data, cap);
    if (!grown) return -1;
    sb->data = grown;
    sb->cap = cap;
    return 0;
}

static void sb_add(sb_t *sb, const char *text, size_t len) {
    if (sb_reserve(sb, len) != 0) return;
    memcpy(sb->data + sb->len, text, len);
    sb->len += len;
    sb->data[sb->len] = '\0';
}

static void sb_adds(sb_t *sb, const char *text) {
    sb_add(sb, text, strlen(text));
}

static void sb_addf(sb_t *sb, const char *fmt, ...) {
    char buffer[128];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (n > 0) sb_add(sb, buffer, (size_t)n < sizeof(buffer) ? (size_t)n : sizeof(buffer) - 1);
}

static void sb_add_json_string(sb_t *sb, const char *text) {
    sb_adds(sb, "\"");
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        switch (*p) {
            case '"': sb_adds(sb, "\\\""); break;
            case '\\': sb_adds(sb, "\\\\"); break;
            case '\b': sb_adds(sb, "\\b"); break;
            case '\f': sb_adds(sb, "\\f"); break;
            case '\n': sb_adds(sb, "\\n"); break;
            case '\r': sb_adds(sb, "\\r"); break;
            case '\t': sb_adds(sb, "\\t"); break;
            default:
                if (*p < 0x20) {
                    sb_addf(sb, "\\u%04x", *p);
                } else {
                    sb_add(sb, (const char *)p, 1);
                }
                break;
        }
    }
    sb_adds(sb, "\"");
}

static void sb_add_hex(sb_t *sb, const uint8_t *data, size_t len) {
    static const char digits[] = "0123456789abcdef";
    if (sb_reserve(sb, len * 2) != 0) return;
    for (size_t i = 0; i < len; i++) {
        sb->data[sb->len++] = digits[data[i] >> 4];
        sb->data[sb->len++] = digits[data[i] & 0x0f];
    }
    sb->data[sb->len] = '\0';
}

static int read_file(const char *path, uint8_t **data, size_t *len) {
    *data = NULL;
    *len = 0;
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "appctl: cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long size = ftell(f);
    if (size < 0 || (unsigned long)size > CTL_MAX_INPUT) {
        fprintf(stderr, "appctl: %s is too large\n", path);
        fclose(f);
        return -1;
    }
    rewind(f);
    uint8_t *buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(f);
        return -1;
    }
    size_t got = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    if (got != (size_t)size) {
        fprintf(stderr, "appctl: short read on %s\n", path);
        free(buffer);
        return -1;
    }
    buffer[size] = '\0';
    *data = buffer;
    *len = (size_t)size;
    return 0;
}

/* Public key files hold 32 raw bytes or 64 hex chars with optional whitespace. */
static int load_public_key(const char *path, uint8_t out[32]) {
    uint8_t *data = NULL;
    size_t len = 0;
    if (read_file(path, &data, &len) != 0) return -1;
    if (len == 32) {
        memcpy(out, data, 32);
        free(data);
        return 0;
    }
    while (len > 0 && (data[len - 1] == '\n' || data[len - 1] == '\r' || data[len - 1] == ' ')) {
        len--;
    }
    if (len != 64) {
        fprintf(stderr, "appctl: %s is not a 32-byte or 64-hex-char key\n", path);
        free(data);
        return -1;
    }
    for (size_t i = 0; i < 32; i++) {
        int hi = data[i * 2] >= '0' && data[i * 2] <= '9'
                     ? data[i * 2] - '0'
                     : (data[i * 2] | 0x20) - 'a' + 10;
        int lo = data[i * 2 + 1] >= '0' && data[i * 2 + 1] <= '9'
                     ? data[i * 2 + 1] - '0'
                     : (data[i * 2 + 1] | 0x20) - 'a' + 10;
        if (hi < 0 || hi > 15 || lo < 0 || lo > 15) {
            fprintf(stderr, "appctl: %s has a non-hex character\n", path);
            free(data);
            return -1;
        }
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    free(data);
    return 0;
}

static int write_all(int fd, const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    size_t written = 0;
    while (written < len) {
        ssize_t chunk = write(fd, bytes + written, len - written);
        if (chunk < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        written += (size_t)chunk;
    }
    return 0;
}

static int read_all(int fd, void *data, size_t len) {
    uint8_t *bytes = (uint8_t *)data;
    size_t got = 0;
    while (got < len) {
        ssize_t chunk = read(fd, bytes + got, len - got);
        if (chunk < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (chunk == 0) return -1;
        got += (size_t)chunk;
    }
    return 0;
}

static void put_u32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8) & 0xffu);
    p[2] = (uint8_t)((value >> 16) & 0xffu);
    p[3] = (uint8_t)((value >> 24) & 0xffu);
}

static uint32_t get_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static const char *socket_path(const char *flag) {
    if (flag && flag[0]) return flag;
    const char *env = getenv("EDGEC_APPCTL_SOCK");
    return (env && env[0]) ? env : CTL_DEFAULT_SOCK;
}

static int ctl_call(const char *sock, const char *method, const char *path, const char *body,
                    size_t body_len, int *status, char **response) {
    *status = 0;
    *response = NULL;

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "appctl: socket: %s\n", strerror(errno));
        return -1;
    }
    struct timeval timeout = {.tv_sec = 30, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strlen(sock) >= sizeof(addr.sun_path)) {
        fprintf(stderr, "appctl: socket path is too long\n");
        close(fd);
        return -1;
    }
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "appctl: cannot connect to %s: %s\n", sock, strerror(errno));
        close(fd);
        return -1;
    }

    uint8_t header[16];
    memcpy(header, CTL_MAGIC, 4);
    put_u32(header + 4, (uint32_t)strlen(method));
    put_u32(header + 8, (uint32_t)strlen(path));
    put_u32(header + 12, (uint32_t)body_len);
    int failed = write_all(fd, header, sizeof(header)) != 0 ||
                 write_all(fd, method, strlen(method)) != 0 ||
                 write_all(fd, path, strlen(path)) != 0 ||
                 (body_len && write_all(fd, body, body_len) != 0);
    if (failed) {
        fprintf(stderr, "appctl: send failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    uint8_t reply[12];
    if (read_all(fd, reply, sizeof(reply)) != 0 || memcmp(reply, CTL_MAGIC, 4) != 0) {
        fprintf(stderr, "appctl: malformed response\n");
        close(fd);
        return -1;
    }
    uint32_t status_code = get_u32(reply + 4);
    uint32_t response_len = get_u32(reply + 8);
    if (response_len > CTL_MAX_BODY) {
        fprintf(stderr, "appctl: response too large\n");
        close(fd);
        return -1;
    }
    char *text = malloc((size_t)response_len + 1);
    if (!text) {
        close(fd);
        return -1;
    }
    if (response_len && read_all(fd, text, response_len) != 0) {
        fprintf(stderr, "appctl: truncated response\n");
        free(text);
        close(fd);
        return -1;
    }
    text[response_len] = '\0';
    close(fd);

    *status = (int)status_code;
    *response = text;
    return 0;
}

static int report(int status, char *response) {
    if (response) printf("%s\n", response);
    int ok = status == 200 && (!response || !strstr(response, "\"status\":\"error\""));
    free(response);
    return ok ? 0 : 2;
}

static int usage(void) {
    fprintf(stderr,
            "usage (all commands accept --sock PATH):\n"
            "  appctl install <pkg> [--key PUB] [--idempotency-key ID]\n"
            "  appctl grant <app> <cap> --scope SCOPE_JSON [--purpose P]\n"
            "               [--expires UNIX_TS] [--max-sensitivity N] [--actor A]\n"
            "  appctl revoke <app> <cap>\n"
            "  appctl start|stop|rollback <app>\n"
            "  appctl uninstall <app> [--disposition keep|purge|export] [--export-path FILE]\n"
            "  appctl upgrade <app> <pkg>\n"
            "  appctl status [<app>]\n"
            "  appctl time [--trust]\n"
            "  appctl snapshot\n"
            "  appctl list | grants [--app ID]\n");
    return 1;
}

static int parse_positionals(int argc, char **argv, int max_pos, const char **pos, int *pos_count,
                             const char *const *names, const char **values, int count) {
    int p = 0;
    for (int i = 2; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            if (i + 1 >= argc) return -1;
            int found = -1;
            for (int n = 0; n < count; n++) {
                if (strcmp(argv[i], names[n]) == 0) found = n;
            }
            if (found < 0 || values[found]) return -1;
            values[found] = argv[i + 1];
            i++;
        } else {
            if (p >= max_pos) return -1;
            pos[p++] = argv[i];
        }
    }
    *pos_count = p;
    return 0;
}

static int simple_lifecycle(int argc, char **argv, const char *route, const char *const *names,
                            const char **values, int count, int with_disposition) {
    const char *pos[1] = {0};
    int pos_count = 0;
    if (parse_positionals(argc, argv, 1, pos, &pos_count, names, values, count) != 0 ||
        pos_count != 1) {
        return usage();
    }

    sb_t body = {0};
    sb_adds(&body, "{\"app_id\":");
    sb_add_json_string(&body, pos[0]);
    if (with_disposition) {
        sb_adds(&body, ",\"disposition\":");
        sb_add_json_string(&body, values[0] ? values[0] : "keep");
    }
    sb_adds(&body, "}");

    const char *sock = values[count - 1];
    int status = 0;
    char *response = NULL;
    if (ctl_call(socket_path(sock), "POST", route, body.data, body.len, &status, &response) != 0) {
        free(body.data);
        return 1;
    }
    free(body.data);
    return report(status, response);
}

static int cmd_start(int argc, char **argv) {
    const char *names[] = {"--sock"};
    const char *values[1] = {0};
    return simple_lifecycle(argc, argv, "/api/v1/apps/start", names, values, 1, 0);
}

static int cmd_stop(int argc, char **argv) {
    const char *names[] = {"--sock"};
    const char *values[1] = {0};
    return simple_lifecycle(argc, argv, "/api/v1/apps/stop", names, values, 1, 0);
}

static int cmd_rollback(int argc, char **argv) {
    const char *names[] = {"--sock"};
    const char *values[1] = {0};
    return simple_lifecycle(argc, argv, "/api/v1/apps/rollback", names, values, 1, 0);
}

static int cmd_uninstall(int argc, char **argv) {
    const char *names[] = {"--disposition", "--export-path", "--sock"};
    const char *values[3] = {0};
    const char *pos[1] = {0};
    int pos_count = 0;
    if (parse_positionals(argc, argv, 1, pos, &pos_count, names, values, 3) != 0 ||
        pos_count != 1) {
        return usage();
    }
    sb_t body = {0};
    sb_adds(&body, "{\"app_id\":");
    sb_add_json_string(&body, pos[0]);
    sb_adds(&body, ",\"disposition\":");
    sb_add_json_string(&body, values[0] ? values[0] : "keep");
    if (values[1]) {
        sb_adds(&body, ",\"export_path\":");
        sb_add_json_string(&body, values[1]);
    }
    sb_adds(&body, "}");
    const char *sock = values[2];
    int status = 0;
    char *response = NULL;
    if (ctl_call(socket_path(sock), "POST", "/api/v1/apps/uninstall", body.data, body.len, &status,
                 &response) != 0) {
        free(body.data);
        return 1;
    }
    free(body.data);
    return report(status, response);
}

static int cmd_time(int argc, char **argv) {
    int trust = 0;
    const char *sock = NULL;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--trust") == 0) {
            trust = 1;
        } else if (strcmp(argv[i], "--sock") == 0 && i + 1 < argc) {
            sock = argv[++i];
        } else {
            return usage();
        }
    }
    int status = 0;
    char *response = NULL;
    const char *body = trust ? "{\"epoch_ms\":0}" : NULL;
    if (ctl_call(socket_path(sock), trust ? "POST" : "GET", "/api/v1/system/time", body,
                 body ? strlen(body) : 0, &status, &response) != 0) {
        return 1;
    }
    return report(status, response);
}

static int cmd_install(int argc, char **argv) {
    const char *names[] = {"--key", "--idempotency-key", "--sock"};
    const char *values[3] = {0};
    const char *pos[1] = {0};
    int pos_count = 0;
    if (parse_positionals(argc, argv, 1, pos, &pos_count, names, values, 3) != 0 || pos_count != 1) {
        return usage();
    }

    uint8_t *package = NULL;
    size_t package_len = 0;
    if (read_file(pos[0], &package, &package_len) != 0) return 1;

    uint8_t pubkey[32];
    int have_pubkey = 0;
    if (values[0]) {
        if (load_public_key(values[0], pubkey) != 0) {
            free(package);
            return 1;
        }
        have_pubkey = 1;
    }

    sb_t body = {0};
    sb_adds(&body, "{\"package_hex\":\"");
    sb_add_hex(&body, package, package_len);
    sb_adds(&body, "\"");
    if (have_pubkey) {
        sb_adds(&body, ",\"pubkey_hex\":\"");
        sb_add_hex(&body, pubkey, sizeof(pubkey));
        sb_adds(&body, "\"");
    }
    if (values[1]) {
        sb_adds(&body, ",\"idempotency_key\":");
        sb_add_json_string(&body, values[1]);
    }
    sb_adds(&body, "}");
    free(package);

    int status = 0;
    char *response = NULL;
    if (ctl_call(socket_path(values[2]), "POST", "/api/v1/apps/package", body.data, body.len,
                 &status, &response) != 0) {
        free(body.data);
        return 1;
    }
    free(body.data);
    return report(status, response);
}

static int cmd_upgrade(int argc, char **argv) {
    const char *names[] = {"--idempotency-key", "--sock"};
    const char *values[2] = {0};
    const char *pos[2] = {0};
    int pos_count = 0;
    if (parse_positionals(argc, argv, 2, pos, &pos_count, names, values, 2) != 0 || pos_count != 2) {
        return usage();
    }

    uint8_t *package = NULL;
    size_t package_len = 0;
    if (read_file(pos[1], &package, &package_len) != 0) return 1;

    sb_t body = {0};
    sb_adds(&body, "{\"app_id\":");
    sb_add_json_string(&body, pos[0]);
    sb_adds(&body, ",\"package_hex\":\"");
    sb_add_hex(&body, package, package_len);
    sb_adds(&body, "\"");
    if (values[0]) {
        sb_adds(&body, ",\"idempotency_key\":");
        sb_add_json_string(&body, values[0]);
    }
    sb_adds(&body, "}");
    free(package);

    int status = 0;
    char *response = NULL;
    if (ctl_call(socket_path(values[1]), "POST", "/api/v1/apps/upgrade_package", body.data,
                 body.len, &status, &response) != 0) {
        free(body.data);
        return 1;
    }
    free(body.data);
    return report(status, response);
}

static int cmd_grant(int argc, char **argv) {
    const char *names[] = {"--scope", "--purpose", "--expires", "--max-sensitivity", "--actor",
                           "--sock"};
    const char *values[6] = {0};
    const char *pos[2] = {0};
    int pos_count = 0;
    if (parse_positionals(argc, argv, 2, pos, &pos_count, names, values, 6) != 0 || pos_count != 2 ||
        !values[0]) {
        return usage();
    }

    sb_t body = {0};
    sb_adds(&body, "{\"app_id\":");
    sb_add_json_string(&body, pos[0]);
    sb_adds(&body, ",\"capability\":");
    sb_add_json_string(&body, pos[1]);
    sb_adds(&body, ",\"scope\":");
    sb_add_json_string(&body, values[0]);
    if (values[1]) {
        sb_adds(&body, ",\"purpose\":");
        sb_add_json_string(&body, values[1]);
    }
    if (values[2]) {
        sb_adds(&body, ",\"expires_at\":");
        sb_adds(&body, values[2]);
    }
    if (values[3]) {
        sb_adds(&body, ",\"sensitivity_max\":");
        sb_adds(&body, values[3]);
    }
    if (values[4]) {
        sb_adds(&body, ",\"actor\":");
        sb_add_json_string(&body, values[4]);
    }
    sb_adds(&body, "}");

    int status = 0;
    char *response = NULL;
    if (ctl_call(socket_path(values[5]), "POST", "/api/v1/apps/grant", body.data, body.len, &status,
                 &response) != 0) {
        free(body.data);
        return 1;
    }
    free(body.data);
    return report(status, response);
}

static int cmd_revoke(int argc, char **argv) {
    const char *names[] = {"--sock"};
    const char *values[1] = {0};
    const char *pos[2] = {0};
    int pos_count = 0;
    if (parse_positionals(argc, argv, 2, pos, &pos_count, names, values, 1) != 0 || pos_count != 2) {
        return usage();
    }

    sb_t body = {0};
    sb_adds(&body, "{\"app_id\":");
    sb_add_json_string(&body, pos[0]);
    sb_adds(&body, ",\"capability\":");
    sb_add_json_string(&body, pos[1]);
    sb_adds(&body, "}");

    int status = 0;
    char *response = NULL;
    if (ctl_call(socket_path(values[0]), "POST", "/api/v1/apps/revoke_grant", body.data, body.len,
                 &status, &response) != 0) {
        free(body.data);
        return 1;
    }
    free(body.data);
    return report(status, response);
}

static int cmd_status(int argc, char **argv) {
    const char *names[] = {"--sock"};
    const char *values[1] = {0};
    const char *pos[1] = {0};
    int pos_count = 0;
    if (parse_positionals(argc, argv, 1, pos, &pos_count, names, values, 1) != 0 || pos_count > 1) {
        return usage();
    }

    char route[256];
    if (pos_count == 1) {
        snprintf(route, sizeof(route), "/api/v1/apps/status/%s", pos[0]);
    } else {
        snprintf(route, sizeof(route), "/api/v1/apps/status");
    }
    int status = 0;
    char *response = NULL;
    if (ctl_call(socket_path(values[0]), "GET", route, NULL, 0, &status, &response) != 0) return 1;
    return report(status, response);
}

static int cmd_snapshot(int argc, char **argv) {
    const char *names[] = {"--sock"};
    const char *values[1] = {0};
    if (parse_positionals(argc, argv, 0, NULL, (int[]){0}, names, values, 1) != 0) return usage();
    int status = 0;
    char *response = NULL;
    if (ctl_call(socket_path(values[0]), "POST", "/api/v1/apps/snapshot", "", 0, &status,
                 &response) != 0) {
        return 1;
    }
    return report(status, response);
}

static int cmd_list(int argc, char **argv) {
    const char *names[] = {"--sock"};
    const char *values[1] = {0};
    if (parse_positionals(argc, argv, 0, NULL, (int[]){0}, names, values, 1) != 0) return usage();
    int status = 0;
    char *response = NULL;
    if (ctl_call(socket_path(values[0]), "GET", "/api/v1/apps/list", NULL, 0, &status, &response) !=
        0) {
        return 1;
    }
    return report(status, response);
}

static int cmd_grants(int argc, char **argv) {
    const char *names[] = {"--app", "--sock"};
    const char *values[2] = {0};
    if (parse_positionals(argc, argv, 0, NULL, (int[]){0}, names, values, 2) != 0) return usage();

    sb_t body = {0};
    sb_adds(&body, "{");
    if (values[0]) {
        sb_adds(&body, "\"app_id\":");
        sb_add_json_string(&body, values[0]);
    }
    sb_adds(&body, "}");

    int status = 0;
    char *response = NULL;
    if (ctl_call(socket_path(values[1]), "POST", "/api/v1/apps/grants", body.data, body.len, &status,
                 &response) != 0) {
        free(body.data);
        return 1;
    }
    free(body.data);
    return report(status, response);
}

typedef struct {
    const char *name;
    int (*run)(int argc, char **argv);
} appctl_command_t;

static const appctl_command_t k_commands[] = {
    {"install", cmd_install}, {"time", cmd_time},   {"status", cmd_status},
    {"list", cmd_list},       {"start", cmd_start}, {"stop", cmd_stop},
    {"rollback", cmd_rollback}, {"uninstall", cmd_uninstall}, {"upgrade", cmd_upgrade},
    {"grant", cmd_grant},     {"revoke", cmd_revoke}, {"grants", cmd_grants},
    {"snapshot", cmd_snapshot},
};

int main(int argc, char **argv) {
    if (argc < 2) return usage();
    for (size_t i = 0; i < sizeof(k_commands) / sizeof(k_commands[0]); i++) {
        if (strcmp(argv[1], k_commands[i].name) == 0) return k_commands[i].run(argc, argv);
    }
    return usage();
}
