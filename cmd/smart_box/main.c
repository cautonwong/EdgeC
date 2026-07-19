#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <uv.h>
#include "edge_log.h"
#include "datahub.h"
#include "contracts.h"

#define GPIO_LED_GREEN "/sys/class/gpio/gpio23/value"
#define GPIO_LED_RED   "/sys/class/gpio/gpio24/value"
#define GPIO_LED_YELLOW "/sys/class/gpio/gpio25/value"
#define GPIO_CAM_VCC   "/sys/class/gpio/gpio29/value"
#define GPIO_MIC_VCC   "/sys/class/gpio/gpio31/value"
#define GPIO_SOS_EDGE  "/sys/class/gpio/gpio26/value"

static datahub_t *g_hub = NULL;
static int g_blink_phase = 0;

static void write_gpio(const char *path, const char *val) {
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, val, 1);
        close(fd);
    }
}

static void timer_hw_poll_cb(uv_timer_t *handle) {
    (void)handle;
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

    uv_timer_t hw_timer;
    uv_timer_init(loop, &hw_timer);
    uv_timer_start(&hw_timer, timer_hw_poll_cb, 0, 500); // 500ms 刷新率

    uv_fs_event_t sos_event;
    uv_fs_event_init(loop, &sos_event);
    uv_fs_event_start(&sos_event, poll_sos_cb, GPIO_SOS_EDGE, 0);

    LOG_INFO(LOG_CAT_CORE, "Smart Box Loop with HW polling initialized");
    return uv_run(loop, UV_RUN_DEFAULT);
}
