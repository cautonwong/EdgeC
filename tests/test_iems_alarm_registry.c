/**
 * @file test_iems_alarm_registry.c
 * @brief P5-04a alarm definition registry: real-file counts, field mapping
 *        including reasonCode, class-sensitive tag lookup, strict JSON
 *        rejection, duplicate-key lookup order and capacity.
 */

#include <cmocka.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iems_alarm_registry.h"
#include "iems_config_loader.h"

/* Byte-identical copy of iems_v3 zh_CN alarm_def.json: 3,167,403 bytes,
 * sha256 13703cee8acfc606c3d531ffe3fc0e17df573efe8d1ada0a4992a47f89546b5a.
 * CMake defines this to tests/fixtures; a missing file is a test failure, not
 * a skip, so CI always runs the real-file assertions. */
#ifndef IEMS_ALARM_REAL_DIR
#error "IEMS_ALARM_REAL_DIR must point at tests/fixtures"
#endif

_Static_assert(IEMS_ALARM_MAX_DEFS > 3821u, "zh_CN snapshot (3821 rows) must fit");
_Static_assert(IEMS_ALARM_MAX_VENDOR_MODELS > 16u, "zh_CN snapshot (16 entries) must fit");

static iems_alarm_registry_t g_reg;

typedef struct {
    const char *text;
} mem_source_t;

static iems_config_source_rc_t mem_read(void *ctx, const char *relative_path, char **data,
                                        size_t *len, char *err, size_t err_size) {
    mem_source_t *src = ctx;
    if (strcmp(relative_path, "alarm_def.json") != 0) {
        snprintf(err, err_size, "ERROR %s: unexpected path", relative_path);
        return IEMS_CONFIG_SOURCE_ERROR;
    }
    size_t size = strlen(src->text);
    char *copy = malloc(size + 1);
    if (!copy) {
        snprintf(err, err_size, "ERROR %s: out of memory", relative_path);
        return IEMS_CONFIG_SOURCE_ERROR;
    }
    memcpy(copy, src->text, size + 1);
    *data = copy;
    *len = size;
    return IEMS_CONFIG_SOURCE_OK;
}

static iems_config_source_t mem_port(mem_source_t *src) {
    iems_config_source_t port;
    port.read_file = mem_read;
    port.list_dir = NULL;
    port.ctx = src;
    return port;
}

static int load_text(const char *text, char *err, size_t err_size) {
    mem_source_t src = {.text = text};
    iems_config_source_t port = mem_port(&src);
    return iems_alarm_registry_load(&port, &g_reg, err, err_size);
}

static size_t distinct_vendor_model_count(const iems_alarm_registry_t *reg) {
    size_t count = 0;
    for (uint32_t i = 0; i < reg->vendor_model_count; i++) {
        bool seen = false;
        for (uint32_t j = 0; j < i; j++) {
            if (strcmp(reg->vendor_models[i].vendor, reg->vendor_models[j].vendor) == 0 &&
                strcmp(reg->vendor_models[i].model, reg->vendor_models[j].model) == 0) {
                seen = true;
                break;
            }
        }
        if (!seen) count++;
    }
    return count;
}

static const iems_alarm_vendor_model_t *find_vendor_model(const iems_alarm_registry_t *reg,
                                                          const char *model,
                                                          const char *class_type) {
    for (uint32_t i = 0; i < reg->vendor_model_count; i++) {
        const iems_alarm_vendor_model_t *vm = &reg->vendor_models[i];
        if (strcmp(vm->model, model) == 0 && strcmp(vm->class_type, class_type) == 0) return vm;
    }
    return NULL;
}

/** Distinct (class_type, model, tag, code, reasonCode) rows for PCS FaultCode1. */
static size_t faultcode1_variant_count(const iems_alarm_registry_t *reg) {
    size_t count = 0;
    for (uint32_t i = 0; i < reg->def_count; i++) {
        const iems_alarm_def_t *def = &reg->defs[i];
        if (strcmp(def->class_type, "PCS") != 0 || strcmp(def->model, "PCS") != 0 ||
            strcmp(def->tag, "FaultCode1") != 0 || def->code != 10101u) {
            continue;
        }
        bool duplicate = false;
        for (uint32_t j = 0; j < i; j++) {
            const iems_alarm_def_t *prev = &reg->defs[j];
            if (prev->code == def->code && prev->reason_code == def->reason_code &&
                strcmp(prev->class_type, def->class_type) == 0 &&
                strcmp(prev->model, def->model) == 0 && strcmp(prev->tag, def->tag) == 0) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) count++;
    }
    return count;
}

