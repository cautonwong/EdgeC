/**
 * @file test_mqtt_subscribe.c
 * @brief MQTT 下行订阅（ADR-0011 N6-1）—— 真 broker 集成测试
 *
 * ## 为什么用真 broker 而不是 mock
 *
 * 订阅这条路上有三件事只有真 broker 能验：
 *   1. `subscribe` 之后消息**真的**会送到 `on_message`（回调注册对不对）
 *   2. 通配 `#` 的主题匹配（自己写的匹配器 vs broker 的匹配规则）
 *   3. 重连后重新订阅（Paho 的 cleansession=1 会丢订阅）
 *
 * mock 只能验证第 3 条的代码路径，验证不了"到底通不通"。
 *
 * ## 环境
 *
 * 需要 `mosquitto` 可执行文件。找不到就跳过（打印 SKIP，不判失败）——
 * CI 镜像里可能没有，不能因为缺 broker 就让整个测试套件红。
 */

#include <cmocka.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "mqtt_producer.h"

#define TEST_PORT 18831
#define WAIT_MS 3000

static pid_t g_broker = -1;

static int _port_open(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return 0;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int ok = connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0;
    close(fd);
    return ok;
}

static pid_t _start_broker(int port) {
    pid_t pid = fork();
    if (pid == 0) {
        char p[16];
        snprintf(p, sizeof(p), "%d", port);
        /* 关掉 mosquitto 自己的日志噪声 */
        freopen("/dev/null", "w", stderr);
        execlp("mosquitto", "mosquitto", "-p", p, (char*)NULL);
        _exit(127);
    }
    return pid;
}

static void _stop_broker(pid_t pid) {
    if (pid <= 0) return;
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
}

static int _wait_connected(nb_transport_ops_t* t, int ms) {
    for (int i = 0; i < ms / 20; i++) {
        if (t->is_connected(t->ctx)) return 0;
        usleep(20000);
    }
    return -1;
}

/* 收到消息就记下来 */
static char g_topic[128];
static char g_payload[128];
static int g_hits = 0;

static void _on_msg(void* user, const char* topic, const uint8_t* p, size_t len) {
    (void)user;
    snprintf(g_topic, sizeof(g_topic), "%s", topic ? topic : "");
    size_t n = len < sizeof(g_payload) - 1 ? len : sizeof(g_payload) - 1;
    memcpy(g_payload, p, n);
    g_payload[n] = '\0';
    g_hits++;
}

static void test_subscribe_receives_message(void** state) {
    (void)state;
    nb_transport_ops_t t = mqtt_producer_create("tcp://127.0.0.1:18831", "edge-test-sub");
    assert_non_null(t.ctx);
    assert_non_null(t.subscribe);
    assert_int_equal(_wait_connected(&t, WAIT_MS), 0);

    g_hits = 0;
    assert_int_equal(t.subscribe(t.ctx, "edge/test/NCMD", _on_msg, NULL), 0);

    /* 用同一个 client 发给自己，省掉第二个连接 */
    usleep(200000); /* 等 SUBACK */
    const char* body = "{\"p\":\"RELAY\",\"v\":1}";
    assert_int_equal(t.publish(t.ctx, "edge/test/NCMD", (const uint8_t*)body, strlen(body)), 0);

    for (int i = 0; i < 100 && g_hits == 0; i++) usleep(20000);

    assert_int_equal(g_hits, 1);
    assert_string_equal(g_topic, "edge/test/NCMD");
    assert_string_equal(g_payload, body);

    mqtt_producer_destroy(&t);
}

static void test_wildcard_subscription(void** state) {
    (void)state;
    nb_transport_ops_t t = mqtt_producer_create("tcp://127.0.0.1:18831", "edge-test-wild");
    assert_non_null(t.ctx);
    assert_int_equal(_wait_connected(&t, WAIT_MS), 0);

    g_hits = 0;
    /* 尾随 # 是最常见的下行写法：一个主题收全部命令 */
    assert_int_equal(t.subscribe(t.ctx, "edge/node7/#", _on_msg, NULL), 0);
    usleep(200000);

    assert_int_equal(t.publish(t.ctx, "edge/node7/NCMD", (const uint8_t*)"x", 1), 0);
    for (int i = 0; i < 100 && g_hits == 0; i++) usleep(20000);

    assert_int_equal(g_hits, 1);
    assert_string_equal(g_topic, "edge/node7/NCMD");

    mqtt_producer_destroy(&t);
}

static void test_duplicate_subscribe_updates_callback(void** state) {
    (void)state;
    nb_transport_ops_t t = mqtt_producer_create("tcp://127.0.0.1:18831", "edge-test-dup");
    assert_non_null(t.ctx);
    assert_int_equal(_wait_connected(&t, WAIT_MS), 0);

    /* 同一主题订两次：不该占两个槽位，也不该收到两条 */
    assert_int_equal(t.subscribe(t.ctx, "edge/dup/NCMD", _on_msg, NULL), 0);
    assert_int_equal(t.subscribe(t.ctx, "edge/dup/NCMD", _on_msg, NULL), 0);
    usleep(200000);

    g_hits = 0;
    assert_int_equal(t.publish(t.ctx, "edge/dup/NCMD", (const uint8_t*)"y", 1), 0);
    for (int i = 0; i < 100 && g_hits == 0; i++) usleep(20000);
    usleep(300000); /* 再等一会，确认没有第二条 */

    assert_int_equal(g_hits, 1);

    mqtt_producer_destroy(&t);
}

static int setup(void** state) {
    (void)state;
    if (system("command -v mosquitto >/dev/null 2>&1") != 0) {
        print_message("[SKIP] mosquitto not found, skipping MQTT integration tests\n");
        return 1; /* 非 0 = 跳过本组 */
    }
    g_broker = _start_broker(TEST_PORT);
    if (g_broker <= 0) return 1;

    for (int i = 0; i < WAIT_MS / 50; i++) {
        if (_port_open(TEST_PORT)) return 0;
        usleep(50000);
    }
    _stop_broker(g_broker);
    g_broker = -1;
    print_message("[SKIP] broker did not come up\n");
    return 1;
}

static int teardown(void** state) {
    (void)state;
    _stop_broker(g_broker);
    g_broker = -1;
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_subscribe_receives_message),
        cmocka_unit_test(test_wildcard_subscription),
        cmocka_unit_test(test_duplicate_subscribe_updates_callback),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
