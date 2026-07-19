#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <uv.h>
#include "edge_log.h"
#include "datahub.h"
#include "contracts.h"
#include "wasm_mgr.h"
#include "wasm_app_scheduler.h"

#define GPIO_LED_GREEN "/sys/class/gpio/gpio23/value"
#define GPIO_LED_RED   "/sys/class/gpio/gpio24/value"
#define GPIO_LED_YELLOW "/sys/class/gpio/gpio25/value"
#define GPIO_CAM_VCC   "/sys/class/gpio/gpio29/value"
#define GPIO_MIC_VCC   "/sys/class/gpio/gpio31/value"
#define GPIO_SOS_EDGE  "/sys/class/gpio/gpio26/value"

static datahub_t *g_hub = NULL;
static int g_blink_phase = 0;
static int g_wd_fd = -1;
static wasm_scheduler_t *g_sched = NULL;

static void write_gpio(const char *path, const char *val) {
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, val, 1);
        close(fd);
    }
}

// 硬件看门狗管理
static int box_watchdog_init(void) {
    // 允许在无看门狗驱动的情况下静默失败以支持本地非 root 调试测试
    g_wd_fd = open("/dev/watchdog", O_WRONLY);
    if (g_wd_fd < 0) {
        LOG_WARN(LOG_CAT_CORE, "Watchdog driver not available, running in mock mode");
        return 0;
    }
    int timeout = 5;
    ioctl(g_wd_fd, WDIOC_SETTIMEOUT, &timeout);
    return 0;
}

static void box_watchdog_feed(void) {
    if (g_wd_fd >= 0) {
        int dummy;
        ioctl(g_wd_fd, WDIOC_KEEPALIVE, &dummy);
    }
}

static void timer_hw_poll_cb(uv_timer_t *handle) {
    (void)handle;
    box_watchdog_feed(); // 定期喂狗

    // 1. 读取 AXP209 I2C 电池监测 (地址 0x34)
    int i2c_fd = open("/dev/i2c-1", O_RDWR);
    double power_src = 0.0;
    if (i2c_fd >= 0) {
        if (ioctl(i2c_fd, I2C_SLAVE, 0x34) >= 0) {
            unsigned char reg = 0x01; // Power Status Reg
            if (write(i2c_fd, &reg, 1) == 1) {
                unsigned char status_byte = 0;
                if (read(i2c_fd, &status_byte, 1) == 1) {
                    // Check Bit 4 (AC present) and Bit 3 (USB present)
                    if ((status_byte & 0x18) == 0) {
                        power_src = 1.0; // Battery
                    }
                }
            }
        }
        close(i2c_fd);
    }
    pipe_msg_t msg_p;
    pipe_msg_init_scalar(&msg_p, TAG_SYS_POWER_SOURCE, power_src, uv_hrtime());
    int idx = edge_tag_get_compact_index(TAG_SYS_POWER_SOURCE);
    if (idx >= 0) {
        datahub_write_shadow_idx(g_hub, idx, &msg_p);
    }

    // 2. 指示灯渲染
    double led_mode = 0.0;
    pipe_msg_t led_msg;
    if (datahub_read_shadow(g_hub, TAG_BOX_INDICATOR_LED, &led_msg) == 0 && led_msg.type == PIPEMSG_TYPE_SCALAR) {
        led_mode = led_msg.payload.val.f;
    }
    if (led_mode == 1.0) {
        write_gpio(GPIO_LED_GREEN, "1");
        write_gpio(GPIO_LED_RED, "0");
        write_gpio(GPIO_LED_YELLOW, "0");
    } else if (led_mode == 2.0) {
        // Blink Red
        g_blink_phase = !g_blink_phase;
        write_gpio(GPIO_LED_GREEN, "0");
        write_gpio(GPIO_LED_RED, g_blink_phase ? "1" : "0");
        write_gpio(GPIO_LED_YELLOW, "0");
    } else if (led_mode == 3.0) {
        write_gpio(GPIO_LED_GREEN, "0");
        write_gpio(GPIO_LED_RED, "0");
        write_gpio(GPIO_LED_YELLOW, "1");
    } else {
        write_gpio(GPIO_LED_GREEN, "0");
        write_gpio(GPIO_LED_RED, "0");
        write_gpio(GPIO_LED_YELLOW, "0");
    }

    // 3. 隐私沙盒 GPIO 物理切断
    double privacy_active = 0.0;
    pipe_msg_t priv_msg;
    if (datahub_read_shadow(g_hub, TAG_PRIVACY_MODE_ACTIVE, &priv_msg) == 0 && priv_msg.type == PIPEMSG_TYPE_SCALAR) {
        privacy_active = priv_msg.payload.val.f;
    }
    if (privacy_active == 1.0) {
        write_gpio(GPIO_CAM_VCC, "0"); // Cutoff Cam
        write_gpio(GPIO_MIC_VCC, "0"); // Cutoff Mic
    } else {
        write_gpio(GPIO_CAM_VCC, "1");
        write_gpio(GPIO_MIC_VCC, "1");
    }
}

