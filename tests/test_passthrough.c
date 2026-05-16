#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <stdio.h>
#include "passthrough_mgr.h"
#include "io_executor.h"
#include "datahub.h"

// Mock IO handle
static int mock_write_count = 0;
static uint8_t last_written[1024];
static size_t last_written_len = 0;

static int mock_write_async(io_handle_t *h, net_buf_t *buf) {
    (void)h;
    mock_write_count++;
    if (buf->b.len <= 1024) {
        memcpy(last_written, buf->b.data, buf->b.len);
        last_written_len = buf->b.len;
    }
    net_buf_unref(buf);
    return 0;
}

static void mock_io_destroy(io_handle_t *h) {
    (void)h;
}

static io_ops_t mock_ops = {
    .write_async = mock_write_async,
    .destroy = mock_io_destroy,
};

static void test_passthrough_basic(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    
    // Need a dummy DataHub for Orchestrator
    datahub_t *hub = calloc(1, sizeof(datahub_t));
    datahub_init(hub, loop, "test", 64, 64, NULL, 0, NULL, NULL, NULL);
    
    arbiter_t *arb;
    arbiter_init(&arb, loop);
    arbiter_resume(arb);
    
    // Create Orchestrator with mock IO on Bus 0
    bus_orchestrator_t *orch = bus_orchestrator_create(hub, arb, NULL, 1, 10, 1024);
    io_handle_t mock_io = { .ops = &mock_ops };
    bus_orchestrator_attach_io(orch, 0, &mock_io);
    
    passthrough_mgr_t *mgr = passthrough_mgr_create(loop, orch, arb);
    passthrough_config_t cfg = { .port = 9988, .target_bus_id = 0 };
    assert_int_equal(passthrough_mgr_start(mgr, &cfg, 1), 1);
    
    // Connect a client to port 9988
    uv_tcp_t client;
    uv_tcp_init(loop, &client);
    struct sockaddr_in dest;
    uv_ip4_addr("127.0.0.1", 9988, &dest);
    
    uv_connect_t conn_req;
    int rc = uv_tcp_connect(&conn_req, &client, (const struct sockaddr *)&dest, NULL);
    assert_int_equal(rc, 0);
    
    // Run loop to establish connection
    for (int i=0; i<10; i++) uv_run(loop, UV_RUN_ONCE);
    
    // Send data from client
    uint8_t test_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uv_buf_t buf = uv_buf_init((char *)test_data, 4);
    uv_write_t write_req;
    uv_write(&write_req, (uv_stream_t *)&client, &buf, 1, NULL);
    
    // Run loop to process write and reach IO
    for (int i=0; i<10; i++) uv_run(loop, UV_RUN_ONCE);
    
    // Verify IO handle received data
    assert_int_equal(mock_write_count, 1);
    assert_int_equal(last_written_len, 4);
    assert_memory_equal(last_written, test_data, 4);
    
    // Verify arbiter exclusive hold
    arbiter_bus_status_t status[1];
    arbiter_get_statuses(arb, status, 1);
    assert_true(status[0].is_exclusive);
    
    // Close client
    uv_close((uv_handle_t *)&client, NULL);
    for (int i=0; i<10; i++) uv_run(loop, UV_RUN_ONCE);
    
    // Verify arbiter exclusive hold released
    arbiter_get_statuses(arb, status, 1);
    assert_false(status[0].is_exclusive);
    
    passthrough_mgr_destroy(mgr);
    bus_orchestrator_destroy(orch);
    arbiter_destroy(arb);
    datahub_destroy(hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_passthrough_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
