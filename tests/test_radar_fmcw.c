#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "radar_bus_port.h"
#include "driver_radar_fmcw.h"
#include "fmcw_dsp_neon.h"

static uint16_t calc_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

typedef struct {
    pipe_msg_t msgs[8];
    int count;
} fake_radar_bus_t;

static int fake_write_shadow(void *ctx, const pipe_msg_t *msg) {
    fake_radar_bus_t *bus = ctx;
    if (bus->count >= 8) return -1;
    bus->msgs[bus->count++] = *msg;
    return 0;
}

static const pipe_msg_t *find_msg(const fake_radar_bus_t *bus, uint32_t tag) {
    for (int i = 0; i < bus->count; i++) {
        if (bus->msgs[i].tag_id == tag) return &bus->msgs[i];
    }
    return NULL;
}

static void test_fmcw_radar_driver_parser(void **state) {
    (void)state;

    fake_radar_bus_t fake = {0};
    radar_bus_port_t port = {.ctx = &fake, .write_shadow = fake_write_shadow};

    assert_int_equal(driver_radar_fmcw_init(), 0);

    fmcw_radar_frame_t frame;
    frame.magic[0] = 0xAA;
    frame.magic[1] = 0x55;
    frame.cmd_type = 0x01;
    frame.payload_len = 8;
    frame.distance_mm = 2500;
    frame.velocity_cms = 150;
    frame.micro_amp = 350;
    frame.snr_db = 28;
    frame.target_cnt = 1;
    frame.checksum = calc_crc16((const uint8_t *)&frame, sizeof(frame) - 2);

    assert_int_equal(driver_radar_fmcw_parse_frame((const uint8_t *)&frame, sizeof(frame), &port), 0);
    assert_int_equal(fake.count, 3);

    const pipe_msg_t *dist = find_msg(&fake, 12021);
    const pipe_msg_t *vel = find_msg(&fake, 12022);
    const pipe_msg_t *amp = find_msg(&fake, 12023);
    assert_non_null(dist);
    assert_non_null(vel);
    assert_non_null(amp);
    assert_float_equal(dist->payload.val.f, 2.500, 0.001);
    assert_float_equal(vel->payload.val.f, 1.50, 0.001);
    assert_float_equal(amp->payload.val.f, 0.350, 0.001);
}

static void test_fmcw_neon_simd_dsp(void **state) {
    (void)state;

    complex_f32_t prev[4] = {{1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}, {7.0f, 8.0f}};
    complex_f32_t curr[4] = {{1.5f, 2.5f}, {4.0f, 5.0f}, {5.2f, 6.2f}, {10.0f, 12.0f}};
    complex_f32_t out[4];

    // Test MTI Static Clutter Subtraction Filter
    assert_int_equal(fmcw_mti_filter_simd(prev, curr, out, 4), 0);
    assert_float_equal(out[0].real, 0.5f, 0.001f);
    assert_float_equal(out[0].imag, 0.5f, 0.001f);
    assert_float_equal(out[1].real, 1.0f, 0.001f);
    assert_float_equal(out[1].imag, 1.0f, 0.001f);

    // Test Power Spectrum calculation sqrt(real^2 + imag^2) for [3+4i] -> sqrt(9+16)=5.0
    complex_f32_t rdm[2] = {{3.0f, 4.0f}, {6.0f, 8.0f}};
    float pwr[2];
    assert_int_equal(fmcw_rdm_power_spectrum_simd(rdm, pwr, 2), 0);
    assert_float_equal(pwr[0], 5.0f, 0.001f);
    assert_float_equal(pwr[1], 10.0f, 0.001f);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fmcw_radar_driver_parser),
        cmocka_unit_test(test_fmcw_neon_simd_dsp),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
