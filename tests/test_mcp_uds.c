/**
 * @file test_mcp_uds.c
 * @brief Unit & Integration tests for Industrial Shared MCP Bus with Multi-Client UDS Transport (AEP-510)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <dirent.h>
#include <uv.h>
#include <cjson/cJSON.h>

#include "edgeos/edge_mcp_dispatcher.h"
#include "edgeos/mcp_transport.h"

#define TEST_UDS_PATH "/tmp/test_edge_mcp_bus.sock"

static int count_open_fds(void) {
    DIR *d = opendir("/proc/self/fd");
    if (!d) return -1;
    int count = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] != '.') {
            count++;
        }
    }
    closedir(d);
    return count;
}

static int connect_uds_client(const char *path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int send_msg(int fd, const char *msg) {
    size_t len = strlen(msg);
    char *buf = (char *)malloc(len + 2);
    if (!buf) return -1;
    memcpy(buf, msg, len);
    if (len == 0 || buf[len - 1] != '\n') {
        buf[len] = '\n';
        buf[len + 1] = '\0';
        len++;
    } else {
        buf[len] = '\0';
    }
    ssize_t written = write(fd, buf, len);
    free(buf);
    return (written == (ssize_t)len) ? 0 : -1;
}

static char *recv_line(int fd) {
    char *buf = (char *)malloc(65536);
    if (!buf) return NULL;
    size_t pos = 0;
    while (pos < 65535) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) break;
        if (c == '\n' || c == '\r') {
            if (pos > 0) break;
            else continue;
        }
        buf[pos++] = c;
    }
    if (pos == 0) {
        free(buf);
        return NULL;
    }
    buf[pos] = '\0';
    return buf;
}

typedef struct {
    uv_async_t *stop_async;
    int status;
} test_runner_ctx_t;

static void *client_test_thread(void *arg) {
    test_runner_ctx_t *ctx = (test_runner_ctx_t *)arg;
    printf("  [Client Thread] Starting multi-client UDS verification on %s...\n", TEST_UDS_PATH);

    /* 1. Test multiple concurrent client connections (at least 3 parallel client pipes) */
    int c1 = connect_uds_client(TEST_UDS_PATH);
    int c2 = connect_uds_client(TEST_UDS_PATH);
    int c3 = connect_uds_client(TEST_UDS_PATH);
    assert(c1 >= 0);
    assert(c2 >= 0);
    assert(c3 >= 0);
    printf("  --> Step 1: 3 concurrent client pipes connected successfully (c1=%d, c2=%d, c3=%d)\n", c1, c2, c3);

    /* 2. Client 1 sends initialize */
    const char *init_req = "{\"jsonrpc\":\"2.0\",\"id\":1001,\"method\":\"initialize\",\"params\":{}}";
    assert(send_msg(c1, init_req) == 0);
    char *resp1 = recv_line(c1);
    assert(resp1 != NULL);
    cJSON *j1 = cJSON_Parse(resp1);
    assert(j1 != NULL);
    cJSON *id1 = cJSON_GetObjectItem(j1, "id");
    assert(id1 && id1->valueint == 1001);
    cJSON *res1 = cJSON_GetObjectItem(j1, "result");
    assert(res1 != NULL);
    cJSON *srv = cJSON_GetObjectItem(res1, "serverInfo");
    assert(srv != NULL);
    cJSON *name = cJSON_GetObjectItem(srv, "name");
    assert(name && strcmp(name->valuestring, "edge-mcp-server") == 0);
    cJSON_Delete(j1);
    free(resp1);
    printf("  --> Step 2: Client 1 initialize handshake verified!\n");

    /* 3. Client 2 sends tools/list */
    const char *list_req = "{\"jsonrpc\":\"2.0\",\"id\":2002,\"method\":\"tools/list\"}";
    assert(send_msg(c2, list_req) == 0);
    char *resp2 = recv_line(c2);
    assert(resp2 != NULL);
    cJSON *j2 = cJSON_Parse(resp2);
    assert(j2 != NULL);
    cJSON *id2 = cJSON_GetObjectItem(j2, "id");
    assert(id2 && id2->valueint == 2002);
    cJSON *res2 = cJSON_GetObjectItem(j2, "result");
    assert(res2 != NULL);
    cJSON *tools_arr = cJSON_GetObjectItem(res2, "tools");
    assert(tools_arr && cJSON_IsArray(tools_arr));
    assert(cJSON_GetArraySize(tools_arr) >= 4);
    cJSON_Delete(j2);
    free(resp2);
    printf("  --> Step 3: Client 2 tools/list verified!\n");

    /* 4. Client 3 calls security_simulate_interlock */
    const char *tool_req3 = "{\"jsonrpc\":\"2.0\",\"id\":3003,\"method\":\"tools/call\",\"params\":{\"name\":\"security_simulate_interlock\",\"arguments\":{\"breaker_id\":\"QF1\",\"target_state\":\"CLOSE\"}}}";
    assert(send_msg(c3, tool_req3) == 0);
    char *resp3 = recv_line(c3);
    assert(resp3 != NULL);
    cJSON *j3 = cJSON_Parse(resp3);
    assert(j3 != NULL);
    cJSON *id3 = cJSON_GetObjectItem(j3, "id");
    assert(id3 && id3->valueint == 3003);
    cJSON *res3 = cJSON_GetObjectItem(j3, "result");
    assert(res3 != NULL);
    cJSON *content3 = cJSON_GetObjectItem(res3, "content");
    assert(content3 && cJSON_GetArraySize(content3) > 0);
    cJSON_Delete(j3);
    free(resp3);
    printf("  --> Step 4: Client 3 tool call (security_simulate_interlock) verified!\n");

    /* 5. Client 2 calls dcu_analyze_power_quality */
    const char *tool_req2 = "{\"jsonrpc\":\"2.0\",\"id\":2004,\"method\":\"tools/call\",\"params\":{\"name\":\"dcu_analyze_power_quality\",\"arguments\":{\"phase_a_v\":220.0,\"phase_b_v\":220.8,\"phase_c_v\":219.4,\"current_thd\":2.3}}}";
    assert(send_msg(c2, tool_req2) == 0);
    char *resp2_tool = recv_line(c2);
    assert(resp2_tool != NULL);
    cJSON *j2_tool = cJSON_Parse(resp2_tool);
    assert(j2_tool != NULL);
    cJSON *id2_tool = cJSON_GetObjectItem(j2_tool, "id");
    assert(id2_tool && id2_tool->valueint == 2004);
    cJSON *res2_tool = cJSON_GetObjectItem(j2_tool, "result");
    assert(res2_tool != NULL);
    cJSON_Delete(j2_tool);
    free(resp2_tool);
    printf("  --> Step 5: Client 2 tool call (dcu_analyze_power_quality) verified!\n");

    /* 6. Parallel Multiplexing & Response Routing Verification */
    assert(send_msg(c1, "{\"jsonrpc\":\"2.0\",\"id\":1111,\"method\":\"ping\"}") == 0);
    assert(send_msg(c2, "{\"jsonrpc\":\"2.0\",\"id\":2222,\"method\":\"ping\"}") == 0);
    assert(send_msg(c3, "{\"jsonrpc\":\"2.0\",\"id\":3333,\"method\":\"ping\"}") == 0);

    char *r1 = recv_line(c1);
    char *r2 = recv_line(c2);
    char *r3 = recv_line(c3);
    assert(r1 && r2 && r3);

    cJSON *jr1 = cJSON_Parse(r1);
    cJSON *jr2 = cJSON_Parse(r2);
    cJSON *jr3 = cJSON_Parse(r3);
    assert(jr1 && jr2 && jr3);

    assert(cJSON_GetObjectItem(jr1, "id")->valueint == 1111);
    assert(cJSON_GetObjectItem(jr2, "id")->valueint == 2222);
    assert(cJSON_GetObjectItem(jr3, "id")->valueint == 3333);

    cJSON_Delete(jr1);
    cJSON_Delete(jr2);
    cJSON_Delete(jr3);
    free(r1);
    free(r2);
    free(r3);
    printf("  --> Step 6: Parallel client response isolation and routing verified!\n");

    /* 7. Streaming buffer fragmentation / reassembly test */
    const char *frag1 = "{\"jsonrpc\":\"2.0\",\"id\":1999,\"meth";
    const char *frag2 = "od\":\"ping\"}\n";
    assert(write(c1, frag1, strlen(frag1)) == (ssize_t)strlen(frag1));
    usleep(15000); /* 15 ms delay between packet fragments */
    assert(write(c1, frag2, strlen(frag2)) == (ssize_t)strlen(frag2));

    char *r_frag = recv_line(c1);
    assert(r_frag != NULL);
    cJSON *j_frag = cJSON_Parse(r_frag);
    assert(j_frag != NULL);
    assert(cJSON_GetObjectItem(j_frag, "id")->valueint == 1999);
    cJSON_Delete(j_frag);
    free(r_frag);
    printf("  --> Step 7: Streaming buffer fragmentation and reassembly verified!\n");

    /* 8. Client disconnection and reconnection test */
    close(c1);
    close(c2);
    usleep(20000); /* Allow server to process client close */

    int c4 = connect_uds_client(TEST_UDS_PATH);
    int c5 = connect_uds_client(TEST_UDS_PATH);
    assert(c4 >= 0);
    assert(c5 >= 0);

    assert(send_msg(c4, "{\"jsonrpc\":\"2.0\",\"id\":4001,\"method\":\"ping\"}") == 0);
    char *r4 = recv_line(c4);
    assert(r4 != NULL);
    cJSON *j4 = cJSON_Parse(r4);
    assert(j4 != NULL && cJSON_GetObjectItem(j4, "id")->valueint == 4001);
    cJSON_Delete(j4);
    free(r4);

    assert(send_msg(c5, "{\"jsonrpc\":\"2.0\",\"id\":5001,\"method\":\"initialize\",\"params\":{}}") == 0);
    char *r5 = recv_line(c5);
    assert(r5 != NULL);
    cJSON *j5 = cJSON_Parse(r5);
    assert(j5 != NULL && cJSON_GetObjectItem(j5, "id")->valueint == 5001);
    cJSON_Delete(j5);
    free(r5);

    close(c3);
    close(c4);
    close(c5);
    printf("  --> Step 8: Client disconnection and reconnect verified!\n");

    ctx->status = 0;
    uv_async_send(ctx->stop_async);
    return NULL;
}

