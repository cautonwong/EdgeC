/**
 * @file custom_inverter_driver.h
 * @brief Secondary Developer Example: Custom Photovoltaic / Storage Inverter Driver
 */

#ifndef EXAMPLES_CUSTOM_INVERTER_DRIVER_H
#define EXAMPLES_CUSTOM_INVERTER_DRIVER_H

#include <edgeos/edgeos.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float voltage_ac;
    float current_ac;
    float active_power_w;
    float grid_freq_hz;
    uint32_t status_flags;
} custom_inverter_telemetry_t;

/**
 * @brief Custom Inverter Driver Descriptor
 */
extern const edgeos_driver_t custom_inverter_driver;

/**
 * @brief Simulate receiving a hardware Modbus frame and publishing to DataHub
 */
int custom_inverter_simulate_poll(edgeos_driver_ctx_t *ctx, edgeos_datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* EXAMPLES_CUSTOM_INVERTER_DRIVER_H */
