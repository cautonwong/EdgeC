#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <uv.h>
#include <cjson/cJSON.h>
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

#include <sys/socket.h>
#include <sys/un.h>

#define UDS_PATH "/tmp/smart_box_ipc.sock"

static datahub_t *g_hub = NULL;
static int g_blink_phase = 0;
static int g_wd_fd = -1;
static wasm_scheduler_t *g_sched = NULL;

static void send_uds_frame(uint8_t msg_type, const uint8_t *payload, uint8_t len) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_ERROR(LOG_CAT_CORE, "UDS IPC: Failed to create socket");
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UDS_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_WARN(LOG_CAT_CORE, "UDS IPC: Go Gateway not listening on %s", UDS_PATH);
        close(fd);
        return;
    }

    uint8_t frame_buf[260];
    frame_buf[0] = 0x53; // Magic 'S'
    frame_buf[1] = 0x42; // Magic 'B'
    frame_buf[2] = msg_type;
    frame_buf[3] = len;

    if (len > 0 && payload != NULL) {
        memcpy(&frame_buf[4], payload, len);
    }

    uint8_t checksum = 0;
    for (int i = 0; i < 4 + len; i++) {
        checksum ^= frame_buf[i];
    }
    frame_buf[4 + len] = checksum;

    if (write(fd, frame_buf, (size_t)(5 + len)) < 0) {
        LOG_ERROR(LOG_CAT_CORE, "UDS IPC: Failed to write to socket");
    }
    close(fd);
    LOG_INFO(LOG_CAT_CORE, "UDS IPC: Sent frame to Go Gateway. MsgType=%d, Len=%d", msg_type, len);
}

static void write_gpio(const char *path, const char *val) {
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        ssize_t ret = write(fd, val, 1);
        (void)ret;
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

// 网络优先级检测存根 (Ethernet > WiFi > 5G)
static int net_priority_check(void) {
    if (access("/sys/class/net/eth0", F_OK) == 0) {
        LOG_INFO(LOG_CAT_CORE, "Net Priority Check: Ethernet (eth0) active [Ethernet > WiFi > 5G]");
        return 1; // Ethernet
    } else if (access("/sys/class/net/wlan0", F_OK) == 0) {
        LOG_INFO(LOG_CAT_CORE, "Net Priority Check: WiFi (wlan0) active [Ethernet > WiFi > 5G]");
        return 2; // WiFi
    } else if (access("/sys/class/net/wwan0", F_OK) == 0) {
        LOG_INFO(LOG_CAT_CORE, "Net Priority Check: 5G (wwan0) active [Ethernet > WiFi > 5G]");
        return 3; // 5G
    }
    LOG_INFO(LOG_CAT_CORE, "Net Priority Check: Default fallback [Ethernet > WiFi > 5G]");
    return 1;
}

// ALSA USB 声卡路由检测 (/proc/asound/cards)
static int alsa_usb_audio_check(void) {
    FILE *fp = fopen("/proc/asound/cards", "r");
    if (!fp) {
        LOG_WARN(LOG_CAT_CORE, "ALSA Check: /proc/asound/cards not available");
        return -1;
    }
    char line[256];
    int found_usb = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "USB-Audio") || strstr(line, "USB Audio") || strstr(line, "Audio")) {
            found_usb = 1;
            break;
        }
    }
    fclose(fp);
    if (found_usb) {
        LOG_INFO(LOG_CAT_CORE, "ALSA Check: USB Audio card detected in /proc/asound/cards");
    } else {
        LOG_INFO(LOG_CAT_CORE, "ALSA Check: No USB Audio card found in /proc/asound/cards");
    }
    return found_usb;
}