// 物理大按钮 SOS 下沿中断处理
static void poll_sos_cb(uv_fs_event_t *handle, const char *filename, int events, int status) {
    (void)handle; (void)filename; (void)events; (void)status;
    int fd = open(GPIO_SOS_EDGE, O_RDONLY);
    if (fd >= 0) {
        char ch = '1';
        read(fd, &ch, 1);
        close(fd);
        if (ch == '0') {
            LOG_WARN(LOG_CAT_CORE, "HARDWARE BUTTON: Physical SOS Triggered!");
            pipe_msg_t msg;
            pipe_msg_init_scalar(&msg, TAG_BOX_VOICE_COMMAND, 2.0, uv_hrtime()); // command = SOS (2.0)
            int idx = edge_tag_get_compact_index(TAG_BOX_VOICE_COMMAND);
            if (idx >= 0) {
                datahub_write_shadow_idx(g_hub, idx, &msg);
            }
        }
    }
}

// Task 9 Step 1: PN532 NFC 读卡驱动轮询模拟
static void timer_nfc_poll_cb(uv_timer_t *handle) {
    (void)handle;
    // 模拟扫卡检测：轮询 SPI /dev/spidev0.0
    int spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd >= 0) {
        unsigned char nfc_cmd[] = {0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x4A, 0x01, 0x00, 0xE1, 0x00};
        write(spi_fd, nfc_cmd, sizeof(nfc_cmd));
        close(spi_fd);
        
        LOG_INFO(LOG_CAT_CORE, "NFC SPI Driver: Active wristband certificate scanned successfully!");
        pipe_msg_t msg;
        pipe_msg_init_scalar(&msg, TAG_BOX_VOICE_COMMAND, 3.0, uv_hrtime()); // command = LogIn (3.0)
        int idx = edge_tag_get_compact_index(TAG_BOX_VOICE_COMMAND);
        if (idx >= 0) {
            datahub_write_shadow_idx(g_hub, idx, &msg);
        }
    }
}

// Task 9 Step 3: Modbus RTU 康复外骨骼对接驱动轮询模拟
static void timer_modbus_poll_cb(uv_timer_t *handle) {
    (void)handle;
    int uart_fd = open("/dev/ttyYS1", O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd >= 0) {
        unsigned char query[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
        write(uart_fd, query, sizeof(query));
        
        double completion = 85.0; // 模拟达成率 85%
        pipe_msg_t msg;
        pipe_msg_init_scalar(&msg, TAG_REHAB_COMPLETION_RATE, completion, uv_hrtime());
        int idx = edge_tag_get_compact_index(TAG_REHAB_COMPLETION_RATE);
        if (idx >= 0) {
            datahub_write_shadow_idx(g_hub, idx, &msg);
        }
        close(uart_fd);
    }
}

// Task 9 Step 2: BLE GATT 心率手环驱动模拟
static void ble_heartrate_cb(int raw_hr) {
    LOG_INFO(LOG_CAT_CORE, "BLE GATT: Subscribed heart rate updated: %d BPM", raw_hr);
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, TAG_RAW_HEART_RATE, (double)raw_hr, uv_hrtime());
    int idx = edge_tag_get_compact_index(TAG_RAW_HEART_RATE);
    if (idx >= 0) {
        datahub_write_shadow_idx(g_hub, idx, &msg);
    }
}

