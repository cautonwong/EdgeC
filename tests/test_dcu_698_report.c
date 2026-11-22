/**
 * @file test_dcu_698_report.c
 * @brief 698 主动上报队列：真实事件产出恰好一条上报（ADR-0017）
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_report.h"
#include "dcu_698_var.h"
#include "dlt698_codec.h"

/* 事件对象（Class 24，OI 3000/3005）的属性 4：当前记录数。 */
#define EVENT_COUNT_OAD(oi) (((uint32_t)(oi) << 16) | (4u << 8))

static void test_event_produces_exactly_one_report(void **state) {
    (void)state;
    dcu_698_report_reset();

    dcu_698_report_enqueue(EVENT_COUNT_OAD(0x3000), DCU_698_TAG_LONG_UNSIGNED, 3);

    uint8_t body[64];
    size_t len = 0;
    assert_int_equal(dcu_698_report_next(NULL, body, sizeof(body), &len), 0);

    /* CHOICE[1] | PIID | 个数 | OAD | Get-Result[1] | long-unsigned 3 */
    static const uint8_t want_tail[] = {0x30, 0x00, 0x04, 0x00, 0x01, 0x12, 0x00, 0x03};
    assert_true(len == 3 + sizeof(want_tail));
    assert_int_equal(body[0], DLT698_REPORT_LIST);
    assert_int_equal(body[2], 0x01);
    assert_memory_equal(body + 3, want_tail, sizeof(want_tail));

    len = 1;
    assert_int_equal(dcu_698_report_next(NULL, body, sizeof(body), &len), 0);
    assert_int_equal(len, 0);
}

static void test_two_events_batch_into_one_report(void **state) {
    (void)state;
    dcu_698_report_reset();

    dcu_698_report_enqueue(EVENT_COUNT_OAD(0x3000), DCU_698_TAG_LONG_UNSIGNED, 1);
    dcu_698_report_enqueue(EVENT_COUNT_OAD(0x3005), DCU_698_TAG_LONG_UNSIGNED, 2);

    uint8_t body[64];
    size_t len = 0;
    assert_int_equal(dcu_698_report_next(NULL, body, sizeof(body), &len), 0);
    assert_int_equal(body[0], DLT698_REPORT_LIST);
    assert_int_equal(body[2], 0x02);
}

static void test_record_list_report_is_explicitly_unsupported(void **state) {
    (void)state;
    uint8_t body[8];
    size_t len = 0;
    assert_int_equal(dlt698_encode_report_notification(body, sizeof(body),
                                                       DLT698_REPORT_RECORD_LIST, 1, NULL, 0,
                                                       &len),
                     DLT698_REPORT_UNSUPPORTED_RECORD);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_event_produces_exactly_one_report),
        cmocka_unit_test(test_two_events_batch_into_one_report),
        cmocka_unit_test(test_record_list_report_is_explicitly_unsupported),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
