#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_comm.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

#define MOCK_SLOTS 16

typedef struct {
    uint16_t oi;
    uint8_t attr;
    uint8_t data[192];
    size_t len;
    bool used;
} mock_slot_t;

typedef struct {
    mock_slot_t slots[MOCK_SLOTS];
    bool fail_save;
    int save_calls;
} mock_comm_store_t;

static int _mock_save(void *ctx, uint16_t oi, uint8_t attr, const uint8_t *buf, size_t len) {
    mock_comm_store_t *s = (mock_comm_store_t *)ctx;
    s->save_calls++;
    if (s->fail_save) return -1;
    if (len > sizeof(s->slots[0].data)) return -1;

    int free_idx = -1;
    for (int i = 0; i < MOCK_SLOTS; i++) {
        if (s->slots[i].used && s->slots[i].oi == oi && s->slots[i].attr == attr) {
            memcpy(s->slots[i].data, buf, len);
            s->slots[i].len = len;
            return 0;
        }
        if (!s->slots[i].used && free_idx < 0) free_idx = i;
    }
    if (free_idx < 0) return -1;
    s->slots[free_idx].used = true;
    s->slots[free_idx].oi = oi;
    s->slots[free_idx].attr = attr;
    memcpy(s->slots[free_idx].data, buf, len);
    s->slots[free_idx].len = len;
    return 0;
}

static int _mock_load(void *ctx, uint16_t oi, uint8_t attr, uint8_t *buf, size_t cap, size_t *len) {
    const mock_comm_store_t *s = (const mock_comm_store_t *)ctx;
    for (int i = 0; i < MOCK_SLOTS; i++) {
        if (s->slots[i].used && s->slots[i].oi == oi && s->slots[i].attr == attr) {
            if (s->slots[i].len > cap) return -1;
            memcpy(buf, s->slots[i].data, s->slots[i].len);
            *len = s->slots[i].len;
            return 0;
        }
    }
    return -1;
}

static void test_wireless_set_get_and_axdr_roundtrip(void **state) {
    (void)state;
    dcu_698_comm_init(NULL);

    assert_true(dcu_698_comm_is_oi(0x4500));
    assert_true(dcu_698_comm_is_oi(0x4501));
    assert_false(dcu_698_comm_is_oi(0x4502));

    dcu_comm_wireless_cfg_t cfg = {
        .apn = "CMNET",
        .user = "card",
        .password = "secret",
        .master_ip = {10, 20, 30, 40},
        .master_port = 5100,
        .backup_ip = {10, 20, 30, 41},
        .backup_port = 5101,
        .heartbeat_s = 120,
    };
    assert_int_equal(dcu_698_comm_set_wireless(0x4500, &cfg), 0);

    dcu_comm_wireless_cfg_t got = {0};
    assert_int_equal(dcu_698_comm_get_wireless(0x4500, &got), 0);
    assert_string_equal(got.apn, "CMNET");
    assert_string_equal(got.user, "card");
    assert_string_equal(got.password, "secret");
    assert_memory_equal(got.master_ip, cfg.master_ip, 4);
    assert_int_equal(got.master_port, 5100);
    assert_memory_equal(got.backup_ip, cfg.backup_ip, 4);
    assert_int_equal(got.backup_port, 5101);
    assert_int_equal(got.heartbeat_s, 120);

    uint8_t a2[192];
    size_t a2_len = 0;
    assert_int_equal(dcu_698_comm_encode_attr(0x4500, 2, a2, sizeof(a2), &a2_len), 0);
    assert_true(a2_len > 10);
    assert_int_equal(a2[0], DLT698_TAG_STRUCTURE);
    assert_int_equal(a2[1], 12);

    uint8_t a3[64];
    size_t a3_len = 0;
    assert_int_equal(dcu_698_comm_encode_attr(0x4500, 3, a3, sizeof(a3), &a3_len), 0);
    assert_int_equal(a3[0], DLT698_TAG_ARRAY);
    assert_int_equal(a3[1], 2);

    /* 通过 A-XDR decode_set 写入 0x4501 并校验编码回读一致 */
    assert_int_equal(dcu_698_comm_decode_set(0x4501, 2, a2, a2_len), 0);
    assert_int_equal(dcu_698_comm_decode_set(0x4501, 3, a3, a3_len), 0);

    uint8_t a2_check[192];
    size_t a2_check_len = 0;
    assert_int_equal(dcu_698_comm_encode_attr(0x4501, 2, a2_check, sizeof(a2_check), &a2_check_len),
                     0);
    assert_int_equal(a2_check_len, a2_len);
    assert_memory_equal(a2_check, a2, a2_len);
}