static void on_stop_async(uv_async_t *handle) {
    uv_stop(handle->loop);
}

static void on_timeout(uv_timer_t *timer) {
    fprintf(stderr, "[FATAL] test_mcp_uds timed out after 10 seconds!\n");
    uv_stop(timer->loop);
    assert(false);
}

int main(void) {
    printf("=== EdgeOS Industrial Shared MCP Bus (AEP-510) UDS Transport Tests ===\n");

    int fds_initial = count_open_fds();

    uv_loop_t loop;
    uv_loop_init(&loop);

    edge_mcp_env_t env = { .loop = &loop };
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(&env);
    assert(disp != NULL);

    mcp_transport_t *uds_trans = mcp_transport_uds_create(&loop, disp, TEST_UDS_PATH);
    assert(uds_trans != NULL);

    int rc = uds_trans->start(uds_trans);
    assert(rc == 0);
    printf("  --> Server started and bound to %s\n", TEST_UDS_PATH);
    assert(access(TEST_UDS_PATH, F_OK) == 0);

    uv_async_t stop_async;
    uv_async_init(&loop, &stop_async, on_stop_async);

    uv_timer_t timeout_timer;
    uv_timer_init(&loop, &timeout_timer);
    uv_timer_start(&timeout_timer, on_timeout, 10000, 0);

    test_runner_ctx_t runner_ctx = {
        .stop_async = &stop_async,
        .status = -1
    };

    pthread_t th;
    assert(pthread_create(&th, NULL, client_test_thread, &runner_ctx) == 0);

    uv_run(&loop, UV_RUN_DEFAULT);

    pthread_join(th, NULL);
    assert(runner_ctx.status == 0);

    /* Stop and cleanup transport */
    uds_trans->stop(uds_trans);
    uds_trans->destroy(uds_trans);
    edge_mcp_dispatcher_destroy(disp);

    uv_timer_stop(&timeout_timer);
    uv_close((uv_handle_t *)&timeout_timer, NULL);
    uv_close((uv_handle_t *)&stop_async, NULL);

    while (uv_run(&loop, UV_RUN_NOWAIT));
    uv_loop_close(&loop);

    /* Verify socket is cleanly unlinked */
    assert(access(TEST_UDS_PATH, F_OK) != 0);
    printf("  --> Verified test socket file unlinked cleanly on destroy.\n");

    /* Verify file descriptor leak absence */
    int fds_final = count_open_fds();
    if (fds_initial > 0 && fds_final > 0) {
        printf("  --> Open fds: initial=%d, final=%d\n", fds_initial, fds_final);
        assert(fds_final <= fds_initial + 2);
    }

    printf("=== All MCP UDS Transport Tests PASSED (AEP-510 Green) ===\n");
    return 0;
}
