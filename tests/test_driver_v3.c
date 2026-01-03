#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "net_buf.h"

// 显式声明，防止头文件包含顺序或缓存导致的隐释声明警告
extern edge_driver_ctx_t *driver_instance_create(const char *protocol_name, uint32_t device_id);
extern void driver_instance_destroy(edge_driver_ctx_t *ctx, const char *protocol_name);

// --- Mock Objects ---
extern edge_driver_t iec104_driver;

typedef struct {
    uint16_t send_seq;
    uint16_t recv_seq;
    uint16_t ack_seq;
    bool gi_active;
    uint64_t last_msg_ts;
} iec104_ctx_t_internal;

/**
 * 验证 V3 协议栈的有状态事务 (Session Persistence)
 */
static void test_v3_session_state(void **state) {
    (void)state;
    driver_manager_init();
    driver_manager_register(&iec104_driver);

    // 1. 创建 IEC104 实例
    uv_loop_t *loop = uv_default_loop();
    edge_driver_ctx_t *ctx = driver_instance_create("IEC104", 1);
    assert_non_null(ctx);
    ctx->user_data = loop;

    // 2. 模拟收到第一帧 I 帧 (V(S)=5)
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 4, 128, 0, NULL, NULL);

    net_buf_t *rx = net_buf_alloc(&pool, 0);
    // IEC104 I帧：68 0E [V(S)低][V(S)高] [V(R)低][V(R)高] ...
    // V(S)=5 -> (5<<1) = 10 (0x0A)
    uint8_t raw_i[] = {0x68, 0x04, 0x0A, 0x00, 0x00, 0x00};
    net_buf_simple_add_mem(&rx->b, raw_i, sizeof(raw_i));

    int ret = ctx->driver->handle_rx(ctx, rx, NULL, NULL, NULL);
    assert_int_equal(ret, 0);

    // 3. 核心验证：检查状态机是否记住了 V(R) 应为 6
    iec104_ctx_t_internal *p = (iec104_ctx_t_internal *)ctx->priv;
    assert_int_equal(p->recv_seq, 6);

    driver_instance_destroy(ctx, "IEC104");
    net_buf_unref(rx);
    net_buf_pool_destroy(&pool);
    driver_manager_fini();
}

/**
 * 验证 V3 net_buf 的内嵌元数据与原子计数
 */
static void test_v3_net_buf_advanced(void **state) {
    (void)state;
    net_buf_pool_t pool;
    // 预留 32 字节的元数据空间，池大小设为 2 以支持 Clone
    net_buf_pool_init(&pool, 2, 128, 32, NULL, NULL);

    net_buf_t *buf = net_buf_alloc(&pool, 0);
    assert_non_null(buf);

    // 1. 验证元数据存取
    uint32_t *meta = (uint32_t *)net_buf_user_data(buf);
    *meta = 0x55AA55AA;

    // 2. 模拟克隆 (元数据应同步)
    net_buf_t *clone = net_buf_clone(buf, 0);
    assert_non_null(clone);
    uint32_t *clone_meta = (uint32_t *)net_buf_user_data(clone);
    assert_int_equal(*clone_meta, 0x55AA55AA);

    // 3. 验证原子计数
    net_buf_ref(buf);
    net_buf_unref(buf);
    net_buf_unref(clone);
    net_buf_unref(buf);  // 真正回收

    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_v3_session_state),
        cmocka_unit_test(test_v3_net_buf_advanced),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}