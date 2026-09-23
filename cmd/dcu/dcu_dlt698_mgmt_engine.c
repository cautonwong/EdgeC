/**
 * @file dcu_dlt698_mgmt_engine.c
 * @brief Production DL/T 698.45 Management Engine Implementation
 */

#include "dcu_dlt698_mgmt_engine.h"

#include <string.h>

static uint32_t calculate_crc32_step(uint32_t crc, const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return crc;
}

void dcu_mgmt_init(dcu_dlt698_mgmt_t* mgmt) {
    if (!mgmt) return;
    memset(mgmt, 0, sizeof(dcu_dlt698_mgmt_t));
    mgmt->dfu.state = DFU_STATE_IDLE;
    mgmt->dfu.active_bank = 0;  // Default on Bank A

    // Setup standard default 4-Rate TOU table
    mgmt->tariff_table.slot_count = 4;
    mgmt->tariff_table.slots[0] = (dcu_tou_slot_t){.slot_id = 1,
                                                   .start_hour = 0,
                                                   .start_minute = 0,
                                                   .tariff_rate = 4};  // Valley (00:00-08:00)
    mgmt->tariff_table.slots[1] = (dcu_tou_slot_t){
        .slot_id = 2, .start_hour = 8, .start_minute = 0, .tariff_rate = 3};  // Flat (08:00-11:00)
    mgmt->tariff_table.slots[2] = (dcu_tou_slot_t){
        .slot_id = 3, .start_hour = 11, .start_minute = 0, .tariff_rate = 1};  // Peak (11:00-15:00)
    mgmt->tariff_table.slots[3] = (dcu_tou_slot_t){.slot_id = 4,
                                                   .start_hour = 15,
                                                   .start_minute = 0,
                                                   .tariff_rate = 2};  // Shoulder (15:00-24:00)
    mgmt->tariff_table.version = 1;
}

int dcu_mgmt_dfu_start(dcu_dlt698_mgmt_t* mgmt, const char* version, uint32_t total_size,
                       uint32_t crc32) {
    if (!mgmt || !version || total_size == 0) return -1;

    dcu_dfu_session_t* dfu = &mgmt->dfu;
    memset(dfu, 0, sizeof(dcu_dfu_session_t));
    strncpy(dfu->target_version, version, sizeof(dfu->target_version) - 1);
    dfu->total_image_size = total_size;
    dfu->target_crc32 = crc32;
    dfu->computed_crc32 = 0xFFFFFFFF;  // Initial CRC seed
    dfu->expected_blocks = (total_size + DCU_DFU_BLOCK_SIZE - 1) / DCU_DFU_BLOCK_SIZE;
    dfu->received_blocks = 0;
    dfu->received_bytes = 0;
    dfu->state = DFU_STATE_INITIATED;

    return 0;
}

int dcu_mgmt_dfu_write_block(dcu_dlt698_mgmt_t* mgmt, uint32_t block_index,
                             const uint8_t* block_data, uint32_t block_len) {
    if (!mgmt || !block_data || block_len == 0) return -1;
    dcu_dfu_session_t* dfu = &mgmt->dfu;

    if (dfu->state != DFU_STATE_INITIATED && dfu->state != DFU_STATE_RECEIVING) {
        return -2;
    }

    if (block_index != dfu->received_blocks) {
        return -3;  // Sequence error (out of order)
    }

    dfu->state = DFU_STATE_RECEIVING;
    dfu->computed_crc32 = calculate_crc32_step(dfu->computed_crc32, block_data, block_len);
    dfu->received_bytes += block_len;
    dfu->received_blocks++;

    if (dfu->received_blocks == dfu->expected_blocks &&
        dfu->received_bytes >= dfu->total_image_size) {
        dfu->state = DFU_STATE_VERIFYING;
    }

    return 0;
}

int dcu_mgmt_dfu_verify_and_activate(dcu_dlt698_mgmt_t* mgmt) {
    if (!mgmt) return -1;
    dcu_dfu_session_t* dfu = &mgmt->dfu;

    if (dfu->state != DFU_STATE_VERIFYING && dfu->state != DFU_STATE_RECEIVING) {
        return -2;
    }

    uint32_t final_crc = ~dfu->computed_crc32;
    if (final_crc != dfu->target_crc32) {
        dfu->state = DFU_STATE_FAILED;
        return -3;  // CRC Mismatch
    }

    dfu->state = DFU_STATE_READY;
    // Swap active bank: Bank A (0) <-> Bank B (1)
    dfu->active_bank = (dfu->active_bank == 0) ? 1 : 0;
    dfu->state = DFU_STATE_ACTIVATED;

    return 0;
}

int dcu_mgmt_set_tariff_table(dcu_dlt698_mgmt_t* mgmt, const dcu_tou_slot_t* slots, uint8_t count) {
    if (!mgmt || !slots || count == 0 || count > DCU_MAX_TOU_SLOTS) return -1;

    for (uint8_t i = 0; i < count; i++) {
        mgmt->tariff_table.slots[i] = slots[i];
    }
    mgmt->tariff_table.slot_count = count;
    mgmt->tariff_table.version++;
    return 0;
}

int dcu_mgmt_authenticate_session(dcu_dlt698_mgmt_t* mgmt, uint8_t level, uint32_t duration_s,
                                  uint64_t now_ms) {
    if (!mgmt) return -1;

    mgmt->sec_session.is_authenticated = true;
    mgmt->sec_session.security_level = level;
    mgmt->sec_session.session_id = (uint32_t)(now_ms & 0xFFFFFFFF);
    mgmt->sec_session.session_expire_ms = now_ms + ((uint64_t)duration_s * 1000ULL);

    return 0;
}