static const char k_two_vendor_models[] =
    "{\"lang\":\"zh_CN\",\"vendor_models\":["
    "{\"type\":\"EMS\",\"vendor\":\"V\",\"model\":\"M\",\"tags\":["
    "{\"tag\":\"Online\",\"alarmCode\":10000,\"alarmInfo\":\"n1\","
    "\"categoryDisplay\":\"c\",\"severityDisplay\":\"\",\"entityTypeDisplay\":\"e\","
    "\"reasonCode\":0,\"reason\":\"r1\",\"additionalInfo\":\"\",\"solution\":\"s1\"},"
    "{\"tag\":\"Other\",\"alarmCode\":10001,\"alarmInfo\":\"n2\","
    "\"categoryDisplay\":\"c\",\"severityDisplay\":\"s\",\"entityTypeDisplay\":\"e\","
    "\"reasonCode\":2,\"reason\":\"r2\",\"additionalInfo\":\"x\","
    "\"additionalInfoParas\":\"p\",\"solution\":\"\"}]},"
    "{\"type\":\"PV\",\"vendor\":\"V\",\"model\":\"PV\",\"tags\":[]}"
    "]}";

static void test_synthetic_load_and_lookup(void **state) {
    (void)state;
    char err[512] = {0};
    int rc = load_text(k_two_vendor_models, err, sizeof(err));
    if (rc != 0) print_message("load failed: %s\n", err);
    assert_int_equal(rc, 0);

    assert_string_equal(g_reg.lang, "zh_CN");
    assert_int_equal(g_reg.vendor_model_count, 2);
    assert_int_equal(g_reg.def_count, 2);
    assert_int_equal(g_reg.vendor_models[0].def_start, 0);
    assert_int_equal(g_reg.vendor_models[0].def_count, 2);
    assert_int_equal(g_reg.vendor_models[1].def_start, 2);
    assert_int_equal(g_reg.vendor_models[1].def_count, 0);

    const iems_alarm_def_t *def = iems_alarm_registry_find(&g_reg, "EMS", "V", "M", "Online");
    assert_non_null(def);
    assert_string_equal(def->class_type, "EMS");
    assert_int_equal(def->code, 10000);
    assert_int_equal(def->reason_code, 0);
    assert_string_equal(def->name, "n1");
    assert_string_equal(def->category, "c");
    assert_string_equal(def->severity, "");
    assert_string_equal(def->entity, "e");
    assert_string_equal(def->reason, "r1");
    assert_string_equal(def->solution, "s1");

    const iems_alarm_def_t *other = iems_alarm_registry_find(&g_reg, "EMS", "V", "M", "Other");
    assert_non_null(other);
    assert_int_equal(other->reason_code, 2);
    assert_string_equal(other->solution, "");

    const iems_alarm_def_t *by_code = iems_alarm_registry_find_by_code(&g_reg, 10001, 2);
    assert_ptr_equal(by_code, other);
    assert_string_equal(by_code->tag, "Other");

    assert_null(iems_alarm_registry_find(&g_reg, "PV", "V", "M", "Online"));
    assert_null(iems_alarm_registry_find(&g_reg, "EMS", "V", "M", "Missing"));
    assert_null(iems_alarm_registry_find(&g_reg, "EMS", "V", "M", NULL));
    assert_null(iems_alarm_registry_find(&g_reg, NULL, "V", "M", "Online"));
    assert_null(iems_alarm_registry_find_by_code(&g_reg, 10001, 0));
    assert_null(iems_alarm_registry_find_by_code(&g_reg, 99999, 0));
}

static char *read_text_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    long size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    char *text = malloc((size_t)size + 1);
    if (!text) {
        fclose(file);
        return NULL;
    }
    size_t got = fread(text, 1, (size_t)size, file);
    int closed = fclose(file);
    if (closed != 0 || got != (size_t)size) {
        free(text);
        return NULL;
    }
    text[got] = '\0';
    return text;
}

