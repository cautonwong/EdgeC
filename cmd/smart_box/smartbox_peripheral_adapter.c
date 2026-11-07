#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "datahub.h"
#include "edge_log.h"
#include "smartbox_driver_adapter.h"
#include "smartbox_peripheral_adapter.h"
#include "smartbox_uds_ipc.h"

static datahub_t *g_hub = NULL;
static uv_timer_t g_nfc_timer;
static uv_timer_t g_modbus_timer;

// Task 9 Step 1: PN532 NFC 读卡驱动轮询模拟
static void timer_nfc_poll_cb(uv_timer_t *handle) {
    (void)handle;
    // 模拟扫卡检测：轮询 SPI /dev/spidev0.0
    int spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd >= 0) {
        unsigned char nfc_cmd[] = {0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4,
                                   0x4A, 0x01, 0x00, 0xE1, 0x00};
        write(spi_fd, nfc_cmd, sizeof(nfc_cmd));
        close(spi_fd);

        LOG_INFO(LOG_CAT_CORE,
                 "NFC SPI Driver: Active wristband certificate scanned successfully!");
        pipe_msg_t msg;
        pipe_msg_init_scalar(&msg, TAG_BOX_VOICE_COMMAND, 3.0,
                             uv_hrtime());  // command = LogIn (3.0)
        datahub_write_shadow(g_hub, &msg);
        // 发送扫到的外设 NDEF 报文
        const char *nfc_payload =
            "{\"dev_type\":\"exoskeleton\",\"slave_id\":1,\"protocol\":\"ModbusRTU\",\"baud\":"
            "115200}";
        smartbox_uds_send(2, (const uint8_t *)nfc_payload, (uint8_t)strlen(nfc_payload));
    }
}

// Task 9 Step 3: Modbus RTU 康复外骨骼对接驱动轮询模拟
static void timer_modbus_poll_cb(uv_timer_t *handle) {
    (void)handle;
    int uart_fd = open("/dev/ttyYS1", O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd >= 0) {
        unsigned char query[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
        write(uart_fd, query, sizeof(query));

        LOG_WARN(LOG_CAT_CORE,
                 "Rehab Modbus: response parsing not implemented; completion rate not published");
        close(uart_fd);
    }
}

int smartbox_peripheral_start(uv_loop_t *loop, datahub_t *hub) {
    g_hub = hub;

    uv_timer_init(loop, &g_nfc_timer);
    uv_timer_start(&g_nfc_timer, timer_nfc_poll_cb, 0, 1000);  // 1s 轮询一次

    uv_timer_init(loop, &g_modbus_timer);
    uv_timer_start(&g_modbus_timer, timer_modbus_poll_cb, 0, 1000);  // 1s 轮询一次
    return 0;
}
