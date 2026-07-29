#include "driver_radar_fmcw.h"
#include <stdio.h>
#include <string.h>
#include <uv.h>
#include "edge_log.h"

static uint16_t calculate_crc16(const uint8_t *data, size_t len) {
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

int driver_radar_fmcw_init(void) {
    LOG_INFO(LOG_CAT_CORE, "[FMCW Radar] Driver initialized. Modbus/Binary Stream Mode ready.");
    return 0;
}

int driver_radar_fmcw_parse_frame(const uint8_t *buf, size_t len, datahub_t *hub) {
    if (!buf || len < sizeof(fmcw_radar_frame_t) || !hub) {
        return -1;
    }

    const fmcw_radar_frame_t *frame = (const fmcw_radar_frame_t *)buf;

    // Check magic bytes (0xAA 0x55)
    if (frame->magic[0] != FMCW_RADAR_MAGIC_0 || frame->magic[1] != FMCW_RADAR_MAGIC_1) {
        LOG_WARN(LOG_CAT_CORE, "[FMCW Radar] Invalid magic header: 0x%02x 0x%02x", frame->magic[0], frame->magic[1]);
        return -2;
    }

    // Check CRC16 checksum
    uint16_t crc_calc = calculate_crc16(buf, sizeof(fmcw_radar_frame_t) - 2);
    if (crc_calc != frame->checksum) {
        LOG_WARN(LOG_CAT_CORE, "[FMCW Radar] CRC16 mismatch: Calc=0x%04x Recv=0x%04x", crc_calc, frame->checksum);
        return -3;
    }

    double dist_m = (double)frame->distance_mm / 1000.0;
    double vel_ms = (double)frame->velocity_cms / 100.0;
    double micro_amp = (double)frame->micro_amp / 1000.0;

    LOG_INFO(LOG_CAT_CORE, "[FMCW Radar] Parsed Frame: Dist=%.3fm, Vel=%.2fm/s, MicroAmp=%.3f, SNR=%ddB, Targets=%d",
             dist_m, vel_ms, micro_amp, frame->snr_db, frame->target_cnt);

    uint64_t ts = uv_hrtime();
    pipe_msg_t msg_dist, msg_vel, msg_amp;
    pipe_msg_init_scalar(&msg_dist, 12021, dist_m, ts);
    pipe_msg_init_scalar(&msg_vel, 12022, vel_ms, ts);
    pipe_msg_init_scalar(&msg_amp, 12023, micro_amp, ts);

    int idx_dist = hub->map_fn_v2 ? hub->map_fn_v2(12021, 0) : hub->map_fn(12021);
    int idx_vel  = hub->map_fn_v2 ? hub->map_fn_v2(12022, 0) : hub->map_fn(12022);
    int idx_amp  = hub->map_fn_v2 ? hub->map_fn_v2(12023, 0) : hub->map_fn(12023);

    if (idx_dist >= 0) datahub_write_shadow_idx(hub, idx_dist, &msg_dist);
    if (idx_vel >= 0)  datahub_write_shadow_idx(hub, idx_vel, &msg_vel);
    if (idx_amp >= 0)  datahub_write_shadow_idx(hub, idx_amp, &msg_amp);

    return 0;
}

int driver_radar_fmcw_poll(datahub_t *hub) {
    (void)hub;
    return 0;
}
