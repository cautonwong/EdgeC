/**
 * @file north_ipc_port.h
 * @brief 现场 MQTT `ipc/{sn}/...` binaryFlow 端点端口。
 *
 * 消费者是 `edge_north` 的订阅编排；实现由产品提供（只有产品认识设备清单），
 * 组合根注入。载荷是原始帧，编排层不解析。
 *
 * 现场若 broker ACL 禁止 `ipc/{sn}/#` 通配，需回退逐设备精确订阅，
 * 并同时重定 `MQTT_MAX_SUBS` 与传输层预算，不能只改本文件。
 */

#ifndef NORTH_IPC_PORT_H
#define NORTH_IPC_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 与 mqtt_producer.c 的订阅主题上限对齐；渲染结果达到该长度即失败，不截断。 */
#define NORTH_IPC_TOPIC_MAX 128
/* sn 之外还要放 module/dev_id/type 与动词，上限取 64 含结尾 NUL。 */
#define NORTH_IPC_SN_MAX 64

typedef enum {
    NORTH_IPC_SET = 0, /* ipc/{sn}/{module}/{dev_id}/{type}/Set/binaryFlow */
    NORTH_IPC_GET,     /* .../Get/binaryFlow */
    NORTH_IPC_SET_RESP,
    NORTH_IPC_GET_RESP
} north_ipc_verb_t;

typedef struct {
    const char *module;
    const char *dev_id;
    const char *type;
} north_ipc_endpoint_t;

typedef struct {
    void *ctx;
    size_t (*endpoint_count)(void *ctx);
    /* 返回 0 并填出第 index 个端点；字符串只在本次调用内有效。 */
    int (*endpoint_at)(void *ctx, size_t index, north_ipc_endpoint_t *out);
    /* 同步运行在 MQTT 回调线程；payload 只在回调内有效，实现方须当场解码或复制。
     * 返回 <0 = 拒绝；编排层只记日志，不重投、不代发回执。 */
    int (*on_frame)(void *ctx, const north_ipc_endpoint_t *ep, north_ipc_verb_t verb,
                    const uint8_t *payload, size_t len);
} north_ipc_port_t;

/* 纯函数。段必须非空且仅含 [A-Za-z0-9_.-]，总长必须 < NORTH_IPC_TOPIC_MAX 且
 * < out_sz。任一条不满足返回 -1，out 一个字节都不写。 */
int north_ipc_render_topic(const char *sn, const north_ipc_endpoint_t *ep,
                           north_ipc_verb_t verb, char *out, size_t out_sz);

bool north_ipc_topic_level_ok(const char *value);

#endif /* NORTH_IPC_PORT_H */
