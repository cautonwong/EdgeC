#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "driver_radar_fmcw.h"
#include "fmcw_dsp_neon.h"

static int dummy_map_fn(uint32_t tag_id) {
    if (tag_id >= 12021 && tag_id <= 12025) {
        return (int)(tag_id - 12021);
    }
    return -1;
}

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

static void test_fmcw_radar_driver_parser(void **state) {
    (void)state;

    // Initialize mock datahub with shadow array
    datahub_t hub;
    memset(&hub, 0, sizeof(hub));
    hub.map_fn = dummy_map_fn;
    hub.shadow_capacity = 10;
    hub.shadow_table = calloc(10, sizeof(pipe_msg_t));

    // Initialize driver
    assert_int_equal(driver_radar_fmcw_init(), 0);

    // Build mock FMCW Frame: Dist=2500mm (2.5m), Vel=150cm/s (1.5m/s), MicroAmp=350 (0.35)
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

    // Parse packet
    int ret = driver_radar_fmcw_parse_frame((const uint8_t *)&frame, sizeof(frame), &hub);
    assert_int_equal(ret, 0);

    // Verify DataHub Tag 12021 (Distance)
    pipe_msg_t msg_dist;
    assert_int_equal(datahub_read_shadow(&hub, 12021, &msg_dist), 0);
    assert_float_equal(msg_dist.payload.val.f, 2.500, 0.001);

    // Verify DataHub Tag 12022 (Velocity)
    pipe_msg_t msg_vel;
    assert_int_equal(datahub_read_shadow(&hub, 12022, &msg_vel), 0);
    assert_float_equal(msg_vel.payload.val.f, 1.50, 0.001);

    // Verify DataHub Tag 12023 (Micro-motion Amplitude)
    pipe_msg_t msg_amp;
    assert_int_equal(datahub_read_shadow(&hub, 12023, &msg_amp), 0);
    assert_float_equal(msg_amp.payload.val.f, 0.350, 0.001);

    free(hub.shadow_table);
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
