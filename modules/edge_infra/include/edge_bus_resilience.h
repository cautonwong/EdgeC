/**
 * @file edge_bus_resilience.h
 * @brief Industrial Bus Resilience, Half-Duplex Flow Control & CAN Bus-Off Auto-Recovery Engine (AEP-462)
 */

#ifndef EDGE_BUS_RESILIENCE_H
#define EDGE_BUS_RESILIENCE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CAN Controller Error States (ISO 11898-1) */
typedef enum {
    EDGE_CAN_STATE_ERROR_ACTIVE = 0,   // Normal operation (TEC < 96, REC < 96)
    EDGE_CAN_STATE_ERROR_WARNING = 1,  // Warning level (TEC >= 96 or REC >= 96)
    EDGE_CAN_STATE_ERROR_PASSIVE = 2,  // Passive level (TEC > 127 or REC > 127)
    EDGE_CAN_STATE_BUS_OFF = 3,        // Bus-off (TEC > 255)
    EDGE_CAN_STATE_RECOVERING = 4      // Auto-recovery reset sequence active
} edge_can_state_t;

typedef struct {
    edge_can_state_t state;
    uint16_t tec;               // Transmit Error Counter
    uint16_t rec;               // Receive Error Counter
    uint32_t bus_off_count;     // Cumulative Bus-Off events
    uint32_t auto_recover_count;// Successful self-heal recovery cycles
    uint32_t consecutive_recessive_bits; // 128 * 11 bits counter simulation
    bool auto_recovery_enabled;
} edge_can_resilience_mgr_t;

/* RS-485 / Modbus RTU Half-Duplex Flow Control */
typedef struct {
    uint32_t baud_rate;
    uint32_t pre_tx_delay_us;   // RTS assert to first byte guard time
    uint32_t post_tx_delay_us;  // Last bit out to RTS deassert guard time (t3.5)
    uint32_t turnaround_us;     // Bus turnaround delay
    bool rts_asserted;
} edge_rs485_flow_ctrl_t;

/* Adaptive Slave Fault Backoff Policy */
#define EDGE_BUS_MAX_SLAVES 64

typedef struct {
    uint32_t slave_id;
    uint16_t consecutive_timeouts;
    uint32_t current_poll_interval_ms;
    uint32_t base_poll_interval_ms;
    uint32_t max_poll_interval_ms;
    bool is_isolated;
} edge_slave_backoff_t;

typedef struct {
    edge_slave_backoff_t slaves[EDGE_BUS_MAX_SLAVES];
    uint32_t slave_count;
} edge_bus_backoff_mgr_t;

/* --- CAN Auto-Recovery Functions --- */
void edge_can_resilience_init(edge_can_resilience_mgr_t *mgr, bool auto_recovery);
void edge_can_record_tx_error(edge_can_resilience_mgr_t *mgr, uint8_t err_increment);
void edge_can_record_rx_error(edge_can_resilience_mgr_t *mgr, uint8_t err_increment);
void edge_can_record_tx_success(edge_can_resilience_mgr_t *mgr);
void edge_can_record_rx_success(edge_can_resilience_mgr_t *mgr);
bool edge_can_step_recovery(edge_can_resilience_mgr_t *mgr);

/* --- RS-485 Flow Control Functions --- */
void edge_rs485_flow_ctrl_init(edge_rs485_flow_ctrl_t *ctrl, uint32_t baud_rate);
uint32_t edge_rs485_compute_t35_us(uint32_t baud_rate);
void edge_rs485_start_tx(edge_rs485_flow_ctrl_t *ctrl);
void edge_rs485_end_tx(edge_rs485_flow_ctrl_t *ctrl);

/* --- Slave Backoff Functions --- */
void edge_bus_backoff_init(edge_bus_backoff_mgr_t *mgr);
int edge_bus_backoff_register_slave(edge_bus_backoff_mgr_t *mgr, uint32_t slave_id,
                                    uint32_t base_interval_ms, uint32_t max_interval_ms);
void edge_bus_backoff_record_timeout(edge_bus_backoff_mgr_t *mgr, uint32_t slave_id);
void edge_bus_backoff_record_success(edge_bus_backoff_mgr_t *mgr, uint32_t slave_id);
uint32_t edge_bus_backoff_get_interval(const edge_bus_backoff_mgr_t *mgr, uint32_t slave_id);

#ifdef __cplusplus
}
#endif

#endif // EDGE_BUS_RESILIENCE_H
