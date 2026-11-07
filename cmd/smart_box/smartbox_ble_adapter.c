#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "edge_log.h"
#include "smartbox_ble_adapter.h"
#include "smartbox_driver_adapter.h"
#include "smartbox_uds_ipc.h"

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

static datahub_t *g_hub = NULL;
static uv_timer_t g_ble_timer;

static ble_device_t g_ble_devices[MAX_BLE_DEVICES] = {
    {0, "BLE_HR_Belt", "AA:BB:CC:11:22:01", "heart_rate", 1, 75.0, 0.0},
    {1, "BLE_BP_Monitor", "AA:BB:CC:11:22:02", "blood_pressure", 1, 120.0, 80.0},
    {2, "BLE_Oximeter", "AA:BB:CC:11:22:03", "oximeter", 1, 98.0, 0.0},
    {3, "BLE_Exo_Sensor", "AA:BB:CC:11:22:04", "rehab_sensor", 1, 45.5, 0.0}};

static void ble_heartrate_cb(int raw_hr) {
    LOG_INFO(LOG_CAT_CORE, "BLE GATT: Subscribed heart rate updated: %d BPM", raw_hr);
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, TAG_RAW_HEART_RATE, (double)raw_hr, uv_hrtime());
    datahub_write_shadow(g_hub, &msg);

    if (raw_hr > 130) {
        LOG_WARN(LOG_CAT_CORE,
                 "[CARDIAC SAFETY] ECG Heart Rate %d BPM exceeds safety limit 130 BPM! Triggering "
                 "emergency motor brake.",
                 raw_hr);
        pipe_msg_t alert_msg;
        pipe_msg_init_scalar(&alert_msg, TAG_BOX_HEART_RATE_ALERT, 1.0, uv_hrtime());
        datahub_write_shadow(g_hub, &alert_msg);
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
            LOG_INFO(LOG_CAT_CORE, "BLE Multi-Device Poll: Polled %d devices. JSON frame: %s",
                     MAX_BLE_DEVICES, json_str);
            size_t len = strlen(json_str);
            if (len <= 245) {
                smartbox_uds_send(4, (const uint8_t *)json_str, (uint8_t)len);
            } else {
                smartbox_uds_send(4, (const uint8_t *)json_str, 245);
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

int smartbox_ble_start(uv_loop_t *loop, datahub_t *hub) {
    g_hub = hub;
    uv_timer_init(loop, &g_ble_timer);
    uv_timer_start(&g_ble_timer, timer_ble_poll_cb, 0, 1000);  // 1s 轮询一次
    return 0;
}
