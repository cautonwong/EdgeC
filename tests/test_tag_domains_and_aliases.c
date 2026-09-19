/**
 * @file test_tag_domains_and_aliases.c
 * @brief AEP-352: Unit tests for TagID domain partitioning and legacy alias mapping
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>
#include <uv.h>

#include "datahub.h"
#include "shmring.h"

/* -------------------------------------------------------------------------- */
/* Domain Boundary Definitions                                                */
/* -------------------------------------------------------------------------- */

typedef enum {
    TAG_DOMAIN_CORE_STANDARD = 0,
    TAG_DOMAIN_DCU           = 1,
    TAG_DOMAIN_IEMS          = 2,
    TAG_DOMAIN_SMART_BOX     = 3,
    TAG_DOMAIN_NPU           = 4,
    TAG_DOMAIN_UNKNOWN       = -1
} tag_domain_t;

static tag_domain_t get_tag_domain(uint32_t tag_id) {
    uint32_t domain_prefix = (tag_id >> 16) & 0xFF;
    switch (domain_prefix) {
        case 0x00: return TAG_DOMAIN_CORE_STANDARD;
        case 0x01: return TAG_DOMAIN_DCU;
        case 0x02: return TAG_DOMAIN_IEMS;
        case 0x03: return TAG_DOMAIN_SMART_BOX;
        case 0x04: return TAG_DOMAIN_NPU;
        default:   return TAG_DOMAIN_UNKNOWN;
    }
}

/* -------------------------------------------------------------------------- */
/* Alias Table Definition                                                     */
/* -------------------------------------------------------------------------- */

typedef struct {
    const char *alias;
    uint32_t canonical_id;
    const char *canonical_name;
    tag_domain_t domain;
} tag_alias_entry_t;

static const tag_alias_entry_t g_test_aliases[] = {
    { "TAG_VOLTAGE_A",    1001,   "VOLTAGE_PHASE_A",             TAG_DOMAIN_CORE_STANDARD },
    { "TAG_VOLTAGE_B",    1002,   "VOLTAGE_PHASE_B",             TAG_DOMAIN_CORE_STANDARD },
    { "TAG_ACTIVE_POWER", 3001,   "ACTIVE_POWER",                TAG_DOMAIN_CORE_STANDARD },
    { "TAG_ESS_SOC",      11001,  "ESS_SOC",                     TAG_DOMAIN_CORE_STANDARD },
    { "DCU_FEEDER_LOSS",  65538,  "DCU_LINELOSS_PCT",            TAG_DOMAIN_DCU },
    { "BESS_MAX_CELL_V",  131075, "IEMS_BMS_MAX_CELL_VOLT",       TAG_DOMAIN_IEMS },
    { "PANEL_LEAKAGE_MA", 196609, "SMARTBOX_RESIDUAL_CURRENT_MA", TAG_DOMAIN_SMART_BOX },
};
static const size_t g_test_aliases_count = sizeof(g_test_aliases) / sizeof(g_test_aliases[0]);

static const tag_alias_entry_t *lookup_alias(const char *alias_name) {
    if (!alias_name) return NULL;
    for (size_t i = 0; i < g_test_aliases_count; i++) {
        if (strcmp(g_test_aliases[i].alias, alias_name) == 0) {
            return &g_test_aliases[i];
        }
    }
    return NULL;
}

/* -------------------------------------------------------------------------- */
/* CMocka Tests                                                               */
/* -------------------------------------------------------------------------- */

static void test_tag_domain_partitioning(void **state) {
    (void)state;

    // Core domain (0x00xxxx)
    assert_int_equal(get_tag_domain(1001), TAG_DOMAIN_CORE_STANDARD);
    assert_int_equal(get_tag_domain(3001), TAG_DOMAIN_CORE_STANDARD);
    assert_int_equal(get_tag_domain(11001), TAG_DOMAIN_CORE_STANDARD);
    assert_int_equal(get_tag_domain(0x00FFFF), TAG_DOMAIN_CORE_STANDARD);

    // DCU domain (0x01xxxx)
    assert_int_equal(get_tag_domain(0x010001), TAG_DOMAIN_DCU);
    assert_int_equal(get_tag_domain(0x010002), TAG_DOMAIN_DCU);
    assert_int_equal(get_tag_domain(0x01FFFF), TAG_DOMAIN_DCU);

    // IEMS domain (0x02xxxx)
    assert_int_equal(get_tag_domain(0x020001), TAG_DOMAIN_IEMS);
    assert_int_equal(get_tag_domain(0x020003), TAG_DOMAIN_IEMS);
    assert_int_equal(get_tag_domain(0x02FFFF), TAG_DOMAIN_IEMS);

    // SmartBox domain (0x03xxxx)
    assert_int_equal(get_tag_domain(0x030001), TAG_DOMAIN_SMART_BOX);
    assert_int_equal(get_tag_domain(0x030002), TAG_DOMAIN_SMART_BOX);
    assert_int_equal(get_tag_domain(0x03FFFF), TAG_DOMAIN_SMART_BOX);

    // NPU domain (0x04xxxx)
    assert_int_equal(get_tag_domain(0x040001), TAG_DOMAIN_NPU);
    assert_int_equal(get_tag_domain(0x040002), TAG_DOMAIN_NPU);
    assert_int_equal(get_tag_domain(0x04FFFF), TAG_DOMAIN_NPU);

    // Unassigned domain (0x05xxxx and above)
    assert_int_equal(get_tag_domain(0x050001), TAG_DOMAIN_UNKNOWN);
}

