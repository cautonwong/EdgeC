#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "datahub.h"
#include "edge_log.h"
#include "smartbox_driver_adapter.h"
#include "smartbox_hw_io_adapter.h"
#include "smartbox_uds_ipc.h"

#define GPIO_LED_GREEN "/sys/class/gpio/gpio23/value"
#define GPIO_LED_RED "/sys/class/gpio/gpio24/value"
#define GPIO_LED_YELLOW "/sys/class/gpio/gpio25/value"
#define GPIO_CAM_VCC "/sys/class/gpio/gpio29/value"
#define GPIO_MIC_VCC "/sys/class/gpio/gpio31/value"
#define GPIO_SOS_EDGE "/sys/class/gpio/gpio26/value"

static datahub_t *g_hub = NULL;
static int g_blink_phase = 0;
static int g_wd_fd = -1;
static uv_timer_t g_hw_timer;
static uv_fs_event_t g_sos_event;

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
        return 1;  // Ethernet
    } else if (access("/sys/class/net/wlan0", F_OK) == 0) {
        LOG_INFO(LOG_CAT_CORE, "Net Priority Check: WiFi (wlan0) active [Ethernet > WiFi > 5G]");
        return 2;  // WiFi
    } else if (access("/sys/class/net/wwan0", F_OK) == 0) {
        LOG_INFO(LOG_CAT_CORE, "Net Priority Check: 5G (wwan0) active [Ethernet > WiFi > 5G]");
        return 3;  // 5G
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
    box_watchdog_feed();  // 定期喂狗

    // 1. 读取 AXP209 I2C 电池监测 (地址 0x34)
    int i2c_fd = open("/dev/i2c-1", O_RDWR);
    double power_src = 0.0;
    double battery_soc = 0.0;
    bool power_valid = false;
    bool soc_valid = false;
    if (i2c_fd >= 0) {
        if (ioctl(i2c_fd, I2C_SLAVE, 0x34) >= 0) {
            unsigned char reg = 0x01;  // Power Status Reg
            if (write(i2c_fd, &reg, 1) == 1) {
                unsigned char status_byte = 0;
                if (read(i2c_fd, &status_byte, 1) == 1) {
                    // Check Bit 4 (AC present) and Bit 3 (USB present)
                    if ((status_byte & 0x18) == 0) {
                        power_src = 1.0;  // Battery
                    }
                    power_valid = true;
                }
            }
            // 读取 0x33 寄存器 (Battery capacity percentage)
            reg = 0x33;
            if (write(i2c_fd, &reg, 1) == 1) {
                unsigned char soc_byte = 0;
                if (read(i2c_fd, &soc_byte, 1) == 1) {
                    battery_soc = (double)(soc_byte & 0x7F);
                    soc_valid = true;
                }
            }
        }
        close(i2c_fd);
    }
    if (power_valid) {
        pipe_msg_t msg_p;
        pipe_msg_init_scalar(&msg_p, TAG_SYS_POWER_SOURCE, power_src, uv_hrtime());
        datahub_write_shadow(g_hub, &msg_p);
    } else {
        LOG_DEBUG(LOG_CAT_CORE, "AXP209: power status unavailable; tag not published");
    }

    if (soc_valid) {
        pipe_msg_t msg_soc;
        pipe_msg_init_scalar(&msg_soc, TAG_BATTERY_SOC, battery_soc, uv_hrtime());
        datahub_write_shadow(g_hub, &msg_soc);
    } else {
        LOG_DEBUG(LOG_CAT_CORE, "AXP209: battery SOC unavailable; tag not published");
    }

    // 低电量预警 (SOC < 15.0%)
    if (soc_valid && battery_soc < 15.0) {
        LOG_WARN(LOG_CAT_CORE, "AXP209: Low battery warning! Battery SOC: %.1f%% (< 15.0%%)",
                 battery_soc);
        char alert_buf[64];
        int alen = snprintf(alert_buf, sizeof(alert_buf),
                            "{\"alert\":\"LOW_BATTERY\",\"soc\":%.1f}", battery_soc);
        smartbox_uds_send(3, (const uint8_t *)alert_buf, (uint8_t)alen);
    }

    // 2. 指示灯渲染 (RGB Status Lights)
    // LED Mode 1 (Green constant) = Device Normal
    // LED Mode 2 (Red blink) = Alarm / Emergency Fault
    // LED Mode 3 (Yellow/Blue constant) = Network Connected
    double led_mode = 0.0;
    pipe_msg_t led_msg;
    if (datahub_read_shadow(g_hub, TAG_BOX_INDICATOR_LED, &led_msg) == 0 &&
        led_msg.type == PIPEMSG_TYPE_SCALAR) {
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
    if (datahub_read_shadow(g_hub, TAG_PRIVACY_MODE_ACTIVE, &priv_msg) == 0 &&
        priv_msg.type == PIPEMSG_TYPE_SCALAR) {
        privacy_active = priv_msg.payload.val.f;
    }
    if (privacy_active == 1.0) {
        write_gpio(GPIO_CAM_VCC, "0");  // Cutoff Cam
        write_gpio(GPIO_MIC_VCC, "0");  // Cutoff Mic
    } else {
        write_gpio(GPIO_CAM_VCC, "1");
        write_gpio(GPIO_MIC_VCC, "1");
    }

    // 4. 网络优先级检测 (Ethernet > WiFi > 5G) & ALSA USB 音频路由检测
    int net_type = net_priority_check();
    pipe_msg_t msg_net;
    pipe_msg_init_scalar(&msg_net, TAG_NET_ACTIVE_INTERFACE, (double)(net_type - 1), uv_hrtime());
    datahub_write_shadow(g_hub, &msg_net);

    int alsa_found = alsa_usb_audio_check();
    pipe_msg_t msg_alsa;
    pipe_msg_init_scalar(&msg_alsa, TAG_ALSA_AUDIO_ROUTE, (double)(alsa_found > 0 ? 1 : 0),
                         uv_hrtime());
    datahub_write_shadow(g_hub, &msg_alsa);

    pipe_msg_t msg_rgb;
    pipe_msg_init_scalar(&msg_rgb, TAG_LED_RGB_STATUS, led_mode, uv_hrtime());
    datahub_write_shadow(g_hub, &msg_rgb);
}

// 物理大按钮 SOS 下沿中断处理
static void poll_sos_cb(uv_fs_event_t *handle, const char *filename, int events, int status) {
    (void)handle;
    (void)filename;
    (void)events;
    (void)status;
    int fd = open(GPIO_SOS_EDGE, O_RDONLY);
    if (fd >= 0) {
        char ch = '1';
        read(fd, &ch, 1);
        close(fd);
        if (ch == '0') {
            LOG_WARN(LOG_CAT_CORE, "HARDWARE BUTTON: Physical SOS Triggered!");
            pipe_msg_t msg;
            pipe_msg_init_scalar(&msg, TAG_BOX_VOICE_COMMAND, 2.0,
                                 uv_hrtime());  // command = SOS (2.0)
            datahub_write_shadow(g_hub, &msg);
            // 通过 UDS IPC 向 Go 后端发送二进制报警帧 (MsgType = 3)
            smartbox_uds_send(3, NULL, 0);
        }
    }
}

int smartbox_hw_io_start(uv_loop_t *loop, datahub_t *hub) {
    g_hub = hub;

    // 初始化 GPIO 节点 (通过安全 HAL 适配，避免 shell 注入风险)
    static const char *gpio_dirs[] = {"/sys/class/gpio/gpio23",
                                      "/sys/class/gpio/gpio24",
                                      "/sys/class/gpio/gpio25",
                                      "/sys/class/gpio/gpio26",
                                      "/sys/class/gpio/gpio29",
                                      "/sys/class/gpio/gpio31",
                                      NULL};
    for (int i = 0; gpio_dirs[i] != NULL; i++) {
        if (access(gpio_dirs[i], F_OK) != 0) {
            mkdir(gpio_dirs[i], 0755);
        }
    }
    write_gpio(GPIO_SOS_EDGE, "1");

    box_watchdog_init();

    uv_timer_init(loop, &g_hw_timer);
    uv_timer_start(&g_hw_timer, timer_hw_poll_cb, 0, 500);  // 500ms 刷新率

    uv_fs_event_init(loop, &g_sos_event);
    uv_fs_event_start(&g_sos_event, poll_sos_cb, GPIO_SOS_EDGE, 0);
    return 0;
}
