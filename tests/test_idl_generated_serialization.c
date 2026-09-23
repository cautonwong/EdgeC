/**
 * @file test_idl_generated_serialization.c
 * @brief Empirical C Unit Test verifying serialization descriptors & structs derived from OMG IDLs
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dlms_bluebook_objects.h"
#include "dlms_cosem_objects.h"
#include "dlt698_objects.h"
#include "idis_pack3_objects.h"

/* Include generated descriptor C files directly from IDL_GEN_DIR without hardcoded paths */
#include "dlms_bluebook_objects.c"
#include "dlms_cosem_objects.c"
#include "dlt698_objects.c"
#include "idis_pack3_objects.c"

static void test_dlt698_idl_struct_and_descriptor(void **state) {
    (void)state;

    dlt698_Dlt698AttributeData data;
    memset(&data, 0, sizeof(data));

    data.oad = dlt698_OAD_VOLTAGE_PHASE_A;
    data.class_id = dlt698_CLASS_DATA;
    data.attribute_id = 2;
    data.double_val = 220.5;
    data.timestamp_ms = 1700000000000ULL;

    assert_int_equal(data.oad, 536871424); /* 0x20000200 */
    assert_int_equal(data.class_id, dlt698_CLASS_DATA);
    assert_float_equal(data.double_val, 220.5, 0.001);

    /* Verify Cyclone DDS Topic Descriptor struct */
    assert_non_null(dlt698_Dlt698AttributeData_desc.m_typename);
    assert_string_equal(dlt698_Dlt698AttributeData_desc.m_typename, "dlt698::Dlt698AttributeData");
    assert_int_equal(dlt698_Dlt698AttributeData_desc.m_size, sizeof(dlt698_Dlt698AttributeData));
    assert_int_equal(dlt698_Dlt698AttributeData_desc.m_nkeys, 1);
}

static void test_idis_pack3_idl_struct_and_descriptor(void **state) {
    (void)state;

    idis_IdisPack3Record rec;
    memset(&rec, 0, sizeof(rec));

    rec.obis_code = "1.0.1.7.0.255";
    rec.timestamp_ms = 1700000000000ULL;
    rec.active_energy_import = 12345.67;
    rec.status_flags = 0x00000001;

    assert_string_equal(rec.obis_code, "1.0.1.7.0.255");
    assert_float_equal(rec.active_energy_import, 12345.67, 0.01);
    assert_int_equal(rec.status_flags, 1);

    /* Verify Cyclone DDS Topic Descriptor struct */
    assert_non_null(idis_IdisPack3Record_desc.m_typename);
    assert_string_equal(idis_IdisPack3Record_desc.m_typename, "idis::IdisPack3Record");
    assert_int_equal(idis_IdisPack3Record_desc.m_size, sizeof(idis_IdisPack3Record));
}

static void test_dlms_bluebook_idl_struct_and_descriptor(void **state) {
    (void)state;

    dlms_bluebook_BluebookRecord rec;
    memset(&rec, 0, sizeof(rec));

    rec.obis_string = "1.0.1.8.0.255";
    rec.class_id = 1;
    rec.value = 999.99;

    assert_string_equal(rec.obis_string, "1.0.1.8.0.255");
    assert_int_equal(rec.class_id, 1);
    assert_float_equal(rec.value, 999.99, 0.01);

    /* Verify Cyclone DDS Topic Descriptor struct */
    assert_non_null(dlms_bluebook_BluebookRecord_desc.m_typename);
    assert_string_equal(dlms_bluebook_BluebookRecord_desc.m_typename,
                        "dlms::bluebook::BluebookRecord");
    assert_int_equal(dlms_bluebook_BluebookRecord_desc.m_size,
                     sizeof(dlms_bluebook_BluebookRecord));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlt698_idl_struct_and_descriptor),
        cmocka_unit_test(test_idis_pack3_idl_struct_and_descriptor),
        cmocka_unit_test(test_dlms_bluebook_idl_struct_and_descriptor),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