static void test_legacy_alias_resolution(void **state) {
    (void)state;

    // 1. Resolve standard voltage alias
    const tag_alias_entry_t *entry = lookup_alias("TAG_VOLTAGE_A");
    assert_non_null(entry);
    assert_int_equal(entry->canonical_id, 1001);
    assert_string_equal(entry->canonical_name, "VOLTAGE_PHASE_A");
    assert_int_equal(entry->domain, TAG_DOMAIN_CORE_STANDARD);

    // 2. Resolve DCU line loss alias
    entry = lookup_alias("DCU_FEEDER_LOSS");
    assert_non_null(entry);
    assert_int_equal(entry->canonical_id, 0x010002);
    assert_string_equal(entry->canonical_name, "DCU_LINELOSS_PCT");
    assert_int_equal(entry->domain, TAG_DOMAIN_DCU);

    // 3. Resolve IEMS BMS cell voltage alias
    entry = lookup_alias("BESS_MAX_CELL_V");
    assert_non_null(entry);
    assert_int_equal(entry->canonical_id, 0x020003);
    assert_string_equal(entry->canonical_name, "IEMS_BMS_MAX_CELL_VOLT");
    assert_int_equal(entry->domain, TAG_DOMAIN_IEMS);

    // 4. Resolve SmartBox leakage current alias
    entry = lookup_alias("PANEL_LEAKAGE_MA");
    assert_non_null(entry);
    assert_int_equal(entry->canonical_id, 0x030001);
    assert_string_equal(entry->canonical_name, "SMARTBOX_RESIDUAL_CURRENT_MA");
    assert_int_equal(entry->domain, TAG_DOMAIN_SMART_BOX);

    // 5. Non-existent alias
    assert_null(lookup_alias("NON_EXISTENT_ALIAS"));
}

static int domain_tag_map_fn(uint32_t tag_id) {
    // Domain partitioned hash/modulo slot index mapping
    return (int)(tag_id % 1024);
}

static void test_datahub_partitioned_tag_telemetry(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    // Initialize DataHub with domain tag mapping function
    int rc = datahub_init(&hub, &loop, "domain_alias_hub", 1024, 1024, NULL, 0, NULL, domain_tag_map_fn, NULL);
    assert_int_equal(rc, 0);

    // 1. Core domain tag write/read
    pipe_msg_t msg_core;
    pipe_msg_init_scalar(&msg_core, 1001, 220.5, 1000);
    assert_int_equal(datahub_write_shadow(&hub, &msg_core), 0);

    // 2. DCU domain tag write/read (0x010002 LineLoss)
    pipe_msg_t msg_dcu;
    pipe_msg_init_scalar(&msg_dcu, 65538, 3.42, 1000);
    assert_int_equal(datahub_write_shadow(&hub, &msg_dcu), 0);

    // 3. IEMS domain tag write/read (0x020003 BMS Cell Volt)
    pipe_msg_t msg_iems;
    pipe_msg_init_scalar(&msg_iems, 131075, 3.652, 1000);
    assert_int_equal(datahub_write_shadow(&hub, &msg_iems), 0);

    // 4. SmartBox domain tag write/read (0x030001 Leakage Current)
    pipe_msg_t msg_box;
    pipe_msg_init_scalar(&msg_box, 196609, 14.8, 1000);
    assert_int_equal(datahub_write_shadow(&hub, &msg_box), 0);

    // Read back and assert physical truth
    pipe_msg_t out;
    assert_int_equal(datahub_read_shadow(&hub, 1001, &out), 0);
    assert_float_equal(out.payload.val.f, 220.5, 0.001);

    assert_int_equal(datahub_read_shadow(&hub, 65538, &out), 0);
    assert_float_equal(out.payload.val.f, 3.42, 0.001);

    assert_int_equal(datahub_read_shadow(&hub, 131075, &out), 0);
    assert_float_equal(out.payload.val.f, 3.652, 0.001);

    assert_int_equal(datahub_read_shadow(&hub, 196609, &out), 0);
    assert_float_equal(out.payload.val.f, 14.8, 0.001);

    // Clean up
    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT));
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_tag_domain_partitioning),
        cmocka_unit_test(test_legacy_alias_resolution),
        cmocka_unit_test(test_datahub_partitioned_tag_telemetry),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
