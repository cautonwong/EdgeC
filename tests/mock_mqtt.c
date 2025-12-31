#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义与 Paho MQTT 一致的接口，但不引入其头文件以避免冲突，
// 或者我们需要复制一份最小化的头文件定义。
// 为了简单，我们这里假设 Paho 的头文件在 include 路径中可用，只实现符号。

#include <MQTTAsync.h>

// 全局断言状态
char g_mock_mqtt_topic[256] = {0};
char g_mock_mqtt_payload[512] = {0};
int g_mock_mqtt_send_count = 0;

void mock_mqtt_reset(void) {
    memset(g_mock_mqtt_topic, 0, sizeof(g_mock_mqtt_topic));
    memset(g_mock_mqtt_payload, 0, sizeof(g_mock_mqtt_payload));
    g_mock_mqtt_send_count = 0;
}

int MQTTAsync_create(MQTTAsync* handle, const char* serverURI, const char* clientId,
                     int persistence_type, void* persistence_context) {
    (void)serverURI;
    (void)clientId;
    (void)persistence_type;
    (void)persistence_context;
    *handle = (void*)0xDEADBEEF;
    return MQTTASYNC_SUCCESS;
}

int MQTTAsync_connect(MQTTAsync handle, const MQTTAsync_connectOptions* options) {
    (void)handle;
    // 模拟立即连接成功
    if (options && options->onSuccess) {
        // 构造一个假的 successData (Paho 结构体比较复杂，这里传 NULL 通常是安全的，取决于回调实现)
        options->onSuccess(options->context, NULL);
    }
    return MQTTASYNC_SUCCESS;
}

int MQTTAsync_disconnect(MQTTAsync handle, const MQTTAsync_disconnectOptions* options) {
    (void)handle;
    (void)options;
    return MQTTASYNC_SUCCESS;
}

int MQTTAsync_send(MQTTAsync handle, const char* destinationName, int payloadlen,
                   const void* payload, int qos, int retained,
                   MQTTAsync_responseOptions* response) {
    (void)handle;
    (void)qos;
    (void)retained;
    (void)response;

    strncpy(g_mock_mqtt_topic, destinationName, 255);

    // Payload 可能是二进制，但我们只测试 JSON 字符串
    int copy_len = payloadlen < 511 ? payloadlen : 511;
    memcpy(g_mock_mqtt_payload, payload, copy_len);
    g_mock_mqtt_payload[copy_len] = '\0';

    g_mock_mqtt_send_count++;
    return MQTTASYNC_SUCCESS;
}

void MQTTAsync_destroy(MQTTAsync* handle) {
    *handle = NULL;
}
