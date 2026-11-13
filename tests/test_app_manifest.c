#include <cmocka.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "edgeos/app_manifest.h"

static const char *valid_manifest =
    "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"publisher_id\":\"acme\","
    "\"language\":\"c\",\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
    "\"arch\":\"aarch64\",\"license\":\"MIT\",\"resources\":{\"priority\":9},"
    "\"limits\":{\"heapKb\":256,\"storageKb\":64,\"cpuMsPerSec\":50,"
    "\"callsPerSec\":100,\"publishPerSec\":10,\"pidsMax\":64},"
    "\"capabilities\":{\"tags\":{\"read\":[\"U_A\",\"U_B\"],\"write\":[],"
    "\"reason\":\"reads voltage\"},"
    "\"points\":{\"publish\":[\"app.demo-app.value\"],\"reason\":\"publishes stats\"},"
    "\"kv\":{\"reason\":\"caches config\"},\"log\":true}}";

static const char *minimal_manifest =
    "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
    "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
    "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
    "\"publishPerSec\":1}}";

static void build_manifest(char *buf, size_t size, const char *limits_extra, const char *extra) {
    snprintf(buf, size,
             "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
             "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
             "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
             "\"publishPerSec\":1%s}%s}",
             limits_extra, extra);
}

static void test_valid_manifest(void **state) {
    (void)state;
    char reason[128];
    assert_int_equal(edge_app_manifest_validate_json(valid_manifest, reason, sizeof(reason)), 0);

    app_spec_t spec;
    assert_int_equal(edge_app_manifest_parse(valid_manifest, &spec, reason, sizeof(reason)), 0);
    assert_int_equal(spec.abi, 1);
    assert_string_equal(spec.id, "demo-app");
    assert_string_equal(spec.version, "1.0.0");
    assert_string_equal(spec.publisher_id, "acme");
    assert_string_equal(spec.language, "c");
    assert_string_equal(spec.execution_profile, "wasm");
    assert_string_equal(spec.entry, "app.wasm");
    assert_string_equal(spec.arch, "aarch64");
    assert_string_equal(spec.license, "MIT");
    assert_int_equal(spec.priority, 9);
    assert_int_equal(spec.limits.heap_kb, 256);
    assert_int_equal(spec.limits.storage_kb, 64);
    assert_int_equal(spec.limits.cpu_ms_per_sec, 50);
    assert_int_equal(spec.limits.calls_per_sec, 100);
    assert_int_equal(spec.limits.publish_per_sec, 10);
    assert_int_equal(spec.limits.pids_max, 64);
    assert_int_equal(spec.capabilities.read_count, 2);
    assert_string_equal(spec.capabilities.read[0], "U_A");
    assert_string_equal(spec.capabilities.read[1], "U_B");
    assert_int_equal(spec.capabilities.publish_count, 1);
    assert_string_equal(spec.capabilities.publish[0], "app.demo-app.value");
    assert_int_equal(spec.capabilities.kv, 1);
    assert_int_equal(spec.capabilities.log, 1);
    assert_string_equal(spec.capabilities.tags_reason, "reads voltage");
    assert_string_equal(spec.capabilities.points_reason, "publishes stats");
    assert_string_equal(spec.capabilities.kv_reason, "caches config");
}

static void test_optional_fields_take_defaults(void **state) {
    (void)state;
    char reason[128];
    app_spec_t spec;
    assert_int_equal(edge_app_manifest_parse(minimal_manifest, &spec, reason, sizeof(reason)), 0);
    assert_string_equal(spec.arch, APP_SPEC_DEFAULT_ARCH);
    assert_string_equal(spec.license, APP_SPEC_DEFAULT_LICENSE);
    assert_int_equal(spec.priority, APP_SPEC_DEFAULT_PRIORITY);
    assert_int_equal(spec.limits.pids_max, APP_SPEC_DEFAULT_PIDS_MAX);
    assert_string_equal(spec.publisher_id, "");
    assert_int_equal(spec.capabilities.read_count, 0);
    assert_int_equal(spec.capabilities.publish_count, 0);
    assert_int_equal(spec.capabilities.kv, 0);
    assert_int_equal(spec.capabilities.log, 0);
    assert_string_equal(spec.capabilities.tags_reason, "");
    assert_string_equal(spec.capabilities.points_reason, "");
    assert_string_equal(spec.capabilities.kv_reason, "");
    assert_int_equal(spec.preview_count, 0);
}

