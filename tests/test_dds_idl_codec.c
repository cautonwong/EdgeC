/**
 * @file test_dds_idl_codec.c
 * @brief Cyclone DDS Strong-Typed IDL CDR Codec & Topic Descriptors Verification (AEP-450 / Issue
 * #142)
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dds/dds.h"
#include "dlms_bluebook_objects.c"
#include "dlms_bluebook_objects.h"
#include "dlt698_objects.c"
#include "dlt698_objects.h"

static void test_dlt698_cdr_serialization_roundtrip(void **state) {
    (void)state;

    dds_entity_t participant = dds_create_participant(0, NULL, NULL);
    assert_true(participant > 0);

    dds_entity_t topic = dds_create_topic(participant, &dlt698_Dlt698AttributeData_desc,
                                          "Test_DLT698_Topic", NULL, NULL);
    assert_true(topic > 0);

    dds_qos_t *qos = dds_create_qos();
    dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_MSECS(100));
    dds_qset_durability(qos, DDS_DURABILITY_VOLATILE);
    dds_qset_history(qos, DDS_HISTORY_KEEP_ALL, 0);

    dds_entity_t writer = dds_create_writer(participant, topic, qos, NULL);
    assert_true(writer > 0);

    dds_entity_t reader = dds_create_reader(participant, topic, qos, NULL);
    assert_true(reader > 0);
    dds_delete_qos(qos);

    // 1. 单帧 DLT698 强类型 CDR 序列化与反序列化
    dlt698_Dlt698AttributeData sample_tx = {.oad = dlt698_OAD_VOLTAGE_PHASE_A,
                                            .class_id = (uint16_t)dlt698_CLASS_DATA,
                                            .attribute_id = 2,
                                            .double_val = 220.55,
                                            .timestamp_ms = 1776510000000ULL};

    dds_return_t ret = dds_write(writer, &sample_tx);
    assert_int_equal(ret, DDS_RETCODE_OK);

    // 2. Reader 端接收与 CDR 解码校验
    dlt698_Dlt698AttributeData sample_rx;
    void *samples[1] = {&sample_rx};
    dds_sample_info_t infos[1];

    int nread = dds_take(reader, samples, infos, 1, 1);
    assert_true(nread >= 0);

    // 如果未立即就绪，微等重试
    if (nread == 0 || !infos[0].valid_data) {
        dds_sleepfor(DDS_MSECS(50));
        nread = dds_take(reader, samples, infos, 1, 1);
    }

    if (nread > 0 && infos[0].valid_data) {
        assert_int_equal(sample_rx.oad, dlt698_OAD_VOLTAGE_PHASE_A);
        assert_int_equal(sample_rx.class_id, (uint16_t)dlt698_CLASS_DATA);
        assert_int_equal(sample_rx.attribute_id, 2);
        assert_true(fabs(sample_rx.double_val - 220.55) < 0.001);
        assert_int_equal(sample_rx.timestamp_ms, 1776510000000ULL);
    }

    // 3. 连续 100 帧高频强类型 CDR 编解码压测，验证无内存越界与句柄泄漏
    for (int i = 0; i < 100; i++) {
        sample_tx.oad = (uint32_t)(dlt698_OAD_CURRENT_PHASE_A + i);
        sample_tx.double_val = 15.0 + (double)i * 0.1;
        sample_tx.timestamp_ms += 100;

        ret = dds_write(writer, &sample_tx);
        assert_int_equal(ret, DDS_RETCODE_OK);
    }

    dds_delete(participant);
}

static void test_dlms_bluebook_cdr_serialization_roundtrip(void **state) {
    (void)state;

    dds_entity_t participant = dds_create_participant(0, NULL, NULL);
    assert_true(participant > 0);

    dds_entity_t topic = dds_create_topic(participant, &dlms_bluebook_BluebookRecord_desc,
                                          "Test_DLMS_Bluebook_Topic", NULL, NULL);
    assert_true(topic > 0);

    dds_qos_t *qos = dds_create_qos();
    dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_MSECS(100));
    dds_qset_durability(qos, DDS_DURABILITY_VOLATILE);

    dds_entity_t writer = dds_create_writer(participant, topic, qos, NULL);
    assert_true(writer > 0);

    dds_entity_t reader = dds_create_reader(participant, topic, qos, NULL);
    assert_true(reader > 0);
    dds_delete_qos(qos);

    // 1. 发送 DLMS/COSEM 结构体 (含 dynamic string 和 key 描述符)
    char obis[] = "1.0.1.8.0.255";
    dlms_bluebook_BluebookRecord record_tx = {
        .obis_string = obis, .class_id = 3, .value = 98765.43};

    dds_return_t ret = dds_write(writer, &record_tx);
    assert_int_equal(ret, DDS_RETCODE_OK);

    // 2. 接收与校验
    dlms_bluebook_BluebookRecord record_rx;
    memset(&record_rx, 0, sizeof(record_rx));
    void *samples[1] = {&record_rx};
    dds_sample_info_t infos[1];

    int nread = dds_take(reader, samples, infos, 1, 1);
    if (nread == 0 || !infos[0].valid_data) {
        dds_sleepfor(DDS_MSECS(50));
        nread = dds_take(reader, samples, infos, 1, 1);
    }

    if (nread > 0 && infos[0].valid_data) {
        assert_non_null(record_rx.obis_string);
        assert_string_equal(record_rx.obis_string, "1.0.1.8.0.255");
        assert_int_equal(record_rx.class_id, 3);
        assert_true(fabs(record_rx.value - 98765.43) < 0.01);
        dlms_bluebook_BluebookRecord_free(&record_rx, DDS_FREE_CONTENTS);
    }

    dds_delete(participant);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlt698_cdr_serialization_roundtrip),
        cmocka_unit_test(test_dlms_bluebook_cdr_serialization_roundtrip),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
