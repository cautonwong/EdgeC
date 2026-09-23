/**
 * @file test_dicom_parser.c
 * @brief Unit tests for Micro-DICOM Frame & Telemetry Parser Operator
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dicom_parser.h"

static void test_dicom_parse_header_null(void **state) {
    (void)state;
    dicom_meta_t meta;
    uint8_t dummy[200] = {0};

    assert_int_equal(dicom_parse_header(NULL, 200, &meta), -1);
    assert_int_equal(dicom_parse_header(dummy, 200, NULL), -1);
    assert_int_equal(dicom_parse_header(dummy, 50, &meta), -1);
}

static void test_dicom_parse_valid_frame(void **state) {
    (void)state;
    uint8_t frame_buf[1024];
    memset(frame_buf, 0, sizeof(frame_buf));

    /* Header DICM */
    frame_buf[128] = 'D';
    frame_buf[129] = 'I';
    frame_buf[130] = 'C';
    frame_buf[131] = 'M';

    /* Tag (0x0028, 0x0010) Rows = 16 */
    frame_buf[132] = 0x28;
    frame_buf[133] = 0x00;
    frame_buf[134] = 0x10;
    frame_buf[135] = 0x00;
    frame_buf[140] = 16;
    frame_buf[141] = 0;

    /* Tag (0x0028, 0x0011) Columns = 16 */
    frame_buf[142] = 0x28;
    frame_buf[143] = 0x00;
    frame_buf[144] = 0x11;
    frame_buf[145] = 0x00;
    frame_buf[150] = 16;
    frame_buf[151] = 0;

    /* Tag (0x7FE0, 0x0010) Pixel Data */
    frame_buf[152] = 0xE0;
    frame_buf[153] = 0x7F;
    frame_buf[154] = 0x10;
    frame_buf[155] = 0x00;
    size_t pixel_offset = 164;

    uint16_t *pixels = (uint16_t *)(frame_buf + pixel_offset);
    for (int i = 0; i < 256; i++) {
        pixels[i] = (uint16_t)(100 + (i % 10));
    }

    dicom_meta_t meta;
    int res = dicom_parse_header(frame_buf, sizeof(frame_buf), &meta);
    assert_int_equal(res, 0);
    assert_int_equal(meta.rows, 16);
    assert_int_equal(meta.columns, 16);
    assert_true(meta.mean_intensity > 100.0 && meta.mean_intensity < 110.0);
    assert_true(meta.snr_db > 20.0);

    /* ROI Mean test */
    double roi_mean = 0.0;
    res = dicom_compute_roi_mean(frame_buf, sizeof(frame_buf), &meta, 0, 0, 4, 4, &roi_mean);
    assert_int_equal(res, 0);
    assert_true(roi_mean > 99.0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dicom_parse_header_null),
        cmocka_unit_test(test_dicom_parse_valid_frame),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
