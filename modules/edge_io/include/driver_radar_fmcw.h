#ifndef DRIVER_RADAR_FMCW_H
#define DRIVER_RADAR_FMCW_H

#include <stdint.h>
#include <stddef.h>
#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FMCW Radar Point-Cloud Frame Magic Header (Modbus RTU / Binary Stream) */
#define FMCW_RADAR_MAGIC_0 0xAA
#define FMCW_RADAR_MAGIC_1 0x55

typedef struct __attribute__((packed)) {
    uint8_t magic[2];      // 0xAA, 0x55
    uint8_t cmd_type;      // 0x01 = Target Cloud, 0x02 = Vital Signs
    uint8_t payload_len;   // Payload length in bytes
    uint16_t distance_mm;  // Distance in millimeters
    int16_t velocity_cms;  // Velocity in cm/s
    uint16_t micro_amp;    // Micro-motion amplitude (raw scaled x1000)
    uint8_t  snr_db;       // Signal-to-Noise ratio in dB
    uint8_t  target_cnt;   // Detected target count
    uint16_t checksum;     // CRC16 Checksum
} fmcw_radar_frame_t;

int driver_radar_fmcw_init(void);
int driver_radar_fmcw_parse_frame(const uint8_t *buf, size_t len, datahub_t *hub);
int driver_radar_fmcw_poll(datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_RADAR_FMCW_H */