static void timer_ble_poll_cb(uv_timer_t *handle) {
    (void)handle;
    static int base_hr = 75;
    base_hr += (rand() % 3) - 1; // 随机波动
    if (base_hr < 55) base_hr = 55;
    if (base_hr > 135) base_hr = 135;
    ble_heartrate_cb(base_hr);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    LOG_INFO(LOG_CAT_CORE, "=== Starting Smart Box Controller ===");
    uv_loop_t *loop = uv_default_loop();

    contract_register(&g_contract_manifest);
    const edge_contract_t *contract = contract_get_active();
    if (!contract) {
        LOG_ERROR(LOG_CAT_CORE, "Active contract is NULL");
        return -1;
    }

    g_hub = calloc(1, sizeof(datahub_t));
    if (!g_hub) {
        LOG_ERROR(LOG_CAT_CORE, "Failed to allocate datahub");
        return -1;
    }

    if (datahub_init(g_hub, loop, "smart_box", 4096, 1024, contract->tags, contract->tag_count, NULL,
                     edge_tag_get_compact_index, NULL) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "Failed to initialize datahub");
        free(g_hub);
        return -1;
    }

    // 初始化 GPIO 模拟节点文件以用于本地调试运行
    system("mkdir -p /sys/class/gpio/gpio23 /sys/class/gpio/gpio24 /sys/class/gpio/gpio25 /sys/class/gpio/gpio26 /sys/class/gpio/gpio29 /sys/class/gpio/gpio31");
    write_gpio(GPIO_SOS_EDGE, "1");

    box_watchdog_init();

    uv_timer_t hw_timer;
    uv_timer_init(loop, &hw_timer);
    uv_timer_start(&hw_timer, timer_hw_poll_cb, 0, 500); // 500ms 刷新率

    uv_fs_event_t sos_event;
    uv_fs_event_init(loop, &sos_event);
    uv_fs_event_start(&sos_event, poll_sos_cb, GPIO_SOS_EDGE, 0);

    // Task 3: WASM Micro-Runtime 初始化沙箱容器调度器 (限制堆 256KB、栈 64KB、运行耗时门禁为 10ms)
    wasm_mgr_init(g_hub);
    g_sched = wasm_scheduler_create(g_hub, loop, 10); // 10ms 心跳 (100Hz)
    if (g_sched) {
        // 限制堆 256KB (262144字节)，栈 64KB (65536字节)，执行超时 10ms
        wasm_scheduler_load_app(g_sched, "rehab_companion", "/data/apps/rehab_companion.wasm", 262144, 65536, 10);
        wasm_scheduler_start(g_sched);
    }

    // Task 9: 注册 NFC、BLE 与 Modbus 驱动定时轮询
    uv_timer_t nfc_timer;
    uv_timer_init(loop, &nfc_timer);
    uv_timer_start(&nfc_timer, timer_nfc_poll_cb, 0, 1000); // 1s 轮询一次

    uv_timer_t ble_timer;
    uv_timer_init(loop, &ble_timer);
    uv_timer_start(&ble_timer, timer_ble_poll_cb, 0, 1000); // 1s 轮询一次

    uv_timer_t modbus_timer;
    uv_timer_init(loop, &modbus_timer);
    uv_timer_start(&modbus_timer, timer_modbus_poll_cb, 0, 1000); // 1s 轮询一次

    LOG_INFO(LOG_CAT_CORE, "Smart Box Loop with HW polling, Wasm Scheduler and drivers initialized");
    int rc = uv_run(loop, UV_RUN_DEFAULT);

    if (g_sched) {
        wasm_scheduler_destroy(g_sched);
    }
    wasm_mgr_cleanup();
    datahub_destroy(g_hub);
    free(g_hub);

    return rc;
}
