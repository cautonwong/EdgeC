/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_bus_resilience_recovery.c
 * @brief Unit tests for Industrial Bus Resilience, Half-Duplex Flow Control & CAN Bus-Off Recovery
 * (AEP-462 / Issue #149)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_bus_resilience.h"

static void test_can_bus_off_and_self_healing(void) {
    printf("[TEST] CAN Error Counters & Bus-Off Self-Healing Recovery...\n");

    edge_can_resilience_mgr_t can;
    edge_can_resilience_init(&can, true);
    assert(can.state == EDGE_CAN_STATE_ERROR_ACTIVE);
    assert(can.tec == 0);
    assert(can.rec == 0);

    // Simulate transient EMI disturbance: 12 TX errors (+8 each)
    for (int i = 0; i < 12; i++) {
        edge_can_record_tx_error(&can, 8);  // tec -> 96
    }
    assert(can.tec == 96);
    assert(can.state == EDGE_CAN_STATE_ERROR_WARNING);

    // Further errors to pass into ERROR_PASSIVE (> 127)
    for (int i = 0; i < 5; i++) {
        edge_can_record_tx_error(&can, 8);  // tec -> 136
    }
    assert(can.state == EDGE_CAN_STATE_ERROR_PASSIVE);

    // Severe line short-circuit / bus-off (> 255)
    for (int i = 0; i < 16; i++) {
        edge_can_record_tx_error(&can, 8);  // tec > 255
    }
    assert(can.bus_off_count == 1);
    assert(can.state == EDGE_CAN_STATE_RECOVERING);

    // Step auto-recovery sequence: 127 steps do not complete
    for (int step = 0; step < 127; step++) {
        bool done = edge_can_step_recovery(&can);
        assert(!done);
        assert(can.state == EDGE_CAN_STATE_RECOVERING);
    }

    // 128th step recovers the bus
    bool recovered = edge_can_step_recovery(&can);
    assert(recovered);
    assert(can.state == EDGE_CAN_STATE_ERROR_ACTIVE);
    assert(can.tec == 0);
    assert(can.rec == 0);
    assert(can.auto_recover_count == 1);

    printf("  -> PASSED: CAN Bus-Off and self-healing state machine verified.\n");
}

static void test_rs485_half_duplex_flow_control(void) {
    printf("[TEST] RS-485 Half-Duplex Guard Times & Timing...\n");

    edge_rs485_flow_ctrl_t ctrl_9600;
    edge_rs485_flow_ctrl_init(&ctrl_9600, 9600);
    // at 9600 baud, 3.5 char * 11 bits * 1000000 / 9600 = 4010 us
    assert(ctrl_9600.post_tx_delay_us >= 4000 && ctrl_9600.post_tx_delay_us <= 4050);

    edge_rs485_flow_ctrl_t ctrl_115200;
    edge_rs485_flow_ctrl_init(&ctrl_115200, 115200);
    // at > 19200 baud, standard t3.5 is fixed at 1750 us
    assert(ctrl_115200.post_tx_delay_us == 1750);

    // Test RTS switching
    assert(!ctrl_9600.rts_asserted);
    edge_rs485_start_tx(&ctrl_9600);
    assert(ctrl_9600.rts_asserted);
    edge_rs485_end_tx(&ctrl_9600);
    assert(!ctrl_9600.rts_asserted);

    printf("  -> PASSED: RS-485 t3.5 delay and RTS flow control verified.\n");
}

static void test_slave_fault_adaptive_backoff(void) {
    printf("[TEST] Adaptive Slave Fault Backoff Policy...\n");

    edge_bus_backoff_mgr_t backoff;
    edge_bus_backoff_init(&backoff);

    edge_bus_backoff_register_slave(&backoff, 1, 1000, 8000);
    edge_bus_backoff_register_slave(&backoff, 2, 500, 4000);

    // Initial interval is base interval
    assert(edge_bus_backoff_get_interval(&backoff, 1) == 1000);
    assert(edge_bus_backoff_get_interval(&backoff, 2) == 500);

    // 1st and 2nd timeouts do not trigger backoff yet
    edge_bus_backoff_record_timeout(&backoff, 1);
    assert(edge_bus_backoff_get_interval(&backoff, 1) == 1000);
    edge_bus_backoff_record_timeout(&backoff, 1);
    assert(edge_bus_backoff_get_interval(&backoff, 1) == 1000);

    // 3rd timeout triggers 2x backoff (1000 -> 2000)
    edge_bus_backoff_record_timeout(&backoff, 1);
    assert(edge_bus_backoff_get_interval(&backoff, 1) == 2000);

    // 4th timeout triggers 4x backoff (2000 -> 4000)
    edge_bus_backoff_record_timeout(&backoff, 1);
    assert(edge_bus_backoff_get_interval(&backoff, 1) == 4000);

    // 5th timeout caps at max_interval (8000)
    edge_bus_backoff_record_timeout(&backoff, 1);
    assert(edge_bus_backoff_get_interval(&backoff, 1) == 8000);

    // Slave 2 remains unaffected
    assert(edge_bus_backoff_get_interval(&backoff, 2) == 500);

    // Success restores base interval immediately
    edge_bus_backoff_record_success(&backoff, 1);
    assert(edge_bus_backoff_get_interval(&backoff, 1) == 1000);

    printf("  -> PASSED: Adaptive slave backoff policy verified.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("     AEP-462: Industrial Bus Resilience & Self-Healing Test      \n");
    printf("=================================================================\n");

    test_can_bus_off_and_self_healing();
    test_rs485_half_duplex_flow_control();
    test_slave_fault_adaptive_backoff();

    printf("\nAll AEP-462 Bus Resilience tests PASSED!\n");
    return 0;
}
