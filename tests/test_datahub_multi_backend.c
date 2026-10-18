#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "datahub_dds.h"
#include "datahub_mqtt.h"
#include "datahub_router.h"

int main(void) {
    printf(
        "=== Testing Three-Backend DataHub Architecture & Consistency Policy (Issue #325) ===\n");

    uv_loop_t *loop = uv_default_loop();
    assert(loop != NULL);

    datahub_register_backend(DATAHUB_BACKEND_DDS, &g_datahub_dds_ops);
    datahub_register_backend(DATAHUB_BACKEND_MQTT, &g_datahub_mqtt_ops);

    // 1. 测试默认 Best Effort 策略
    datahub_router_config_t config;
    memset(&config, 0, sizeof(config));
    config.enable_shm = true;
    config.enable_dds = true;
    config.enable_mqtt = true;
    config.dds_ops = &g_datahub_dds_ops;
    config.mqtt_ops = &g_datahub_mqtt_ops;
    config.write_policy = DATAHUB_ROUTER_POLICY_BEST_EFFORT;

    snprintf(config.shm_config.shm_name, sizeof(config.shm_config.shm_name), "test_router_shm");
    config.shm_config.ring_slots = 64;
    config.shm_config.shadow_capacity = 2048;

    datahub_router_t router;
    int rc = datahub_router_init(&router, loop, &config, NULL, 0, NULL, NULL, NULL);
    assert(rc == 0);
    assert(router.write_policy == DATAHUB_ROUTER_POLICY_BEST_EFFORT);
    printf("  [PASS] DataHub Router Initialized with Best-Effort Policy\n");

    pipe_msg_t test_msg;
    memset(&test_msg, 0, sizeof(test_msg));
    test_msg.tag_id = 1001;
    test_msg.payload.val.f = 220.5f;

    rc = datahub_router_publish(&router, &test_msg);
    assert(rc == 0);
    printf("  [PASS] Best-Effort Concurrent Publish Fan-Out\n");

    rc = datahub_router_write_shadow(&router, &test_msg);
    assert(rc == 0);
    printf("  [PASS] Best-Effort Concurrent Write Shadow Fan-Out\n");

    pipe_msg_t read_msg;
    memset(&read_msg, 0, sizeof(read_msg));
    rc = datahub_router_read_shadow(&router, 1001, &read_msg);
    assert(rc == 0);
    assert(read_msg.tag_id == 1001);
    printf("  [PASS] Low-Latency Primary Shadow Read\n");

    datahub_router_destroy(&router);
    printf("  [PASS] Router Destroyed Cleanly\n");

    // 2. 测试 ALL_REQUIRED 强一致性策略
    memset(&config, 0, sizeof(config));
    config.enable_shm = true;
    config.enable_dds = false;
    config.enable_mqtt = false;
    config.write_policy = DATAHUB_ROUTER_POLICY_ALL_REQUIRED;
    snprintf(config.shm_config.shm_name, sizeof(config.shm_config.shm_name),
             "test_router_strict_shm");
    config.shm_config.ring_slots = 64;
    config.shm_config.shadow_capacity = 2048;

    rc = datahub_router_init(&router, loop, &config, NULL, 0, NULL, NULL, NULL);
    assert(rc == 0);
    assert(router.write_policy == DATAHUB_ROUTER_POLICY_ALL_REQUIRED);

    rc = datahub_router_publish(&router, &test_msg);
    assert(rc == 0);
    rc = datahub_router_write_shadow(&router, &test_msg);
    assert(rc == 0);
    printf("  [PASS] ALL_REQUIRED Consistency Policy verified with 100%% active backends\n");

    datahub_router_destroy(&router);

    // 3. 测试 DDS Backend 独立初始化与 Shadow Read
    datahub_t dds_hub;
    memset(&dds_hub, 0, sizeof(dds_hub));
    datahub_config_t dds_conf;
    memset(&dds_conf, 0, sizeof(dds_conf));
    dds_conf.dds_domain_id = 0;

    rc = datahub_dds_init(&dds_hub, &dds_conf);
    assert(rc == 0);

    pipe_msg_t dds_read;
    memset(&dds_read, 0, sizeof(dds_read));
    rc = dds_hub.ops->read_shadow(&dds_hub, 1001, &dds_read);
    /* 无硬件对端样本时应返回真实错误码，绝不假冒 0 成功 */
    assert(rc == -ENOENT || rc == -ENOTSUP || rc == -2 || rc == -38);
    printf(
        "  [PASS] DDS Shadow Read returns non-zero when no peer data is present (No Fake "
        "Success)\n");

    if (dds_hub.ops && dds_hub.ops->destroy) {
        dds_hub.ops->destroy(&dds_hub);
    }

    // 4. 测试 DDS 禁用时的优雅降级 (Graceful Degradation when DDS is disabled)
    memset(&config, 0, sizeof(config));
    config.enable_shm = true;
    config.enable_dds = true;  // 即使配置开启 DDS
    static const datahub_ops_t s_mock_disabled_ops = {
        .init = NULL  // ops 存在但无 init 或 init 失败
    };
    config.dds_ops = &s_mock_disabled_ops;
    config.enable_mqtt = false;
    config.write_policy = DATAHUB_ROUTER_POLICY_BEST_EFFORT;
    snprintf(config.shm_config.shm_name, sizeof(config.shm_config.shm_name),
             "test_router_degraded");
    config.shm_config.ring_slots = 64;
    config.shm_config.shadow_capacity = 2048;

    rc = datahub_router_init(&router, loop, &config, NULL, 0, NULL, NULL, NULL);
    assert(rc == 0);
    assert(router.shm_active == true);
    assert(router.dds_active == false);  // DDS 应优雅降级为非活跃
    printf("  [PASS] Graceful Degradation: Router successfully active with DDS disabled\n");

    rc = datahub_router_publish(&router, &test_msg);
    assert(rc == 0);
    printf("  [PASS] Best-Effort Publish succeeds when DDS is disabled (routed via active SHM)\n");

    datahub_router_destroy(&router);

    // 5. 测试 DATAHUB_BACKEND_HYBRID 经由 datahub_init_with_config 统一 Seam 初始化与写入
    datahub_t hybrid_hub;
    memset(&hybrid_hub, 0, sizeof(hybrid_hub));
    datahub_config_t hybrid_conf;
    memset(&hybrid_conf, 0, sizeof(hybrid_conf));
    hybrid_conf.backend_type = DATAHUB_BACKEND_HYBRID;
    snprintf(hybrid_conf.shm_name, sizeof(hybrid_conf.shm_name), "test_hybrid_seam");
    hybrid_conf.ring_slots = 64;
    hybrid_conf.shadow_capacity = 2048;

    rc = datahub_init_with_config(&hybrid_hub, loop, &hybrid_conf, NULL, 0, NULL, NULL, NULL);
    assert(rc == 0);
    assert(hybrid_hub.ops == &g_datahub_router_ops);
    printf("  [PASS] DATAHUB_BACKEND_HYBRID correctly selects g_datahub_router_ops\n");

    rc = datahub_write_shadow(&hybrid_hub, &test_msg);
    assert(rc == 0);
    printf("  [PASS] datahub_write_shadow successfully routes across composite seam\n");

    datahub_destroy(&hybrid_hub);
    printf("  [PASS] Hybrid DataHub destroyed cleanly\n");

    printf("=== All DataHub Consistency & DDS Shadow Read Tests Passed Successfully! ===\n");
    return 0;
}
