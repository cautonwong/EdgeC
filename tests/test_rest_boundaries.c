/**
 * @file test_rest_boundaries.c
 * @brief REST management-plane boundaries: health identity, strict tag ids,
 *        and bind failure as a startup failure.
 */

#include <arpa/inet.h>
#include <cmocka.h>
#include <cjson/cJSON.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "datahub_priv.h"
#include "edge_service.h"
#include "edge_version.h"
#include "rest_server.h"

static const edge_tag_meta_t k_metadata[] = {
    {.id = 0x1001, .name = "U_A", .type = "float", .unit = "V", .category = "GRID"},
};

typedef struct {
    uv_loop_t loop;
    arbiter_t *arb;
    datahub_t *hub;
    service_t *svc;
    rest_server_t *srv;
    rest_server_storage_t *storage;
} rest_fixture_t;

static void fixture_start(rest_fixture_t *f, const char *shm_name) {
    memset(f, 0, sizeof(*f));
    assert_int_equal(uv_loop_init(&f->loop), 0);
    f->hub = calloc(1, sizeof(*f->hub));
    assert_non_null(f->hub);
    shm_unlink(shm_name);
    assert_int_equal(
        datahub_init(f->hub, &f->loop, shm_name, 128, 65536, k_metadata, 1, NULL, NULL, NULL), 0);
    assert_int_equal(arbiter_init(&f->arb, &f->loop), 0);
    f->svc = service_new(&f->loop, f->hub, f->arb, NULL, NULL);
    assert_non_null(f->svc);
    f->storage = malloc(sizeof(*f->storage));
    assert_non_null(f->storage);
    f->srv = rest_server_create_in_ex(f->storage, &f->loop, f->svc, 0, NULL);
    assert_non_null(f->srv);
}

static void fixture_stop(rest_fixture_t *f) {
    if (f->srv) rest_server_destroy(f->srv);
    if (f->svc) service_delete(f->svc);
    if (f->hub) datahub_destroy(f->hub);
    arbiter_destroy(f->arb);
    int rc = uv_loop_close(&f->loop);
    for (int i = 0; i < 100000 && rc == UV_EBUSY; i++) {
        uv_run(&f->loop, UV_RUN_NOWAIT);
        rc = uv_loop_close(&f->loop);
    }
    assert_int_equal(rc, 0);
    free(f->hub);
    free(f->storage);
}

static double machine_uptime_sec(void) {
    FILE *f = fopen("/proc/uptime", "r");
    if (!f) return -1.0;
    double value = -1.0;
    if (fscanf(f, "%lf", &value) != 1) value = -1.0;
    fclose(f);
    return value;
}

static void test_health_reports_process_uptime_and_version(void **state) {
    (void)state;
    rest_fixture_t f;
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "/test_rest_health_%d", (int)getpid());
    fixture_start(&f, shm_name);

    char *resp = rest_dispatch(f.svc, "GET", "/api/v1/system/health", "");
    assert_non_null(resp);
    cJSON *root = cJSON_Parse(resp);
    assert_non_null(root);
    free(resp);

    cJSON *version = cJSON_GetObjectItem(root, "version");
    assert_true(cJSON_IsString(version));
    unsigned major = 0, minor = 0, patch = 0;
    char extra = 0;
    assert_int_equal(sscanf(version->valuestring, "%u.%u.%u%c", &major, &minor, &patch, &extra),
                     3);
    assert_string_equal(version->valuestring, edge_get_version()->version_str);

    cJSON *uptime = cJSON_GetObjectItem(root, "uptime");
    assert_true(cJSON_IsNumber(uptime));
    double first = uptime->valuedouble;
    /* uv_now() would report the machine's monotonic clock; process uptime
     * starts at REST server creation and stays far below it. */
    assert_true(first >= 0.0);
    assert_true(first < 10.0);
    double machine = machine_uptime_sec();
    if (machine > 20.0) assert_true(first < machine - 10.0);

    cJSON_Delete(root);

    resp = rest_dispatch(f.svc, "GET", "/api/v1/system/stats", "");
    assert_non_null(resp);
    root = cJSON_Parse(resp);
    assert_non_null(root);
    cJSON *proof = cJSON_GetObjectItem(root, "uptime_sec_proof");
    assert_true(cJSON_IsNumber(proof));
    assert_true(proof->valuedouble >= 0.0);
    assert_true(proof->valuedouble < 10.0);
    cJSON_Delete(root);
    free(resp);

    /* The same process keeps counting from its own baseline. */
    uv_run(&f.loop, UV_RUN_NOWAIT);
    usleep(1100000);
    uv_run(&f.loop, UV_RUN_NOWAIT);
    resp = rest_dispatch(f.svc, "GET", "/api/v1/system/health", "");
    assert_non_null(resp);
    root = cJSON_Parse(resp);
    assert_non_null(root);
    uptime = cJSON_GetObjectItem(root, "uptime");
    assert_true(cJSON_IsNumber(uptime));
    assert_true(uptime->valuedouble >= 1.0);
    assert_true(uptime->valuedouble > first);
    cJSON_Delete(root);
    free(resp);

    fixture_stop(&f);
}

static void test_data_latest_rejects_invalid_and_unknown_tags(void **state) {
    (void)state;
    rest_fixture_t f;
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "/test_rest_latest_%d", (int)getpid());
    fixture_start(&f, shm_name);

    pipe_msg_t msg = {
        .tag_id = 0x1001,
        .payload = {.val = {.f = 220.5f}},
        .quality = 0,
        .type = PIPEMSG_TYPE_SCALAR,
        .ts = 1720000000000ULL,
    };
    assert_int_equal(datahub_write_shadow(f.hub, &msg), 0);

    char *resp = rest_dispatch(f.svc, "GET", "/api/v1/data/latest/4097", "");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"tag_id\":4097"));
    assert_non_null(strstr(resp, "220.5"));
    free(resp);

    const char *bad[] = {"/api/v1/data/latest/abc", "/api/v1/data/latest/0",
                         "/api/v1/data/latest/12x", "/api/v1/data/latest/",
                         "/api/v1/data/latest/9999", "/api/v1/data/latest/4294967296"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        resp = rest_dispatch(f.svc, "GET", bad[i], "");
        assert_non_null(resp);
        assert_string_equal(resp, "{\"error\":\"not_found\"}");
        free(resp);
    }

    fixture_stop(&f);
}

static void test_bind_conflict_fails_startup(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    int holder = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(holder >= 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    assert_int_equal(bind(holder, (struct sockaddr *)&addr, sizeof(addr)), 0);
    assert_int_equal(listen(holder, 1), 0);
    socklen_t addr_len = sizeof(addr);
    assert_int_equal(getsockname(holder, (struct sockaddr *)&addr, &addr_len), 0);
    int port = ntohs(addr.sin_port);
    assert_true(port > 0);

    rest_server_t *srv = rest_server_create_in_ex(NULL, &loop, NULL, port, NULL);
    assert_null(srv);

    close(holder);
    int rc = uv_loop_close(&loop);
    for (int i = 0; i < 100000 && rc == UV_EBUSY; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        rc = uv_loop_close(&loop);
    }
    assert_int_equal(rc, 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_health_reports_process_uptime_and_version),
        cmocka_unit_test(test_data_latest_rejects_invalid_and_unknown_tags),
        cmocka_unit_test(test_bind_conflict_fails_startup),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
