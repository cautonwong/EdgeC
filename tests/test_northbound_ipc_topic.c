/**
 * @file test_northbound_ipc_topic.c
 * @brief 现场 ipc/{sn}/... binaryFlow 主题：逐字矩阵与拒绝路径。
 *
 * 主题拼错时"看起来正常但主站永远收不到"，所以样例必须逐字锁定，
 * 越界与非法段必须整条拒绝，不能写出半截主题。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "north_ipc_port.h"

#define TEST_SN "E4B853F333"
#define CANARY 0xAA

static void _assert_rejected(const char* sn, const north_ipc_endpoint_t* ep,
                             north_ipc_verb_t verb) {
    char out[160];
    memset(out, CANARY, sizeof(out));
    assert_int_equal(north_ipc_render_topic(sn, ep, verb, out, sizeof(out)), -1);
    for (size_t i = 0; i < sizeof(out); i++) {
        assert_int_equal((unsigned char)out[i], CANARY);
    }
}

static void test_sample_matrix(void** state) {
    (void)state;
    /* v3 样例配置的 2 个设备 x 4 个动词，逐字等于设计 §1.1。 */
    static const struct {
        north_ipc_endpoint_t ep;
        north_ipc_verb_t verb;
        const char* expect;
    } cases[] = {
        {{"env", "ems", "emsctrl"},
         NORTH_IPC_SET,
         "ipc/E4B853F333/env/ems/emsctrl/Set/binaryFlow"},
        {{"env", "ems", "emsctrl"},
         NORTH_IPC_GET,
         "ipc/E4B853F333/env/ems/emsctrl/Get/binaryFlow"},
        {{"env", "ems", "emsctrl"},
         NORTH_IPC_SET_RESP,
         "ipc/E4B853F333/env/ems/emsctrl/SetResp/binaryFlow"},
        {{"env", "ems", "emsctrl"},
         NORTH_IPC_GET_RESP,
         "ipc/E4B853F333/env/ems/emsctrl/GetResp/binaryFlow"},
        {{"ems", "pcs456", "pcsdata"},
         NORTH_IPC_SET,
         "ipc/E4B853F333/ems/pcs456/pcsdata/Set/binaryFlow"},
        {{"ems", "pcs456", "pcsdata"},
         NORTH_IPC_GET,
         "ipc/E4B853F333/ems/pcs456/pcsdata/Get/binaryFlow"},
        {{"ems", "pcs456", "pcsdata"},
         NORTH_IPC_SET_RESP,
         "ipc/E4B853F333/ems/pcs456/pcsdata/SetResp/binaryFlow"},
        {{"ems", "pcs456", "pcsdata"},
         NORTH_IPC_GET_RESP,
         "ipc/E4B853F333/ems/pcs456/pcsdata/GetResp/binaryFlow"},
    };

    char topic[NORTH_IPC_TOPIC_MAX];
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        assert_int_equal(
            north_ipc_render_topic(TEST_SN, &cases[i].ep, cases[i].verb, topic, sizeof(topic)), 0);
        assert_string_equal(topic, cases[i].expect);
    }
}

static void test_rejects_bad_segments(void** state) {
    (void)state;
    const north_ipc_endpoint_t good = {"env", "ems", "emsctrl"};

    assert_int_equal(north_ipc_render_topic(NULL, &good, NORTH_IPC_SET, (char[128]){0}, 128), -1);
    assert_int_equal(north_ipc_render_topic("", &good, NORTH_IPC_SET, (char[128]){0}, 128), -1);
    _assert_rejected("bad/sn", &good, NORTH_IPC_SET);
    _assert_rejected("bad+sn", &good, NORTH_IPC_SET);
    _assert_rejected("bad#sn", &good, NORTH_IPC_SET);
    _assert_rejected("bad sn", &good, NORTH_IPC_SET);
    _assert_rejected("sn\xC3\xA4", &good, NORTH_IPC_SET);

    assert_int_equal(north_ipc_render_topic(TEST_SN, NULL, NORTH_IPC_SET, (char[128]){0}, 128),
                     -1);

    const north_ipc_endpoint_t bad_module[] = {
        {NULL, "ems", "emsctrl"},
        {"", "ems", "emsctrl"},
        {"a/b", "ems", "emsctrl"},
        {"a+b", "ems", "emsctrl"},
        {"a#b", "ems", "emsctrl"},
        {"a b", "ems", "emsctrl"},
        {"\xC3\xA4", "ems", "emsctrl"},
    };
    for (size_t i = 0; i < sizeof(bad_module) / sizeof(bad_module[0]); i++) {
        _assert_rejected(TEST_SN, &bad_module[i], NORTH_IPC_SET);
    }

    const north_ipc_endpoint_t bad_dev = {"env", "a/b", "emsctrl"};
    _assert_rejected(TEST_SN, &bad_dev, NORTH_IPC_SET);
    const north_ipc_endpoint_t bad_type = {"env", "ems", "a#b"};
    _assert_rejected(TEST_SN, &bad_type, NORTH_IPC_SET);

    _assert_rejected(TEST_SN, &good, (north_ipc_verb_t)4);
    _assert_rejected(TEST_SN, &good, (north_ipc_verb_t)99);
    _assert_rejected(TEST_SN, &good, (north_ipc_verb_t)-1);

    char out[128];
    assert_int_equal(north_ipc_render_topic(TEST_SN, &good, NORTH_IPC_SET, NULL, sizeof(out)), -1);
    memset(out, CANARY, sizeof(out));
    assert_int_equal(north_ipc_render_topic(TEST_SN, &good, NORTH_IPC_SET, out, 0), -1);
    for (size_t i = 0; i < sizeof(out); i++) {
        assert_int_equal((unsigned char)out[i], CANARY);
    }
}