static void test_rejects_invalid_arch(void **state) {
    (void)state;
    char buf[512];
    char reason[128];
    app_spec_t spec;
    build_manifest(buf, sizeof(buf), "", ",\"arch\":\"mips\"");
    assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "arch is invalid");
}

static void test_rejects_invalid_priority(void **state) {
    (void)state;
    static const char *values[] = {"-1", "11", "5.5", "\"high\""};
    char buf[512];
    char reason[128];
    app_spec_t spec;
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        char extra[64];
        snprintf(extra, sizeof(extra), ",\"resources\":{\"priority\":%s}", values[i]);
        build_manifest(buf, sizeof(buf), "", extra);
        assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), -1);
        assert_string_equal(reason, "resources.priority must be an integer between 0 and 10");
    }
}

static void test_rejects_invalid_pids_max(void **state) {
    (void)state;
    static const char *values[] = {"0", "1025", "1.5", "\"many\""};
    char buf[512];
    char reason[128];
    app_spec_t spec;
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        char limits_extra[64];
        snprintf(limits_extra, sizeof(limits_extra), ",\"pidsMax\":%s", values[i]);
        build_manifest(buf, sizeof(buf), limits_extra, "");
        assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), -1);
        assert_string_equal(reason, "limits.pidsMax must be an integer between 1 and 1024");
    }
}

static void test_rejects_invalid_license_and_reason(void **state) {
    (void)state;
    char buf[768];
    char reason[128];
    app_spec_t spec;

    build_manifest(buf, sizeof(buf), "", ",\"license\":\"\"");
    assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "license must be a non-empty string");

    build_manifest(buf, sizeof(buf), "",
                   ",\"capabilities\":{\"tags\":{\"read\":[\"U_A\"],\"reason\":\"\"}}");
    assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "capabilities.tags.reason must be a non-empty string");
}

static void test_license_parses(void **state) {
    (void)state;
    char buf[512];
    char reason[128];
    app_spec_t spec;
    build_manifest(buf, sizeof(buf), "", ",\"license\":\"Apache-2.0\"");
    assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), 0);
    assert_string_equal(spec.license, "Apache-2.0");
}

static void test_preview_requires_policy_and_known_names(void **state) {
    (void)state;
    char buf[768];
    char reason[128];
    app_spec_t spec;

    build_manifest(buf, sizeof(buf), "", ",\"preview\":[\"ui.pages\",\"app.functions\"]");
    assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "DENIED_PREVIEW: ui.pages");

    app_manifest_policy_t allow = {0};
    allow.allow_preview_apps = 1;
    assert_int_equal(
        edge_app_manifest_parse_with_policy(buf, &allow, &spec, reason, sizeof(reason)), 0);
    assert_int_equal(spec.preview_count, 2);
    assert_string_equal(spec.preview[0], "ui.pages");
    assert_string_equal(spec.preview[1], "app.functions");

    build_manifest(buf, sizeof(buf), "", ",\"preview\":[\"bogus.preview\"]");
    assert_int_equal(
        edge_app_manifest_validate_json_with_policy(buf, &allow, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "unknown preview: bogus.preview");
}

static void test_rejects_unknown_fields(void **state) {
    (void)state;
    char buf[768];
    char reason[128];
    app_spec_t spec;

    build_manifest(buf, sizeof(buf), "", ",\"bogus\":1");
    assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "unknown field: bogus");

    build_manifest(buf, sizeof(buf), "", ",\"resources\":{\"priority\":5,\"bogus\":1}");
    assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "unknown field: bogus");
}