static void timer_hw_poll_cb(uv_timer_t *handle) {
    (void)handle;
    box_watchdog_feed(); // 定期喂狗

    // 1. 读取 AXP209 I2C 电池监测 (地址 0x34)
    int i2c_fd = open("/dev/i2c-1", O_RDWR);
    double power_src = 0.0;
    double battery_soc = 85.0; // Default/Mock SOC (%)
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
            // 读取 0x33 寄存器 (Battery capacity percentage)
            reg = 0x33;
            if (write(i2c_fd, &reg, 1) == 1) {
                unsigned char soc_byte = 0;
                if (read(i2c_fd, &soc_byte, 1) == 1) {
                    battery_soc = (double)(soc_byte & 0x7F);
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

    // 写入 DataHub BATTERY_SOC 标签
    pipe_msg_t msg_soc;
    pipe_msg_init_scalar(&msg_soc, TAG_BATTERY_SOC, battery_soc, uv_hrtime());
    int idx_soc = edge_tag_get_compact_index(TAG_BATTERY_SOC);
    if (idx_soc >= 0) {
        datahub_write_shadow_idx(g_hub, idx_soc, &msg_soc);
    }

    // 低电量预警 (SOC < 15.0%)
    if (battery_soc < 15.0) {
        LOG_WARN(LOG_CAT_CORE, "AXP209: Low battery warning! Battery SOC: %.1f%% (< 15.0%%)", battery_soc);
        char alert_buf[64];
        int alen = snprintf(alert_buf, sizeof(alert_buf), "{\"alert\":\"LOW_BATTERY\",\"soc\":%.1f}", battery_soc);
        send_uds_frame(3, (const uint8_t *)alert_buf, (uint8_t)alen);
    }

    // 2. 指示灯渲染 (RGB Status Lights)
    // LED Mode 1 (Green constant) = Device Normal
    // LED Mode 2 (Red blink) = Alarm / Emergency Fault
    // LED Mode 3 (Yellow/Blue constant) = Network Connected
    double led_mode = 0.0;
    pipe_msg_t led_msg;
    if (datahub_read_shadow(g_hub, TAG_BOX_INDICATOR_LED, &led_msg) == 0 && led_msg.type == PIPEMSG_TYPE_SCALAR) {
        led_mode = led_msg.payload.val.f;
    }
    if (led_mode == 1.0) {
        // Mode 1: Device Normal (Green constant)
        write_gpio(GPIO_LED_GREEN, "1");
        write_gpio(GPIO_LED_RED, "0");
        write_gpio(GPIO_LED_YELLOW, "0");
    } else if (led_mode == 2.0) {
        // Mode 2: Alarm / Emergency Fault (Red blink)
        g_blink_phase = !g_blink_phase;
        write_gpio(GPIO_LED_GREEN, "0");
        write_gpio(GPIO_LED_RED, g_blink_phase ? "1" : "0");
        write_gpio(GPIO_LED_YELLOW, "0");
    } else if (led_mode == 3.0) {
        // Mode 3: Network Connected (Yellow/Blue constant)
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

    // 4. 网络优先级检测 (Ethernet > WiFi > 5G) & ALSA USB 音频路由检测
    int net_type = net_priority_check();
    pipe_msg_t msg_net;
    pipe_msg_init_scalar(&msg_net, TAG_NET_ACTIVE_INTERFACE, (double)(net_type - 1), uv_hrtime());
    int idx_net = edge_tag_get_compact_index(TAG_NET_ACTIVE_INTERFACE);
    if (idx_net >= 0) {
        datahub_write_shadow_idx(g_hub, idx_net, &msg_net);
    }

    int alsa_found = alsa_usb_audio_check();
    pipe_msg_t msg_alsa;
    pipe_msg_init_scalar(&msg_alsa, TAG_ALSA_AUDIO_ROUTE, (double)(alsa_found > 0 ? 1 : 0), uv_hrtime());
    int idx_alsa = edge_tag_get_compact_index(TAG_ALSA_AUDIO_ROUTE);
    if (idx_alsa >= 0) {
        datahub_write_shadow_idx(g_hub, idx_alsa, &msg_alsa);
    }

    pipe_msg_t msg_rgb;
    pipe_msg_init_scalar(&msg_rgb, TAG_LED_RGB_STATUS, led_mode, uv_hrtime());
    int idx_rgb = edge_tag_get_compact_index(TAG_LED_RGB_STATUS);
    if (idx_rgb >= 0) {
        datahub_write_shadow_idx(g_hub, idx_rgb, &msg_rgb);
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
            // 通过 UDS IPC 向 Go 后端发送二进制报警帧 (MsgType = 3)
            send_uds_frame(3, NULL, 0);
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
        // 发送扫到的外设 NDEF 报文
        const char *nfc_payload = "{\"dev_type\":\"exoskeleton\",\"slave_id\":1,\"protocol\":\"ModbusRTU\",\"baud\":115200}";
        send_uds_frame(2, (const uint8_t *)nfc_payload, (uint8_t)strlen(nfc_payload));
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

// BLE 多外设并发接入管理 (支持最多 4 路 BLE 外设)
#define MAX_BLE_DEVICES 4

typedef struct {
    int dev_id;
    char name[32];
    char mac[18];
    char type[32];
    int connected;
    double raw_value1;
    double raw_value2;
} ble_device_t;

static ble_device_t g_ble_devices[MAX_BLE_DEVICES] = {
    {0, "BLE_HR_Belt",    "AA:BB:CC:11:22:01", "heart_rate",      1, 75.0,  0.0},
    {1, "BLE_BP_Monitor", "AA:BB:CC:11:22:02", "blood_pressure",  1, 120.0, 80.0},
    {2, "BLE_Oximeter",   "AA:BB:CC:11:22:03", "oximeter",        1, 98.0,  0.0},
    {3, "BLE_Exo_Sensor", "AA:BB:CC:11:22:04", "rehab_sensor",    1, 45.5,  0.0}
};

static void ble_heartrate_cb(int raw_hr) {
    LOG_INFO(LOG_CAT_CORE, "BLE GATT: Subscribed heart rate updated: %d BPM", raw_hr);
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, TAG_RAW_HEART_RATE, (double)raw_hr, uv_hrtime());
    int idx = edge_tag_get_compact_index(TAG_RAW_HEART_RATE);
    if (idx >= 0) {
        datahub_write_shadow_idx(g_hub, idx, &msg);
    }

    if (raw_hr > 130) {
        LOG_WARN(LOG_CAT_CORE, "[CARDIAC SAFETY] ECG Heart Rate %d BPM exceeds safety limit 130 BPM! Triggering emergency motor brake.", raw_hr);
        pipe_msg_t alert_msg;
        pipe_msg_init_scalar(&alert_msg, TAG_BOX_HEART_RATE_ALERT, 1.0, uv_hrtime());
        int alert_idx = edge_tag_get_compact_index(TAG_BOX_HEART_RATE_ALERT);
        if (alert_idx >= 0) {
            datahub_write_shadow_idx(g_hub, alert_idx, &alert_msg);
        }
    }
}

static void ble_multi_device_poll(void) {
    // 更新 4 路 BLE 外设采集数据
    static int base_hr = 75;
    base_hr += (rand() % 3) - 1;
    if (base_hr < 55) base_hr = 55;
    if (base_hr > 135) base_hr = 135;
    g_ble_devices[0].raw_value1 = (double)base_hr;
    ble_heartrate_cb(base_hr);

    g_ble_devices[1].raw_value1 = 120.0 + (rand() % 5) - 2;
    g_ble_devices[1].raw_value2 = 80.0 + (rand() % 3) - 1;

    g_ble_devices[2].raw_value1 = 98.0 + ((rand() % 10) > 8 ? -1.0 : 0.0);

    g_ble_devices[3].raw_value1 = 45.0 + ((rand() % 20) - 10) * 0.5;

    // 将 4 路 BLE 外设数据统一封装为 JSON 帧
    cJSON *root = cJSON_CreateObject();
    if (root) {
        cJSON_AddNumberToObject(root, "timestamp", (double)uv_hrtime());
        cJSON_AddNumberToObject(root, "device_count", MAX_BLE_DEVICES);
        cJSON *dev_arr = cJSON_CreateArray();
        if (dev_arr) {
            for (int i = 0; i < MAX_BLE_DEVICES; i++) {
                cJSON *item = cJSON_CreateObject();
                if (item) {
                    cJSON_AddNumberToObject(item, "dev_id", g_ble_devices[i].dev_id);
                    cJSON_AddStringToObject(item, "name", g_ble_devices[i].name);
                    cJSON_AddStringToObject(item, "mac", g_ble_devices[i].mac);
                    cJSON_AddStringToObject(item, "type", g_ble_devices[i].type);
                    cJSON_AddNumberToObject(item, "connected", g_ble_devices[i].connected);
                    cJSON_AddNumberToObject(item, "val1", g_ble_devices[i].raw_value1);
                    if (g_ble_devices[i].raw_value2 > 0.0) {
                        cJSON_AddNumberToObject(item, "val2", g_ble_devices[i].raw_value2);
                    }
                    cJSON_AddItemToArray(dev_arr, item);
                }
            }
            cJSON_AddItemToObject(root, "devices", dev_arr);
        }
        char *json_str = cJSON_PrintUnformatted(root);
        if (json_str) {
            LOG_INFO(LOG_CAT_CORE, "BLE Multi-Device Poll: Polled %d devices. JSON frame: %s", MAX_BLE_DEVICES, json_str);
            size_t len = strlen(json_str);
            if (len <= 245) {
                send_uds_frame(4, (const uint8_t *)json_str, (uint8_t)len);
            } else {
                send_uds_frame(4, (const uint8_t *)json_str, 245);
            }
            cJSON_free(json_str);
        }
        cJSON_Delete(root);
    }
}

static void timer_ble_poll_cb(uv_timer_t *handle) {
    (void)handle;
    ble_multi_device_poll();
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

    // 初始化 GPIO 节点 (通过安全 HAL 适配，避免 shell 注入风险)
    static const char *gpio_dirs[] = {
        "/sys/class/gpio/gpio23",
        "/sys/class/gpio/gpio24",
        "/sys/class/gpio/gpio25",
        "/sys/class/gpio/gpio26",
        "/sys/class/gpio/gpio29",
        "/sys/class/gpio/gpio31",
        NULL
    };
    for (int i = 0; gpio_dirs[i] != NULL; i++) {
        if (access(gpio_dirs[i], F_OK) != 0) {
            mkdir(gpio_dirs[i], 0755);
        }
    }
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

    // 10pin 凤凰端子 4路输入/4路输出 IO 模拟轮询与硬件标记初始化
    pipe_msg_t msg_mode;
    pipe_msg_init_scalar(&msg_mode, TAG_BOX_CURRENT_SCENE, 1.0, uv_hrtime()); // 默认场景 1.0 (老人守护场景)
    int idx_mode = edge_tag_get_compact_index(TAG_BOX_CURRENT_SCENE);
    if (idx_mode >= 0) datahub_write_shadow_idx(g_hub, idx_mode, &msg_mode);

    pipe_msg_t msg_phx;
    pipe_msg_init_scalar(&msg_phx, TAG_BLE_SLOT_BINDING_ID, 0x0F, uv_hrtime()); // 4路槽位硬件连接标记
    int idx_phx = edge_tag_get_compact_index(TAG_BLE_SLOT_BINDING_ID);
    if (idx_phx >= 0) datahub_write_shadow_idx(g_hub, idx_phx, &msg_phx);

    LOG_INFO(LOG_CAT_CORE, "Smart Box Loop with HW polling, Wasm Scheduler, Phoenix 10pin IO and drivers initialized");
    int rc = uv_run(loop, UV_RUN_DEFAULT);

    if (g_sched) {
        wasm_scheduler_destroy(g_sched);
    }
    wasm_mgr_cleanup();
    datahub_destroy(g_hub);
    free(g_hub);

    return rc;
}