static void test_real_alarm_def_counts(void **state) {
    (void)state;
    const char *path = IEMS_ALARM_REAL_DIR "/alarm_def.json";

    /* The shared config file source caps files at 1 MiB; the real alarm file
     * is ~3.1 MiB, so inject the bytes through the test source port. */
    char *text = read_text_file(path);
    if (!text) print_message("real alarm_def fixture is not readable: %s\n", path);
    assert_non_null(text);
    char err[512] = {0};
    int rc = load_text(text, err, sizeof(err));
    free(text);
    if (rc != 0) print_message("real load failed: %s\n", err);
    assert_int_equal(rc, 0);
    print_message("real alarm_def: lang=%s vendor_models=%u defs=%u distinct_vendor_model=%zu\n",
                  g_reg.lang, g_reg.vendor_model_count, g_reg.def_count,
                  distinct_vendor_model_count(&g_reg));

    assert_string_equal(g_reg.lang, "zh_CN");
    assert_int_equal(g_reg.vendor_model_count, 16);
    assert_int_equal(g_reg.def_count, 3821);
    assert_int_equal(distinct_vendor_model_count(&g_reg), 5);
    for (uint32_t i = 0; i < g_reg.vendor_model_count; i++) {
        assert_string_equal(g_reg.vendor_models[i].vendor, "Livoltek");
    }

    const iems_alarm_vendor_model_t *pcs = find_vendor_model(&g_reg, "PCS", "PCS");
    assert_non_null(pcs);
    assert_int_equal(pcs->def_count, 3650);
    const iems_alarm_vendor_model_t *ems = find_vendor_model(&g_reg, "EMS", "EMS");
    assert_non_null(ems);
    assert_int_equal(ems->def_count, 140);
    const iems_alarm_vendor_model_t *lc = find_vendor_model(&g_reg, "LC", "LC");
    assert_non_null(lc);
    assert_int_equal(lc->def_count, 19);
    const iems_alarm_vendor_model_t *pv = find_vendor_model(&g_reg, "PV", "PV");
    assert_non_null(pv);
    assert_int_equal(pv->def_count, 0);
    const iems_alarm_vendor_model_t *hybrid = find_vendor_model(&g_reg, "HYBRID", "HYBRID");
    assert_non_null(hybrid);
    assert_int_equal(hybrid->def_count, 0);

    assert_string_equal(g_reg.defs[0].vendor, "Livoltek");
    assert_string_equal(g_reg.defs[0].model, "EMS");
    assert_string_equal(g_reg.defs[0].class_type, "EMS");
    assert_string_equal(g_reg.defs[0].tag, "Online");
    assert_int_equal(g_reg.defs[0].code, 10000);
    assert_int_equal(g_reg.defs[0].reason_code, 0);
    assert_string_equal(g_reg.defs[0].name, "EMS控制器通讯故障");
    assert_string_equal(g_reg.defs[0].category, "通讯");
    assert_string_equal(g_reg.defs[0].severity, "严重");
    assert_string_equal(g_reg.defs[0].entity, "EMS控制器");
    assert_string_equal(g_reg.defs[0].reason, "EMS控制器通讯故障");
    assert_string_equal(g_reg.defs[0].solution, "排查现场网络,或者4G流量超上限");

    /* v3 keys its info lookup by <type>.<vendor>.<model>.<tag>, so the class
     * type is part of the identity even when vendor/model/tag repeat. */
    const iems_alarm_def_t *ems_online =
        iems_alarm_registry_find(&g_reg, "EMS", "Livoltek", "EMS", "Online");
    assert_non_null(ems_online);
    assert_int_equal(ems_online->code, 10000);
    assert_string_equal(ems_online->name, "EMS控制器通讯故障");
    const iems_alarm_def_t *lc_online =
        iems_alarm_registry_find(&g_reg, "LC", "Livoltek", "EMS", "Online");
    assert_non_null(lc_online);
    assert_int_equal(lc_online->code, 10001);
    assert_string_equal(lc_online->name, "LC控制器通讯故障");
    assert_true(ems_online->code != lc_online->code);

    /* (vendor, model, tag) and alarmCode are duplicated in the real file;
     * the registry preserves file order and returns the first match. */
    const iems_alarm_def_t *fault =
        iems_alarm_registry_find(&g_reg, "PCS", "Livoltek", "PCS", "FaultCode1");
    assert_non_null(fault);
    assert_int_equal(fault->code, 10101);
    assert_int_equal(fault->reason_code, 1);
    assert_string_equal(fault->name, "电网过压");
    assert_ptr_equal(iems_alarm_registry_find_by_code(&g_reg, 10101, 1), fault);

    /* v3 keys its code lookup by <alarmCode>.<reasonCode>: FaultCode1 carries
     * 365 reasonCode variants, each with its own name. */
    assert_int_equal(faultcode1_variant_count(&g_reg), 365);
    assert_string_equal(iems_alarm_registry_find_by_code(&g_reg, 10101, 2)->name, "电网暂态过压");
    assert_string_equal(iems_alarm_registry_find_by_code(&g_reg, 10101, 3)->name, "电网欠压");
    assert_string_equal(iems_alarm_registry_find_by_code(&g_reg, 10101, 806)->name,
                        "低压电池功能安全MOS异常保护");
    assert_null(iems_alarm_registry_find_by_code(&g_reg, 10101, 0));

    assert_null(iems_alarm_registry_find(&g_reg, "PCS", "Livoltek", "PCS", "NoSuchTag"));
    assert_null(iems_alarm_registry_find_by_code(&g_reg, 1, 0));
}