static void test_rejects_wildcard_and_unknown_capability(void **state) {
    (void)state;
    char reason[128];
    const char *wildcard =
        "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1},\"capabilities\":{\"tags\":{\"read\":[\"U_*\"]}}}";
    assert_int_equal(edge_app_manifest_validate_json(wildcard, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "wildcards are not allowed");

    const char *unknown =
        "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1},\"capabilities\":{\"net\":{}}}";
    assert_int_equal(edge_app_manifest_validate_json(unknown, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "unknown field: net");
}

static void test_requires_points_must_be_covered(void **state) {
    (void)state;
    char reason[128];
    const char *covered =
        "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1},\"capabilities\":{\"tags\":{\"read\":[\"U_A\"]}},"
        "\"requires\":{\"points\":[\"U_A\"]}}";
    assert_int_equal(edge_app_manifest_validate_json(covered, reason, sizeof(reason)), 0);

    const char *uncovered =
        "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1},\"capabilities\":{\"tags\":{\"read\":[\"U_A\"]}},"
        "\"requires\":{\"points\":[\"U_B\"]}}";
    assert_int_equal(edge_app_manifest_validate_json(uncovered, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "requires.points must be covered by tags.read");

    const char *no_read =
        "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1},\"requires\":{\"points\":[\"U_A\"]}}";
    assert_int_equal(edge_app_manifest_validate_json(no_read, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "requires.points must be covered by tags.read");

    const char *wildcard =
        "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1},\"requires\":{\"points\":[\"U_*\"]}}";
    assert_int_equal(edge_app_manifest_validate_json(wildcard, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "wildcards are not allowed");

    app_spec_t spec;
    assert_int_equal(edge_app_manifest_parse(covered, &spec, reason, sizeof(reason)), 0);
    assert_int_equal(spec.requires_points_count, 1);
    assert_string_equal(spec.requires_points[0], "U_A");
}

static void test_rejects_old_abi_and_bad_publish_namespace(void **state) {
    (void)state;
    char reason[128];
    const char *missing_abi =
        "{\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1}}";
    assert_int_equal(edge_app_manifest_validate_json(missing_abi, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "abi must be 1");

    const char *old_abi =
        "{\"abi\":2,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1}}";
    assert_int_equal(edge_app_manifest_validate_json(old_abi, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "abi must be 1");

    const char *fractional_abi =
        "{\"abi\":1.5,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1}}";
    assert_int_equal(edge_app_manifest_validate_json(fractional_abi, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "abi must be 1");

    const char *bad_publish =
        "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1},\"capabilities\":{\"points\":{\"publish\":[\"U_A\"]}}}";
    assert_int_equal(edge_app_manifest_validate_json(bad_publish, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "points.publish outside app namespace");
}

static const char *points_manifest_template =
    "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
    "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
    "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
    "\"publishPerSec\":1},"
    "\"capabilities\":{\"points\":{\"publish\":[\"app.demo-app.sag_count\"],"
    "\"reason\":\"own points\"}},"
    "\"contributes\":{\"telemetry\":{\"points\":[%s]}}}";

static void test_contributed_points_parse(void **state) {
    (void)state;
    char buf[1024];
    snprintf(buf, sizeof(buf), points_manifest_template,
             "{\"name\":\"sag_count\",\"type\":\"number\",\"unit\":\"count\",\"contractVersion\":1,"
             "\"retention\":\"latest\",\"staleAfterMs\":5000,\"stability\":\"stable\","
             "\"source\":\"derived\"}");
    char reason[160];
    app_spec_t spec;
    assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), 0);
    assert_int_equal(spec.points_count, 1);
    assert_string_equal(spec.points[0].name, "sag_count");
    assert_string_equal(spec.points[0].unit, "count");
    assert_int_equal(spec.points[0].type, APP_POINT_NUMBER);
    assert_int_equal(spec.points[0].contract_version, 1);
    assert_int_equal(spec.points[0].retention, APP_POINT_LATEST);
    assert_int_equal(spec.points[0].stale_after_ms, 5000);
    assert_int_equal(spec.points[0].stability, APP_POINT_STABLE);
    assert_int_equal(spec.points[0].source, APP_POINT_DERIVED);
}

static void test_contributed_point_defaults(void **state) {
    (void)state;
    char buf[1024];
    snprintf(buf, sizeof(buf), points_manifest_template,
             "{\"name\":\"sag_count\",\"type\":\"integer\",\"contractVersion\":2}");
    char reason[160];
    app_spec_t spec;
    assert_int_equal(edge_app_manifest_parse(buf, &spec, reason, sizeof(reason)), 0);
    assert_int_equal(spec.points[0].type, APP_POINT_INTEGER);
    assert_int_equal(spec.points[0].contract_version, 2);
    assert_int_equal(spec.points[0].stale_after_ms, APP_SPEC_POINT_STALE_DEFAULT_MS);
    assert_int_equal(spec.points[0].retention, APP_POINT_LATEST);
    assert_int_equal(spec.points[0].stability, APP_POINT_STABLE);
    assert_int_equal(spec.points[0].source, APP_POINT_MEASURED);
    assert_string_equal(spec.points[0].unit, "");
}

static void test_contributed_points_must_be_covered(void **state) {
    (void)state;
    const char *manifest =
        "{\"abi\":1,\"id\":\"demo-app\",\"version\":\"1.0.0\",\"language\":\"c\","
        "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
        "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
        "\"publishPerSec\":1},"
        "\"contributes\":{\"telemetry\":{\"points\":[{\"name\":\"sag_count\","
        "\"type\":\"number\",\"contractVersion\":1}]}}}";
    char reason[200];
    assert_int_equal(edge_app_manifest_validate_json(manifest, reason, sizeof(reason)), -1);
    assert_non_null(strstr(reason, "must be covered by points.publish"));
}

static void expect_point_rejected(const char *point_json, const char *expected) {
    char buf[1024];
    snprintf(buf, sizeof(buf), points_manifest_template, point_json);
    char reason[200];
    int rc = edge_app_manifest_validate_json(buf, reason, sizeof(reason));
    assert_int_equal(rc, -1);
    assert_non_null(strstr(reason, expected));
}

static void test_contributed_points_rejections(void **state) {
    (void)state;
    expect_point_rejected(
        "{\"name\":\"sag_count\",\"type\":\"currency\",\"contractVersion\":1}",
        "point type is invalid");
    expect_point_rejected("{\"name\":\"SagCount\",\"type\":\"number\",\"contractVersion\":1}",
                          "point name must match");
    expect_point_rejected(
        "{\"name\":\"sag_count\",\"type\":\"number\",\"contractVersion\":0}",
        "contractVersion must be an integer");
    expect_point_rejected(
        "{\"name\":\"sag_count\",\"type\":\"number\",\"contractVersion\":1,"
        "\"staleAfterMs\":50}",
        "staleAfterMs must be an integer");
    expect_point_rejected(
        "{\"name\":\"sag_count\",\"type\":\"number\",\"contractVersion\":1,\"scale\":2}",
        "unknown field");
    expect_point_rejected(
        "{\"name\":\"sag_count\",\"type\":\"number\",\"contractVersion\":1},"
        "{\"name\":\"sag_count\",\"type\":\"number\",\"contractVersion\":1}",
        "duplicate point name");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_valid_manifest),
        cmocka_unit_test(test_optional_fields_take_defaults),
        cmocka_unit_test(test_rejects_invalid_arch),
        cmocka_unit_test(test_rejects_invalid_priority),
        cmocka_unit_test(test_rejects_invalid_pids_max),
        cmocka_unit_test(test_rejects_invalid_license_and_reason),
        cmocka_unit_test(test_license_parses),
        cmocka_unit_test(test_preview_requires_policy_and_known_names),
        cmocka_unit_test(test_rejects_unknown_fields),
        cmocka_unit_test(test_rejects_wildcard_and_unknown_capability),
        cmocka_unit_test(test_requires_points_must_be_covered),
        cmocka_unit_test(test_rejects_old_abi_and_bad_publish_namespace),
        cmocka_unit_test(test_contributed_points_parse),
        cmocka_unit_test(test_contributed_point_defaults),
        cmocka_unit_test(test_contributed_points_must_be_covered),
        cmocka_unit_test(test_contributed_points_rejections),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
