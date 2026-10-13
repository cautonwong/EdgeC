#ifndef IO_EXECUTOR_H
#define IO_EXECUTOR_H

#include <stdbool.h>
#include <stdint.h>
typedef struct uv_loop_s uv_loop_t;

#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- IO 抽象接口定义 --- */

typedef enum {
    IO_EVENT_RX,      /**< 收到原始数据 / MQTT 消息 */
    IO_EVENT_TX_DONE, /**< 异步发送完成 / MQTT 发布成功 */
    IO_EVENT_ERROR,   /**< 物理层链路错误 */
} io_event_t;

typedef enum {
    IO_TYPE_UART,
    IO_TYPE_UDS,
    IO_TYPE_UDP,
    IO_TYPE_HTTP,
    IO_TYPE_MQTT, /**< [新增] MQTT IO 类型 */
    IO_TYPE_CUSTOM
} io_type_t;

// 控制指令定义
#define IO_CTL_SET_BAUD 1
#define IO_CTL_FLUSH 2
#define IO_CTL_HTTP_SET_URL 10
#define IO_CTL_HTTP_SET_METHOD 11
#define IO_CTL_HTTP_ADD_HEADER 12

// MQTT 专用控制指令
#define IO_CTL_MQTT_SET_TOPIC 20
#define IO_CTL_MQTT_SUBSCRIBE 21

typedef struct io_handle_s io_handle_t;

typedef struct {
    int (*enable_read)(io_handle_t *h, bool enable);
    int (*write_async)(io_handle_t *h, net_buf_t *buf);
    int (*ioctl)(io_handle_t *h, int cmd, void *arg);
    void (*destroy)(io_handle_t *h);
} io_ops_t;

struct io_handle_s {
    const io_ops_t *ops;
    void (*cb)(io_handle_t *h, io_event_t evt, void *payload, void *user_data);
    void *user_data;
};

typedef struct {
    io_type_t type;
    union {
        struct {
            char dev[64];
            uint32_t baud;
        } uart;
        struct {
            char path[128];
        } uds;
        struct {
            char local_ip[32];
            uint16_t local_port;
            char remote_ip[32];
            uint16_t remote_port;
        } udp;
        struct {
            char url[256];
            char method[16];
        } http;
        struct {
            char broker[128];
            char client_id[64];
            char topic[128];
        } mqtt;
    } params;
} io_config_t;

io_handle_t *io_executor_create(uv_loop_t *loop, const io_config_t *cfg, net_buf_pool_t *pool);
void io_executor_cleanup(void);
io_handle_t *io_executor_create_from_fd(uv_loop_t *loop, int fd, net_buf_pool_t *pool);
void io_executor_http_request(uv_loop_t *loop, const char *url, const char *method);

#ifdef __cplusplus
}
#endif

#endif  // IO_EXECUTOR_H