static void test_eth_set_get_and_axdr_roundtrip(void **state) {
    (void)state;
    dcu_698_comm_init(NULL);

    assert_true(dcu_698_comm_is_oi(0x4510));
    assert_true(dcu_698_comm_is_oi(0x4517));
    assert_false(dcu_698_comm_is_oi(0x4518));

    dcu_comm_eth_cfg_t cfg = {
        .ip_mode = DCU_ETH_MODE_STATIC,
        .ip = {192, 168, 1, 100},
        .netmask = {255, 255, 255, 0},
        .gateway = {192, 168, 1, 1},
        .master_ip = {192, 168, 1, 200},
        .master_port = 7000,
    };
    assert_int_equal(dcu_698_comm_set_eth(0x4510, &cfg), 0);

    dcu_comm_eth_cfg_t got = {0};
    assert_int_equal(dcu_698_comm_get_eth(0x4510, &got), 0);
    assert_int_equal(got.ip_mode, DCU_ETH_MODE_STATIC);
    assert_memory_equal(got.ip, cfg.ip, 4);
    assert_memory_equal(got.netmask, cfg.netmask, 4);
    assert_memory_equal(got.gateway, cfg.gateway, 4);
    assert_memory_equal(got.master_ip, cfg.master_ip, 4);
    assert_int_equal(got.master_port, 7000);

    uint8_t a4[64];
    size_t a4_len = 0;
    assert_int_equal(dcu_698_comm_encode_attr(0x4510, 4, a4, sizeof(a4), &a4_len), 0);
    assert_int_equal(a4[0], DLT698_TAG_STRUCTURE);
    assert_int_equal(a4[1], 6);

    uint8_t a3[32];
    size_t a3_len = 0;
    assert_int_equal(dcu_698_comm_encode_attr(0x4510, 3, a3, sizeof(a3), &a3_len), 0);
    assert_int_equal(a3[0], DLT698_TAG_ARRAY);
    assert_int_equal(a3[1], 1);

    assert_int_equal(dcu_698_comm_decode_set(0x4517, 4, a4, a4_len), 0);
    assert_int_equal(dcu_698_comm_decode_set(0x4517, 3, a3, a3_len), 0);
    assert_int_equal(dcu_698_comm_get_eth(0x4517, &got), 0);
    assert_int_equal(got.ip_mode, DCU_ETH_MODE_STATIC);
    assert_memory_equal(got.ip, cfg.ip, 4);
    assert_int_equal(got.master_port, 7000);
}