static void expect_rejected(const char *text) {
    char err[512] = {0};
    assert_int_equal(load_text(text, err, sizeof(err)), -1);
    assert_true(err[0] != '\0');
    assert_int_equal(g_reg.def_count, 0);
    assert_int_equal(g_reg.vendor_model_count, 0);
    assert_string_equal(g_reg.lang, "");
}

static void test_rejects_unknown_keys(void **state) {
    (void)state;
    expect_rejected("{\"lang\":\"zh_CN\",\"vendor_models\":[],\"extra\":1}");
    expect_rejected(
        "{\"lang\":\"zh_CN\",\"vendor_models\":["
        "{\"type\":\"EMS\",\"vendor\":\"V\",\"model\":\"M\",\"tags\":[],\"extra\":1}]}");
    expect_rejected(
        "{\"lang\":\"zh_CN\",\"vendor_models\":["
        "{\"type\":\"EMS\",\"vendor\":\"V\",\"model\":\"M\",\"tags\":["
        "{\"tag\":\"t\",\"alarmCode\":1,\"alarmInfo\":\"n\",\"categoryDisplay\":\"c\","
        "\"severityDisplay\":\"s\",\"entityTypeDisplay\":\"e\",\"reasonCode\":0,"
        "\"reason\":\"r\",\"additionalInfo\":\"\",\"solution\":\"\",\"extra\":1}]}]}");
}

static void test_rejects_wrong_types_and_missing_fields(void **state) {
    (void)state;
    expect_rejected("[]");
    expect_rejected("{\"lang\":1,\"vendor_models\":[]}");
    expect_rejected("{\"lang\":\"zh_CN\",\"vendor_models\":{}}");
    expect_rejected("{\"lang\":\"zh_CN\",\"vendor_models\":[[]]}");
    expect_rejected("{\"vendor_models\":[]}");
    expect_rejected("{\"lang\":\"zh_CN\"}");
    expect_rejected(
        "{\"lang\":\"zh_CN\",\"vendor_models\":["
        "{\"type\":\"EMS\",\"vendor\":\"V\",\"model\":\"M\"}]}");
    expect_rejected(
        "{\"lang\":\"zh_CN\",\"vendor_models\":["
        "{\"type\":\"EMS\",\"vendor\":\"V\",\"model\":\"M\",\"tags\":[\"x\"]}]}");
    expect_rejected(
        "{\"lang\":\"zh_CN\",\"vendor_models\":["
        "{\"type\":\"EMS\",\"vendor\":\"V\",\"model\":\"M\",\"tags\":["
        "{\"tag\":\"t\",\"alarmCode\":\"1\",\"alarmInfo\":\"n\","
        "\"categoryDisplay\":\"c\",\"severityDisplay\":\"s\","
        "\"entityTypeDisplay\":\"e\",\"reasonCode\":0,\"reason\":\"r\","
        "\"additionalInfo\":\"\",\"solution\":\"\"}]}]}");
    expect_rejected(
        "{\"lang\":\"zh_CN\",\"vendor_models\":["
        "{\"type\":\"EMS\",\"vendor\":\"V\",\"model\":\"M\",\"tags\":["
        "{\"tag\":\"t\",\"alarmCode\":1,\"alarmInfo\":\"n\","
        "\"categoryDisplay\":\"c\",\"severityDisplay\":\"s\","
        "\"entityTypeDisplay\":\"e\",\"reasonCode\":0,\"reason\":\"r\","
        "\"additionalInfo\":\"\"}]}]}");
}

static void test_rejects_strict_json_violations(void **state) {
    (void)state;
    /* duplicate key */
    expect_rejected("{\"lang\":\"zh_CN\",\"lang\":\"en_US\",\"vendor_models\":[]}");
    /* trailing data */
    expect_rejected("{\"lang\":\"zh_CN\",\"vendor_models\":[]} {}");
    /* UTF-8 BOM */
    expect_rejected("\xEF\xBB\xBF{\"lang\":\"zh_CN\",\"vendor_models\":[]}");
    /* invalid UTF-8 inside a string */
    expect_rejected("{\"lang\":\"\xFF\",\"vendor_models\":[]}");
}

