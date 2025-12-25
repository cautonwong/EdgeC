#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include <string.h>
#include <stdlib.h> // For free

#include "modbus_protocol.h"

// Helper function to calculate CRC16-Modbus for verification in tests
static uint16_t test_modbus_crc16(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// Mock callback data for testing
typedef struct {
    int status;
    uint16_t num_registers;
    uint16_t *registers;
    void *user_data;
    int called;
} mock_modbus_cb_data_t;

static void mock_modbus_read_cb(int status, const uint16_t *registers, uint16_t num_registers, void *user) {
    mock_modbus_cb_data_t *data = (mock_modbus_cb_data_t *)user;
    data->status = status;
    data->num_registers = num_registers;
    // Copy registers if provided
    if (registers && num_registers > 0) {
        data->registers = (uint16_t *)malloc(sizeof(uint16_t) * num_registers);
        memcpy(data->registers, registers, sizeof(uint16_t) * num_registers);
    } else {
        data->registers = NULL;
    }
    data->called = 1;
}

static void test_modbus_create_read_job(void **state) {
    (void)state; // Unused

    uint8_t slave_id = 1;
    uint16_t start_addr = 0x006B; // 107
    uint16_t num_regs = 3;
    size_t rx_cap = 256;

    mock_modbus_cb_data_t cb_data = {0};
    io_job_t *job = modbus_protocol_create_read_job(slave_id, start_addr, num_regs, rx_cap, mock_modbus_read_cb, &cb_data);

    assert_non_null(job);
    assert_non_null(job->tx);
    assert_int_equal(job->tx_len, 8); // Slave ID + FC + Start Addr (2) + Num Regs (2) + CRC (2)

    // Expected TX frame: 01 03 00 6B 00 03 76 87
    uint8_t expected_tx[] = {0x01, 0x03, 0x00, 0x6B, 0x00, 0x03, 0x76, 0x87};
    uint16_t calculated_crc = test_modbus_crc16(job->tx, 6);
    assert_int_equal(calculated_crc, 0x8776); // CRC is little endian in frame

    assert_memory_equal(job->tx, expected_tx, job->tx_len);
    assert_non_null(job->rx);
    assert_int_equal(job->rx_cap, rx_cap);

    // Clean up
    free(job->tx);
    free(job->rx);
    // The modbus_req_ctx_t is stored in job->user, need to free it
    free(job->user);
    free(job);
}

static void test_modbus_handle_response_success(void **state) {
    (void)state; // Unused

    uint8_t slave_id = 1;
    uint16_t start_addr = 0x006B;
    uint16_t num_regs = 3;
    size_t rx_cap = 256;

    mock_modbus_cb_data_t cb_data = {0};
    io_job_t *job = modbus_protocol_create_read_job(slave_id, start_addr, num_regs, rx_cap, mock_modbus_read_cb, &cb_data);
    assert_non_null(job);

    // Simulate a successful Modbus response: 01 03 06 00 01 00 02 00 03 72 80
    // Slave ID (1) + FC (3) + Byte Count (6) + Data (0001 0002 0003) + CRC (7280)
    uint8_t mock_rx_data[] = {0x01, 0x03, 0x06, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x72, 0x80};
    job->rx_len = sizeof(mock_rx_data);
    memcpy(job->rx, mock_rx_data, job->rx_len);

    modbus_protocol_handle_response(job);

    assert_int_equal(cb_data.called, 1);
    assert_int_equal(cb_data.status, 0);
    assert_int_equal(cb_data.num_registers, 3);
    assert_non_null(cb_data.registers);
    assert_int_equal(cb_data.registers[0], 1);
    assert_int_equal(cb_data.registers[1], 2);
    assert_int_equal(cb_data.registers[2], 3);

    // Clean up
    free(job->tx);
    free(job->rx);
    if (cb_data.registers) free(cb_data.registers);
    // job->user is freed inside modbus_protocol_handle_response
    free(job);
}

static void test_modbus_handle_response_crc_error(void **state) {
    (void)state; // Unused

    uint8_t slave_id = 1;
    uint16_t start_addr = 0x006B;
    uint16_t num_regs = 3;
    size_t rx_cap = 256;

    mock_modbus_cb_data_t cb_data = {0};
    io_job_t *job = modbus_protocol_create_read_job(slave_id, start_addr, num_regs, rx_cap, mock_modbus_read_cb, &cb_data);
    assert_non_null(job);

    // Simulate a response with incorrect CRC: 01 03 06 00 01 00 02 00 03 72 81 (last byte changed)
    uint8_t mock_rx_data[] = {0x01, 0x03, 0x06, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x72, 0x81};
    job->rx_len = sizeof(mock_rx_data);
    memcpy(job->rx, mock_rx_data, job->rx_len);

    modbus_protocol_handle_response(job);

    assert_int_equal(cb_data.called, 1);
    assert_int_equal(cb_data.status, -2); // -2 for CRC error
    assert_int_equal(cb_data.num_registers, 0);
    assert_null(cb_data.registers);

    // Clean up
    free(job->tx);
    free(job->rx);
    // job->user is freed inside modbus_protocol_handle_response
    free(job);
}

static void test_modbus_handle_response_exception(void **state) {
    (void)state; // Unused

    uint8_t slave_id = 1;
    uint16_t start_addr = 0x006B;
    uint16_t num_regs = 3;
    size_t rx_cap = 256;

    mock_modbus_cb_data_t cb_data = {0};
    io_job_t *job = modbus_protocol_create_read_job(slave_id, start_addr, num_regs, rx_cap, mock_modbus_read_cb, &cb_data);
    assert_non_null(job);

    // Simulate a Modbus exception response (FC + 0x80), e.g., Illegal Data Address (0x02)
    // Frame: 01 83 02 C0 B0
    uint8_t mock_rx_data[] = {0x01, 0x83, 0x02, 0xC0, 0xB0};
    job->rx_len = sizeof(mock_rx_data);
    memcpy(job->rx, mock_rx_data, job->rx_len);

    modbus_protocol_handle_response(job);

    assert_int_equal(cb_data.called, 1);
    assert_int_equal(cb_data.status, MODBUS_EXC_ILLEGAL_DATA_ADDRESS); // Expect exception code 0x02
    assert_int_equal(cb_data.num_registers, 0);
    assert_null(cb_data.registers);

    // Clean up
    free(job->tx);
    free(job->rx);
    free(job);
}


int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_modbus_create_read_job),
        cmocka_unit_test(test_modbus_handle_response_success),
        cmocka_unit_test(test_modbus_handle_response_crc_error),
        cmocka_unit_test(test_modbus_handle_response_exception),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
