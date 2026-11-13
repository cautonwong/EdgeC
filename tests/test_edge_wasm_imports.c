/**
 * @file test_edge_wasm_imports.c
 * @brief Import whitelist validator tests over hand-built wasm modules.
 */

#include <cmocka.h>
#include <stdint.h>
#include <string.h>

#include "edge_wasm_imports.h"

static size_t put_name(uint8_t *buf, size_t pos, const char *text) {
    size_t length = strlen(text);
    buf[pos++] = (uint8_t)length;
    memcpy(buf + pos, text, length);
    return pos + length;
}

static size_t put_func_import(uint8_t *buf, size_t pos, const char *module, const char *name) {
    pos = put_name(buf, pos, module);
    pos = put_name(buf, pos, name);
    buf[pos++] = 0x00; /* kind: func */
    buf[pos++] = 0x00; /* type index */
    return pos;
}

static size_t build_module(uint8_t *buf, size_t capacity, const char *const *modules,
                           const char *const *names, int count) {
    (void)capacity;
    const uint8_t header[8] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    memcpy(buf, header, sizeof(header));
    size_t pos = sizeof(header);
    buf[pos++] = 0x02; /* import section */
    size_t size_pos = pos++;
    buf[pos++] = (uint8_t)count;
    for (int i = 0; i < count; i++) pos = put_func_import(buf, pos, modules[i], names[i]);
    buf[size_pos] = (uint8_t)(pos - size_pos - 1);
    return pos;
}

static void test_allowed_import(void **state) {
    (void)state;
    const char *allowed[] = {"host_get_tag", "point_resolve", "point_read", "point_publish",
                             "app_log", "app_now"};
    for (size_t i = 0; i < sizeof(allowed) / sizeof(allowed[0]); i++) {
        uint8_t module[128];
        const char *modules[] = {"env"};
        const char *names[] = {allowed[i]};
        size_t len = build_module(module, sizeof(module), modules, names, 1);
        char reason[EDGE_WASM_IMPORT_REASON_SIZE];
        assert_int_equal(edge_wasm_imports_validate(module, len, reason, sizeof(reason)), 0);
        assert_string_equal(reason, "");
    }
}

static void test_denied_import_names_the_pair(void **state) {
    (void)state;
    uint8_t module[128];
    const char *modules[] = {"env"};
    const char *names[] = {"fd_write"};
    size_t len = build_module(module, sizeof(module), modules, names, 1);
    char reason[EDGE_WASM_IMPORT_REASON_SIZE];
    assert_int_equal(edge_wasm_imports_validate(module, len, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "denied_import: env.fd_write");
}

static void test_denied_module(void **state) {
    (void)state;
    uint8_t module[128];
    const char *modules[] = {"wasi_snapshot_preview1"};
    const char *names[] = {"proc_exit"};
    size_t len = build_module(module, sizeof(module), modules, names, 1);
    char reason[EDGE_WASM_IMPORT_REASON_SIZE];
    assert_int_equal(edge_wasm_imports_validate(module, len, reason, sizeof(reason)), -1);
    assert_non_null(strstr(reason, "denied_import: wasi_snapshot_preview1.proc_exit"));
}

static void test_mixed_imports_rejected(void **state) {
    (void)state;
    uint8_t module[256];
    const char *modules[] = {"env", "env"};
    const char *names[] = {"ext_log", "evil"};
    size_t len = build_module(module, sizeof(module), modules, names, 2);
    char reason[EDGE_WASM_IMPORT_REASON_SIZE];
    assert_int_equal(edge_wasm_imports_validate(module, len, reason, sizeof(reason)), -1);
    assert_non_null(strstr(reason, "evil"));
}

static void test_bad_magic_and_truncation(void **state) {
    (void)state;
    uint8_t module[128];
    const char *modules[] = {"env"};
    const char *names[] = {"ext_log"};
    size_t len = build_module(module, sizeof(module), modules, names, 1);
    char reason[EDGE_WASM_IMPORT_REASON_SIZE];

    uint8_t bad_magic[8] = {0x01, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    assert_int_equal(edge_wasm_imports_validate(bad_magic, sizeof(bad_magic), reason,
                                                sizeof(reason)),
                     -1);
    assert_string_equal(reason, "invalid_wasm");

    assert_int_equal(edge_wasm_imports_validate(module, len - 2, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "invalid_wasm");
    assert_int_equal(edge_wasm_imports_validate(module, 4, reason, sizeof(reason)), -1);
}

static void test_header_only_is_valid(void **state) {
    (void)state;
    const uint8_t header[8] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    char reason[EDGE_WASM_IMPORT_REASON_SIZE];
    assert_int_equal(edge_wasm_imports_validate(header, sizeof(header), reason, sizeof(reason)),
                     0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_allowed_import),
        cmocka_unit_test(test_denied_import_names_the_pair),
        cmocka_unit_test(test_denied_module),
        cmocka_unit_test(test_mixed_imports_rejected),
        cmocka_unit_test(test_bad_magic_and_truncation),
        cmocka_unit_test(test_header_only_is_valid),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
