/**
 * @file test_tag_collision_checker.c
 * @brief AEP-355: Unit and integration tests for cross-product Tag collision checker and domain boundaries
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

/* -------------------------------------------------------------------------- */
/* Domain Range Definitions                                                   */
/* -------------------------------------------------------------------------- */

#define DOMAIN_CORE_MIN      0x000000
#define DOMAIN_CORE_MAX      0x00FFFF

#define DOMAIN_DCU_MIN       0x010000
#define DOMAIN_DCU_MAX       0x01FFFF

#define DOMAIN_IEMS_MIN      0x020000
#define DOMAIN_IEMS_MAX      0x02FFFF

#define DOMAIN_SMARTBOX_MIN  0x030000
#define DOMAIN_SMARTBOX_MAX  0x03FFFF

typedef enum {
    DOMAIN_TYPE_CORE      = 0,
    DOMAIN_TYPE_DCU       = 1,
    DOMAIN_TYPE_IEMS      = 2,
    DOMAIN_TYPE_SMARTBOX  = 3,
    DOMAIN_TYPE_UNKNOWN   = 4
} domain_type_t;

static domain_type_t get_domain_from_tag_id(uint32_t tag_id) {
    if (tag_id <= DOMAIN_CORE_MAX) return DOMAIN_TYPE_CORE;
    if (tag_id >= DOMAIN_DCU_MIN && tag_id <= DOMAIN_DCU_MAX) return DOMAIN_TYPE_DCU;
    if (tag_id >= DOMAIN_IEMS_MIN && tag_id <= DOMAIN_IEMS_MAX) return DOMAIN_TYPE_IEMS;
    if (tag_id >= DOMAIN_SMARTBOX_MIN && tag_id <= DOMAIN_SMARTBOX_MAX) return DOMAIN_TYPE_SMARTBOX;
    return DOMAIN_TYPE_UNKNOWN;
}

/* -------------------------------------------------------------------------- */
/* Mock Tag Registry & Collision Detector                                     */
/* -------------------------------------------------------------------------- */

typedef struct {
    uint32_t tag_id;
    char name[64];
    char domain[32];
} mock_tag_entry_t;

static int check_tag_collision(const mock_tag_entry_t *existing, size_t count, const mock_tag_entry_t *new_tag) {
    for (size_t i = 0; i < count; i++) {
        if (existing[i].tag_id == new_tag->tag_id) {
            // Same ID with different names is an illegal collision
            if (strcmp(existing[i].name, new_tag->name) != 0) {
                return -1; // Collision detected
            }
        }
        if (strcmp(existing[i].name, new_tag->name) == 0) {
            // Same name in different domain with different ID is an un-prefixed collision
            if (strcmp(existing[i].domain, new_tag->domain) != 0 && existing[i].tag_id != new_tag->tag_id) {
                return -2; // Cross-domain name collision
            }
        }
    }
    return 0; // Clean
}

/* -------------------------------------------------------------------------- */
/* CMocka Tests                                                               */
/* -------------------------------------------------------------------------- */

static void test_domain_namespace_ranges(void **state) {
    (void)state;

    // 1. Core domain boundaries
    assert_int_equal(get_domain_from_tag_id(1001), DOMAIN_TYPE_CORE);
    assert_int_equal(get_domain_from_tag_id(0x000000), DOMAIN_TYPE_CORE);
    assert_int_equal(get_domain_from_tag_id(0x00FFFF), DOMAIN_TYPE_CORE);

    // 2. DCU domain boundaries (0x010000..0x01FFFF)
    assert_int_equal(get_domain_from_tag_id(0x010000), DOMAIN_TYPE_DCU);
    assert_int_equal(get_domain_from_tag_id(65538), DOMAIN_TYPE_DCU); // 0x010002
    assert_int_equal(get_domain_from_tag_id(0x01FFFF), DOMAIN_TYPE_DCU);

    // 3. IEMS domain boundaries (0x020000..0x02FFFF)
    assert_int_equal(get_domain_from_tag_id(0x020000), DOMAIN_TYPE_IEMS);
    assert_int_equal(get_domain_from_tag_id(131074), DOMAIN_TYPE_IEMS); // 0x020002
    assert_int_equal(get_domain_from_tag_id(0x02FFFF), DOMAIN_TYPE_IEMS);

    // 4. SmartBox domain boundaries (0x030000..0x03FFFF)
    assert_int_equal(get_domain_from_tag_id(0x030000), DOMAIN_TYPE_SMARTBOX);
    assert_int_equal(get_domain_from_tag_id(196609), DOMAIN_TYPE_SMARTBOX); // 0x030001
    assert_int_equal(get_domain_from_tag_id(0x03FFFF), DOMAIN_TYPE_SMARTBOX);

    // 5. Out of bounds
    assert_int_equal(get_domain_from_tag_id(0x040000), DOMAIN_TYPE_UNKNOWN);
    assert_int_equal(get_domain_from_tag_id(0xFFFFFF), DOMAIN_TYPE_UNKNOWN);
}

static void test_collision_detection_logic(void **state) {
    (void)state;

    mock_tag_entry_t registry[4] = {
        { .tag_id = 1001, .name = "VOLTAGE_PHASE_A", .domain = "core_standard" },
        { .tag_id = 65538, .name = "DCU_LINELOSS_PCT", .domain = "dcu_distribution" },
        { .tag_id = 131074, .name = "IEMS_PCS_POWER_SET", .domain = "iems_microgrid" },
        { .tag_id = 196609, .name = "SMARTBOX_RESIDUAL_CURRENT_MA", .domain = "smart_box_panel" },
    };

    // Clean new tag
    mock_tag_entry_t new_clean = { .tag_id = 65540, .name = "DCU_DI_BITMAP", .domain = "dcu_distribution" };
    assert_int_equal(check_tag_collision(registry, 4, &new_clean), 0);

    // Conflicting Tag ID with different name
    mock_tag_entry_t colliding_id = { .tag_id = 1001, .name = "BATTERY_TEMPERATURE", .domain = "iems_microgrid" };
    assert_int_equal(check_tag_collision(registry, 4, &colliding_id), -1);

    // Conflicting Tag Name in different domain with different ID
    mock_tag_entry_t colliding_name = { .tag_id = 131080, .name = "VOLTAGE_PHASE_A", .domain = "iems_microgrid" };
    assert_int_equal(check_tag_collision(registry, 4, &colliding_name), -2);
}

static void test_collision_checker_python_tool_execution(void **state) {
    (void)state;

    // Run the Python contract collision checker tool against the real repository schemas
    int rc = system("python3 tools/check_contract_collisions.py > /dev/null 2>&1");
    assert_int_equal(rc, 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_domain_namespace_ranges),
        cmocka_unit_test(test_collision_detection_logic),
        cmocka_unit_test(test_collision_checker_python_tool_execution),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
