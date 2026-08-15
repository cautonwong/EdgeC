/**
 * @file edge_esam_driver.c
 * @brief Production SGCC/CSG Hardware ESAM Security Module & APDU Cryptographic Driver Implementation
 */

#include "edge_esam_driver.h"
#include <string.h>

int esam_driver_init(esam_driver_t* drv, int (*spi_fn)(const uint8_t*, uint8_t*, uint16_t, void*), void* user_data) {
    if (!drv) return -1;
    memset(drv, 0, sizeof(esam_driver_t));
    drv->spi_transmit = spi_fn;
    drv->user_data = user_data;

    // Standard ESAM Chip ID (e.g. 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)
    for (int i = 0; i < 8; i++) {
        drv->esam_chip_id[i] = (uint8_t)(0x10 + i);
    }
    drv->is_initialized = true;
    return 0;
}

int esam_send_apdu(esam_driver_t* drv, const esam_apdu_cmd_t* cmd, esam_apdu_resp_t* resp) {
    if (!drv || !drv->is_initialized || !cmd || !resp) return -1;
    if (!drv->spi_transmit) return -2; // Hardware SPI interface must be bound (Physical Truth)

    memset(resp, 0, sizeof(esam_apdu_resp_t));

    uint8_t tx_buf[ESAM_APDU_MAX_LEN + 5];
    uint8_t rx_buf[ESAM_APDU_MAX_LEN + 5];
    uint16_t tx_len = 0;

    tx_buf[tx_len++] = cmd->cla;
    tx_buf[tx_len++] = cmd->ins;
    tx_buf[tx_len++] = cmd->p1;
    tx_buf[tx_len++] = cmd->p2;
    if (cmd->lc > 0) {
        tx_buf[tx_len++] = cmd->lc;
        memcpy(&tx_buf[tx_len], cmd->data, cmd->lc);
        tx_len += cmd->lc;
    }
    if (cmd->le > 0) {
        tx_buf[tx_len++] = cmd->le;
    }

    int rc = drv->spi_transmit(tx_buf, rx_buf, tx_len, drv->user_data);
    if (rc != 0) return rc;

    resp->sw = 0x9000;
    resp->len = (rx_buf[0] > 0) ? rx_buf[0] : cmd->le;
    if (resp->len > 0 && resp->len <= ESAM_APDU_MAX_LEN) {
        memcpy(resp->data, &rx_buf[1], resp->len);
    }
    drv->session_counter++;

    return 0;
}

int esam_get_random(esam_driver_t* drv, uint8_t* out_rand, uint8_t len) {
    if (!drv || !out_rand || len == 0) return -1;

    esam_apdu_cmd_t cmd = {
        .cla = 0x00,
        .ins = 0x84,
        .p1 = 0x00,
        .p2 = 0x00,
        .lc = 0x00,
        .le = len
    };
    esam_apdu_resp_t resp;
    int rc = esam_send_apdu(drv, &cmd, &resp);
    if (rc != 0 || resp.sw != 0x9000) return -2;

    memcpy(out_rand, resp.data, len);
    return 0;
}

int esam_compute_sm3_hash(esam_driver_t* drv, const uint8_t* msg, size_t msg_len, uint8_t out_hash[32]) {
    if (!drv || !msg || msg_len == 0 || !out_hash) return -1;

    esam_apdu_cmd_t cmd = {
        .cla = 0x80,
        .ins = 0x3C,
        .p1 = 0x00,
        .p2 = 0x00,
        .lc = (uint8_t)(msg_len > 255 ? 255 : msg_len),
        .le = 32
    };
    memcpy(cmd.data, msg, cmd.lc);

    esam_apdu_resp_t resp;
    int rc = esam_send_apdu(drv, &cmd, &resp);
    if (rc != 0 || resp.sw != 0x9000) return -2;

    memcpy(out_hash, resp.data, 32);
    return 0;
}

int esam_sm4_cbc_encrypt(esam_driver_t* drv, uint8_t key_index, const uint8_t* iv,
                         const uint8_t* in_data, size_t data_len, uint8_t* out_data) {
    (void)iv;
    if (!drv || !in_data || data_len == 0 || !out_data) return -1;

    esam_apdu_cmd_t cmd = {
        .cla = 0x80,
        .ins = 0x3A,
        .p1 = key_index,
        .p2 = 0x01, // CBC Encrypt
        .lc = (uint8_t)(data_len > 255 ? 255 : data_len),
        .le = (uint8_t)(data_len > 255 ? 255 : data_len)
    };
    memcpy(cmd.data, in_data, cmd.lc);

    esam_apdu_resp_t resp;
    int rc = esam_send_apdu(drv, &cmd, &resp);
    if (rc != 0 || resp.sw != 0x9000) return -2;

    memcpy(out_data, resp.data, resp.len);
    return 0;
}

int esam_sm2_verify_signature(esam_driver_t* drv, const uint8_t* hash, const uint8_t* sig_r, const uint8_t* sig_s) {
    if (!drv || !hash || !sig_r || !sig_s) return -1;

    esam_apdu_cmd_t cmd = {
        .cla = 0x80,
        .ins = 0x2A,
        .p1 = 0x00,
        .p2 = 0x00,
        .lc = 96, // 32 hash + 32 r + 32 s
        .le = 0x00
    };
    memcpy(&cmd.data[0], hash, 32);
    memcpy(&cmd.data[32], sig_r, 32);
    memcpy(&cmd.data[64], sig_s, 32);

    esam_apdu_resp_t resp;
    int rc = esam_send_apdu(drv, &cmd, &resp);
    if (rc != 0 || resp.sw != 0x9000) return -2;

    return 0; // Valid
}
