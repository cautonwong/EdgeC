#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <math.h>
#include <unistd.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contract_data_types.h"
#include "datahub.h"
#include "stream_engine.h"
#include "driver_manager.h"
#include "iec_asdu.h"
#include "iec104_apci.h"
#include "modbus_codec.h"

// IEMS Generated Contracts
#include "contracts.h"

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    arbiter_t *arb;
    stream_engine_t *engine;
    bus_orchestrator_t *orch;
    driver_manager_t *drv_mgr;
    net_buf_pool_t pool;
    io_handle_t iec_io;
    io_handle_t mb_io;
} iems_test_ctx_t;

// Mock IO for IEC 104
static int iec_tx_count = 0;
static uint8_t iec_last_tx[256];
static size_t iec_last_tx_len = 0;

static int iec_write_async(io_handle_t *h, net_buf_t *buf) {
    (void)h;
    iec_tx_count++;
    if (buf->b.len <= 256) {
        memcpy(iec_last_tx, buf->b.data, buf->b.len);
        iec_last_tx_len = buf->b.len;
    }
    net_buf_unref(buf);
    return 0;
}

static int mock_enable_read(io_handle_t *h, bool enable) {
    (void)h; (void)enable;
    return 0;
}

static void mock_io_destroy(io_handle_t *h) {
    (void)h;
}

static io_ops_t iec_mock_ops = { .write_async = iec_write_async, .enable_read = mock_enable_read, .destroy = mock_io_destroy };

// Mock IO for Modbus
static int mb_tx_count = 0;
static uint8_t mb_last_tx[256];
static size_t mb_last_tx_len = 0;

static int mb_write_async(io_handle_t *h, net_buf_t *buf) {
    (void)h;
    mb_tx_count++;
    if (buf->b.len <= 256) {
        memcpy(mb_last_tx, buf->b.data, buf->b.len);
        mb_last_tx_len = buf->b.len;
    }
    net_buf_unref(buf);
    return 0;
}

static io_ops_t mb_mock_ops = { .write_async = mb_write_async, .enable_read = mock_enable_read, .destroy = mock_io_destroy };

static int setup(void **state) {
    iems_test_ctx_t *ctx = calloc(1, sizeof(iems_test_ctx_t));
    uv_loop_init(&ctx->loop);

    arbiter_init(&ctx->arb, &ctx->loop);
    arbiter_resume(ctx->arb);

    ctx->drv_mgr = driver_manager_create();
    extern int iec104_driver_register(driver_manager_t *mgr);
    extern int modbus_driver_register(driver_manager_t *mgr);
    iec104_driver_register(ctx->drv_mgr);
    modbus_driver_register(ctx->drv_mgr);

    // [Nexus] Hub and Orch should have independent pools for reliable unit testing
    datahub_init(&ctx->hub, &ctx->loop, "iems_test", 128, 1024, g_tag_metadata, g_tag_metadata_len, NULL, (datahub_tag_map_cb)edge_tag_get_compact_index, NULL);

    ctx->orch = bus_orchestrator_create(&ctx->hub, ctx->arb, ctx->drv_mgr, 2, 64, 1024);

    extern pipeline_stage_t *stage_mpc_dispatcher_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
    static const edge_stage_factory_entry_t test_stages[] = {
        {"mpc_dispatch", stage_mpc_dispatcher_create_v2},
        {NULL, NULL}
    };
    printf("[DEBUG] Setup: Creating StreamEngine...\n");
    ctx->engine = stream_engine_create(&ctx->hub, &ctx->loop, NULL, test_stages);
    if (!ctx->engine) {
        printf("[ERROR] Failed to create StreamEngine\n");
        return -1;
    }
    
    printf("[DEBUG] Setup: Starting StreamEngine...\n");
    stream_engine_start(ctx->engine);

    *state = ctx;
    printf("[DEBUG] Setup Complete.\n");
    return 0;
}

static void _on_force_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int teardown(void **state) {
    iems_test_ctx_t *ctx = *state;
    if (!ctx) return 0;
    
    // [Nexus] Flush all pending IO/Hub notifications BEFORE destroying anything
    printf("[DEBUG] Teardown: Pre-destruction loop flush...\n");
    for(int i=0; i<100; i++) uv_run(&ctx->loop, UV_RUN_NOWAIT);
    
    printf("[DEBUG] Teardown: Stopping engine...\n");
    if (ctx->engine) stream_engine_destroy(ctx->engine);
    
    printf("[DEBUG] Teardown: Destroying orchestrator...\n");
    if (ctx->orch) bus_orchestrator_destroy(ctx->orch);
    
    printf("[DEBUG] Teardown: Destroying datahub...\n");
    datahub_destroy(&ctx->hub);
    
    if (ctx->arb) arbiter_destroy(ctx->arb);

    uv_walk(&ctx->loop, _on_force_close, NULL);
    printf("[DEBUG] Teardown: Final loop flush...\n");
    uv_run(&ctx->loop, UV_RUN_DEFAULT);
    
    uv_loop_close(&ctx->loop);
    free(ctx);
    return 0;
}

