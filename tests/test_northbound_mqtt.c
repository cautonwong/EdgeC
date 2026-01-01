#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "edge_service.h"
#include "northbound_mgr.h"

// 引用 Mock 状态
extern char g_mock_mqtt_topic[];
extern char g_mock_mqtt_payload[];
extern int g_mock_mqtt_send_count;
extern void mock_mqtt_reset(void);

// 创建临时契约文件 (YAML)
static void create_test_contract(const char *path) {
    FILE *f = fopen(path, "w");
    // 注意：这里的 TagID 1003 必须与测试发送的数据一致
    // 且必须与链接的 contract_iems 中的定义一致 (IEMS 中 1003 是 GRID_POWER)
    fprintf(f,
            "product: TEST_MOCK\ntags:\n  - name: GRID_POWER\n    id: 1003\n    unit: W\n    "
            "attributes:\n      upload: true\n");
    fclose(f);
}

static void test_mqtt_push_logic(void **state) {
    (void)state;
    mock_mqtt_reset();

    uv_loop_t *loop = uv_default_loop();

    // 1. 准备临时契约
    const char *contract_path = "mqtt_test_contract.yaml";
    create_test_contract(contract_path);

    // 2. 准备 DataHub
    shm_unlink("mqtt_test_hub");  // 预清理
    datahub_t hub;
    int ret = datahub_init(&hub, loop, "mqtt_test_hub", 64);
    assert_int_equal(ret, 0);

    // 3. 准备 Northbound Mgr
    northbound_mgr_t mgr;
    northbound_mgr_init(&mgr, loop, &hub.ring);
    mgr.web_port = 18080;
    mgr.rest_port = 18081;

    service_options_t opts = {0};
    service_t *svc = service_new(&opts);
    mgr.user_data = svc;
    service_set_contract_path(svc, contract_path);

    // 4. 启动
    northbound_mgr_start(&mgr);
    usleep(200000);  // 等待线程和 MQTT 连接

    // 5. 注入数据 (3001: ENERGY_ACTIVE_IMPORT, upload=true by default)

    PipeMsg msg = {0};

    msg.tag_id = 3001;

    msg.payload.val.f = 999.99;

    msg.ts = 100000;

    datahub_publish(&hub, &msg);

    // 6. 等待轮询 (northbound 100ms 轮询)

    for (int i = 0; i < 20; i++) {
        usleep(50000);

        if (g_mock_mqtt_send_count > 0) break;
    }

    // 7. 验证

    assert_int_equal(g_mock_mqtt_send_count, 1);

    assert_string_equal(g_mock_mqtt_topic, "telemetry/ENERGY_ACTIVE_IMPORT");

    // 8. 清理
    northbound_mgr_stop(&mgr);
    datahub_destroy(&hub);
    service_delete(svc);
    remove(contract_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mqtt_push_logic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}