static void test_persistence_save_load_and_rollback(void **state) {
    (void)state;
    mock_comm_store_t mock = {0};
    dcu_comm_store_t store = {.ctx = &mock, .save = _mock_save, .load = _mock_load};
    dcu_698_comm_init(&store);

    dcu_comm_wireless_cfg_t w_init = {
        .apn = "UNINET",
        .user = "u1",
        .password = "p1",
        .master_ip = {1, 2, 3, 4},
        .master_port = 8001,
        .heartbeat_s = 45,
    };
    assert_int_equal(dcu_698_comm_set_wireless(0x4500, &w_init), 0);

    dcu_comm_eth_cfg_t e_init = {
        .ip_mode = DCU_ETH_MODE_STATIC,
        .ip = {172, 16, 0, 10},
        .netmask = {255, 255, 0, 0},
        .gateway = {172, 16, 0, 1},
        .master_ip = {172, 16, 0, 2},
        .master_port = 9001,
    };
    assert_int_equal(dcu_698_comm_set_eth(0x4511, &e_init), 0);

    /* 模拟重启后从持久化端口恢复 */
    dcu_698_comm_init(&store);
    dcu_comm_wireless_cfg_t w_reloaded = {0};
    assert_int_equal(dcu_698_comm_get_wireless(0x4500, &w_reloaded), 0);
    assert_string_equal(w_reloaded.apn, "UNINET");
    assert_int_equal(w_reloaded.master_port, 8001);
    assert_int_equal(w_reloaded.heartbeat_s, 45);

    dcu_comm_eth_cfg_t e_reloaded = {0};
    assert_int_equal(dcu_698_comm_get_eth(0x4511, &e_reloaded), 0);
    assert_memory_equal(e_reloaded.ip, e_init.ip, 4);
    assert_int_equal(e_reloaded.master_port, 9001);

    /* save 失败时回滚至旧值 */
    mock.fail_save = true;
    dcu_comm_wireless_cfg_t w_bad = w_init;
    strcpy(w_bad.apn, "NEWAPN");
    w_bad.master_port = 9999;
    assert_int_equal(dcu_698_comm_set_wireless(0x4500, &w_bad), EDGE_SLAVE_SRC_TEMPORARY);

    dcu_comm_wireless_cfg_t w_after = {0};
    assert_int_equal(dcu_698_comm_get_wireless(0x4500, &w_after), 0);
    assert_string_equal(w_after.apn, "UNINET");
    assert_int_equal(w_after.master_port, 8001);

    /* A-XDR decode_set 在 save 失败时同样回滚 */
    uint8_t eth_a3[] = {
        DLT698_TAG_ARRAY, 1,
        DLT698_TAG_STRUCTURE, 2,
        DLT698_TAG_OCTET_STRING, 4, 10, 0, 0, 1,
        DLT698_TAG_LONG_UNSIGNED, 0x1F, 0x90,
    };
    assert_int_equal(dcu_698_comm_decode_set(0x4511, 3, eth_a3, sizeof(eth_a3)),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(dcu_698_comm_get_eth(0x4511, &e_reloaded), 0);
    assert_int_equal(e_reloaded.master_port, 9001);
}

static void test_negative_boundaries_and_dar7(void **state) {
    (void)state;
    dcu_698_comm_init(NULL);

    uint8_t buf[192];
    size_t len = 0;

    /* 未知 OI -> DAR 6 (NOT_FOUND) */
    assert_int_equal(dcu_698_comm_encode_attr(0x4502, 2, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_NOT_FOUND);
    assert_int_equal(dcu_698_comm_decode_set(0x4518, 3, buf, 4), EDGE_SLAVE_SRC_NOT_FOUND);

    /* 未实现类属性 -> DAR 5，未定义属性 -> DAR 4，只读 A1 SET -> DAR 3 */
    assert_int_equal(dcu_698_comm_encode_attr(0x4500, 7, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_comm_encode_attr(0x4500, 12, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_comm_encode_attr(0x4510, 5, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_comm_encode_attr(0x4510, 6, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_comm_decode_set(0x4500, 1, buf, 4), EDGE_SLAVE_SRC_DENIED);

    /* cap 不足返回 -1 */
    assert_int_equal(dcu_698_comm_encode_attr(0x4500, 2, buf, 4, &len), -1);
    assert_int_equal(dcu_698_comm_encode_attr(0x4510, 4, buf, 8, &len), -1);

    /* 错 tag / 截断 / 越界端口 0 -> DAR 7 (EDGE_SLAVE_SRC_TYPE_MISMATCH) */
    uint8_t bad_tag[] = {DLT698_TAG_UNSIGNED, 1};
    assert_int_equal(dcu_698_comm_decode_set(0x4500, 2, bad_tag, sizeof(bad_tag)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    uint8_t zero_port_a3[] = {
        DLT698_TAG_ARRAY, 1,
        DLT698_TAG_STRUCTURE, 2,
        DLT698_TAG_OCTET_STRING, 4, 192, 168, 1, 2,
        DLT698_TAG_LONG_UNSIGNED, 0x00, 0x00,
    };
    assert_int_equal(dcu_698_comm_decode_set(0x4500, 3, zero_port_a3, sizeof(zero_port_a3)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_comm_decode_set(0x4510, 3, zero_port_a3, sizeof(zero_port_a3)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    /* 截断与尾随字节 -> DAR 7 */
    assert_int_equal(dcu_698_comm_decode_set(0x4510, 3, zero_port_a3, sizeof(zero_port_a3) - 1),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    uint8_t trailing_a3[] = {
        DLT698_TAG_ARRAY, 1,
        DLT698_TAG_STRUCTURE, 2,
        DLT698_TAG_OCTET_STRING, 4, 192, 168, 1, 2,
        DLT698_TAG_LONG_UNSIGNED, 0x13, 0x88,
        0xFF,
    };
    assert_int_equal(dcu_698_comm_decode_set(0x4510, 3, trailing_a3, sizeof(trailing_a3)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    /* 字符串定长截断保护 */
    dcu_comm_wireless_cfg_t long_apn = {
        .master_ip = {1, 1, 1, 1},
        .master_port = 1234,
        .heartbeat_s = 30,
    };
    memset(long_apn.apn, 'A', sizeof(long_apn.apn) - 1);
    long_apn.apn[sizeof(long_apn.apn) - 1] = 'Z';
    assert_int_equal(dcu_698_comm_set_wireless(0x4500, &long_apn), 0);
    dcu_comm_wireless_cfg_t got = {0};
    assert_int_equal(dcu_698_comm_get_wireless(0x4500, &got), 0);
    assert_int_equal(strlen(got.apn), DCU_COMM_APN_MAX - 1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_wireless_set_get_and_axdr_roundtrip),
        cmocka_unit_test(test_eth_set_get_and_axdr_roundtrip),
        cmocka_unit_test(test_persistence_save_load_and_rollback),
        cmocka_unit_test(test_negative_boundaries_and_dar7),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