static char *build_tags_doc(size_t tag_count) {
    static const char prefix[] =
        "{\"lang\":\"zh_CN\",\"vendor_models\":["
        "{\"type\":\"EMS\",\"vendor\":\"V\",\"model\":\"M\",\"tags\":[";
    static const char object[] =
        "{\"tag\":\"T\",\"alarmCode\":1,\"alarmInfo\":\"n\",\"categoryDisplay\":\"c\","
        "\"severityDisplay\":\"s\",\"entityTypeDisplay\":\"e\",\"reasonCode\":0,"
        "\"reason\":\"r\",\"additionalInfo\":\"\",\"solution\":\"\"}";
    static const char suffix[] = "]}]}";

    size_t prefix_len = strlen(prefix);
    size_t object_len = strlen(object);
    size_t suffix_len = strlen(suffix);
    size_t cap = prefix_len + tag_count * object_len + tag_count + suffix_len + 1;
    char *doc = malloc(cap);
    assert_non_null(doc);
    size_t off = 0;
    memcpy(doc, prefix, prefix_len);
    off += prefix_len;
    for (size_t i = 0; i < tag_count; i++) {
        if (i > 0) doc[off++] = ',';
        memcpy(doc + off, object, object_len);
        off += object_len;
    }
    memcpy(doc + off, suffix, suffix_len + 1);
    return doc;
}

static void test_definition_capacity_fails_closed(void **state) {
    (void)state;
    char err[512] = {0};

    char *doc = build_tags_doc(IEMS_ALARM_MAX_DEFS);
    int rc = load_text(doc, err, sizeof(err));
    free(doc);
    if (rc != 0) print_message("capacity load failed: %s\n", err);
    assert_int_equal(rc, 0);
    assert_int_equal(g_reg.def_count, IEMS_ALARM_MAX_DEFS);

    doc = build_tags_doc((size_t)IEMS_ALARM_MAX_DEFS + 1);
    assert_int_equal(load_text(doc, err, sizeof(err)), -1);
    free(doc);
    assert_int_equal(g_reg.def_count, 0);
}

static void test_string_arena_overflow_fails_closed(void **state) {
    (void)state;
    static const char prefix[] =
        "{\"lang\":\"zh_CN\",\"vendor_models\":["
        "{\"type\":\"EMS\",\"vendor\":\"V\",\"model\":\"M\",\"tags\":["
        "{\"tag\":\"t\",\"alarmCode\":1,\"alarmInfo\":\"n\",\"categoryDisplay\":\"c\","
        "\"severityDisplay\":\"s\",\"entityTypeDisplay\":\"e\",\"reasonCode\":0,"
        "\"reason\":\"r\",\"additionalInfo\":\"\",\"solution\":\"";
    static const char suffix[] = "\"}]}]}";
    size_t big = (size_t)IEMS_ALARM_MAX_STRINGS + 1;
    size_t prefix_len = strlen(prefix);
    size_t suffix_len = strlen(suffix);
    char *doc = malloc(prefix_len + big + suffix_len + 1);
    assert_non_null(doc);
    memcpy(doc, prefix, prefix_len);
    memset(doc + prefix_len, 'a', big);
    memcpy(doc + prefix_len + big, suffix, suffix_len + 1);

    char err[512] = {0};
    assert_int_equal(load_text(doc, err, sizeof(err)), -1);
    free(doc);
    assert_int_equal(g_reg.def_count, 0);
}

static void test_failed_reload_empties_registry(void **state) {
    (void)state;
    char err[512] = {0};
    assert_int_equal(load_text(k_two_vendor_models, err, sizeof(err)), 0);
    assert_int_equal(g_reg.def_count, 2);
    assert_non_null(iems_alarm_registry_find(&g_reg, "EMS", "V", "M", "Online"));

    assert_int_equal(load_text("{\"lang\":\"zh_CN\"}", err, sizeof(err)), -1);
    assert_int_equal(g_reg.def_count, 0);
    assert_null(iems_alarm_registry_find(&g_reg, "EMS", "V", "M", "Online"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_synthetic_load_and_lookup),
        cmocka_unit_test(test_real_alarm_def_counts),
        cmocka_unit_test(test_rejects_unknown_keys),
        cmocka_unit_test(test_rejects_wrong_types_and_missing_fields),
        cmocka_unit_test(test_rejects_strict_json_violations),
        cmocka_unit_test(test_definition_capacity_fails_closed),
        cmocka_unit_test(test_string_arena_overflow_fails_closed),
        cmocka_unit_test(test_failed_reload_empties_registry),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