static void _fill(char* buf, size_t len) {
    memset(buf, 'a', len);
    buf[len] = '\0';
}

static void test_length_boundary(void** state) {
    (void)state;
    char sn[NORTH_IPC_SN_MAX];
    char module[64];
    char dev[64];
    char type[64];
    /* 固定部分 = "ipc/" + 5 个分隔斜杠 + "binaryFlow" = 19 B；Set = 3 B。
     * 127 = 19 + 3 + 105，128 只多一个字符。 */
    _fill(sn, 57);
    _fill(module, 16);
    _fill(dev, 16);
    _fill(type, 16);
    north_ipc_endpoint_t ep = {module, dev, type};

    char out[160];
    memset(out, CANARY, sizeof(out));
    assert_int_equal(north_ipc_render_topic(sn, &ep, NORTH_IPC_SET, out, sizeof(out)), 0);
    assert_int_equal(strlen(out), 127);
    for (size_t i = 128; i < sizeof(out); i++) {
        assert_int_equal((unsigned char)out[i], CANARY);
    }

    memset(out, CANARY, sizeof(out));
    assert_int_equal(north_ipc_render_topic(sn, &ep, NORTH_IPC_SET, out, 128), 0);
    memset(out, CANARY, sizeof(out));
    assert_int_equal(north_ipc_render_topic(sn, &ep, NORTH_IPC_SET, out, 127), -1);
    for (size_t i = 0; i < sizeof(out); i++) {
        assert_int_equal((unsigned char)out[i], CANARY);
    }

    /* 总长 128：即使 out_sz 够也必须拒绝。 */
    _fill(module, 17);
    memset(out, CANARY, sizeof(out));
    assert_int_equal(north_ipc_render_topic(sn, &ep, NORTH_IPC_SET, out, sizeof(out)), -1);
    for (size_t i = 0; i < sizeof(out); i++) {
        assert_int_equal((unsigned char)out[i], CANARY);
    }
}

static void test_verb_case_is_exact(void** state) {
    (void)state;
    const north_ipc_endpoint_t ep = {"env", "ems", "emsctrl"};
    static const struct {
        north_ipc_verb_t verb;
        const char* suffix;
    } verbs[] = {
        {NORTH_IPC_SET, "/Set/binaryFlow"},
        {NORTH_IPC_GET, "/Get/binaryFlow"},
        {NORTH_IPC_SET_RESP, "/SetResp/binaryFlow"},
        {NORTH_IPC_GET_RESP, "/GetResp/binaryFlow"},
    };

    char topic[NORTH_IPC_TOPIC_MAX];
    for (size_t i = 0; i < sizeof(verbs) / sizeof(verbs[0]); i++) {
        assert_int_equal(north_ipc_render_topic(TEST_SN, &ep, verbs[i].verb, topic,
                                                sizeof(topic)),
                         0);
        size_t len = strlen(topic);
        size_t suffix_len = strlen(verbs[i].suffix);
        assert_true(len > suffix_len);
        assert_string_equal(topic + len - suffix_len, verbs[i].suffix);
    }
    assert_null(strstr(topic, "/set/"));
    assert_null(strstr(topic, "/get/"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sample_matrix),
        cmocka_unit_test(test_rejects_bad_segments),
        cmocka_unit_test(test_length_boundary),
        cmocka_unit_test(test_verb_case_is_exact),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