static void test_iems_dispatch_flow(void **state) {
    iems_test_ctx_t *ctx = (iems_test_ctx_t *)*state;
    
    driver_contract_injection_t iec_inj = {
        .lookup_by_key = (const void *(*)(uint64_t))edge_get_map_iec104,
        .lookup_by_tag = (const void *(*)(uint32_t))edge_get_map_iec104_by_tag
    };
    bus_orchestrator_setup_slave(ctx->orch, 0, "IEC104", 1, &iec_inj);
    ctx->iec_io.ops = &iec_mock_ops;
    bus_orchestrator_attach_io(ctx->orch, 0, &ctx->iec_io);

    driver_contract_injection_t mb_inj = {
        .lookup_by_key = (const void *(*)(uint64_t))edge_get_map_modbus,
        .lookup_by_tag = (const void *(*)(uint32_t))edge_get_map_modbus_by_tag
    };
    bus_orchestrator_setup_slave(ctx->orch, 1, "ModbusRTU", 1, &mb_inj);
    ctx->mb_io.ops = &mb_mock_ops;
    bus_orchestrator_attach_io(ctx->orch, 1, &ctx->mb_io);

    // [Nexus] Use stack variables instead of static const to avoid segfault on modification
    edge_stage_def_t stages[] = {
        {.name = "dispatch", .type = "mpc_dispatch", 
         .params = {.u1 = 1001, .u3 = 4003, .f1 = 1.0, .f2 = 100.0, .f3 = 100.0},
         .next_indices = {-1, -1, -1, -1}}
    };
    edge_pipeline_def_t mpc_pipe = {
        .id = "mpc_logic", .trigger_tag = 1001, .stages = stages, .stage_count = 1
    };
    pipeline_loader_ctx_t l_ctx = { .orch = ctx->orch, .db = NULL, .js_engine = NULL, .params = NULL };
    stream_engine_load_static_metadata(ctx->engine, &mpc_pipe, 1, &l_ctx);

    // [Nexus] Use the orchestrator's pool for test allocations
    net_buf_pool_t *pool = bus_orchestrator_get_pool(ctx->orch);

    net_buf_t *in_buf = net_buf_alloc(pool, 0);
    uint8_t startdt[] = { 0x68, 0x04, 0x07, 0x00, 0x00, 0x00 };
    net_buf_simple_add_mem(&in_buf->b, startdt, 6);
    ctx->iec_io.cb(&ctx->iec_io, IO_EVENT_RX, in_buf, ctx->iec_io.user_data);
    for (int i=0; i<10; i++) uv_run(&ctx->loop, UV_RUN_NOWAIT);

    net_buf_t *cmd_buf = net_buf_alloc(pool, 0);
    iec_asdu_init(cmd_buf, IEC_TYPE_C_SE_NC_1, false, 1, IEC_COT_ACT, 1, &IEC_ADDR_CONF_104);
    iec_asdu_add_c_se_nc_1(cmd_buf, 16386, 1.0f, 0, false, &IEC_ADDR_CONF_104); 
    
    uint8_t apci[6] = { 0x68, (uint8_t)(cmd_buf->b.len + 4), 0x00, 0x00, 0x00, 0x00 };
    net_buf_t *full_cmd = net_buf_alloc(pool, 0);
    net_buf_simple_add_mem(&full_cmd->b, apci, 6);
    net_buf_simple_add_mem(&full_cmd->b, cmd_buf->b.data, cmd_buf->b.len);
    net_buf_unref(cmd_buf);

    printf("Injecting IEC 104 Command (Tag 1001=1.0)...\n");
    ctx->iec_io.cb(&ctx->iec_io, IO_EVENT_RX, full_cmd, ctx->iec_io.user_data);
    
    for (int i=0; i<100; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        usleep(2000);
    }

    pipe_msg_t tag_val;
    assert_int_equal(datahub_read_shadow(&ctx->hub, 1001, &tag_val), 0);
    printf("DataHub Tag 1001 final value: %.2f\n", tag_val.payload.val.f);
    
    assert_int_equal(datahub_read_shadow(&ctx->hub, 4003, &tag_val), 0);
    printf("DataHub Tag 4003 (MPC Output): %.2f A\n", tag_val.payload.val.f);
    assert_true(tag_val.payload.val.f > 0.05); 

    // Wait for Hub -> Slave propagation
    for (int i=0; i<100; i++) uv_run(&ctx->loop, UV_RUN_NOWAIT);
    usleep(100000); 

    // Wait for slave engine to update its registers from Hub
    uint16_t rv = 0;
    printf("[DEBUG] Starting polling for MPC Output (Tag 4003) -> Modbus Reg 15...\n");
    for (int retry = 0; retry < 100; retry++) {
        net_buf_t *mb_req = net_buf_alloc(pool, 0);
        uint8_t mb_read_req[] = { 0x01, 0x03, 0x00, 0x0F, 0x00, 0x01, 0x00, 0x00 };
        uint16_t crc = modbus_calculate_crc(mb_read_req, 6);
        mb_read_req[6] = (uint8_t)(crc & 0xFF);
        mb_read_req[7] = (uint8_t)(crc >> 8);
        net_buf_simple_add_mem(&mb_req->b, mb_read_req, 8);

        mb_tx_count = 0;
        ctx->mb_io.cb(&ctx->mb_io, IO_EVENT_RX, mb_req, ctx->mb_io.user_data);
        
        // Deep loop flush to ensure async notifications from Hub reach the Slave
        for (int i=0; i<300; i++) uv_run(&ctx->loop, UV_RUN_NOWAIT);
        
        if (mb_tx_count > 0) {
            rv = (uint16_t)((mb_last_tx[3] << 8) | mb_last_tx[4]);
            if (rv > 0) {
                printf("[DEBUG] Success! Found Reg 15 = %u at retry %d\n", rv, retry);
                break;
            }
        }
        usleep(30000); // 30ms
    }
    
    printf("Modbus Value for 4003 (Reg 15): %u\n", rv);
    assert_true(rv > 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_iems_dispatch_flow, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
