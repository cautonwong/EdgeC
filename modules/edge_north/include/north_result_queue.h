#ifndef NORTH_RESULT_QUEUE_H
#define NORTH_RESULT_QUEUE_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 回执（accepted / result）的线程安全投递队列。生产者在任意线程入队 +
 * uv_async_send；消费者（队列所属 loop 线程）drain 后调用 publish 回调。
 * 所有回执都只在 loop 线程编码/发布，驱动里的非原子会话状态（如 Sparkplug
 * seq）不会跨线程并发。 */

#define NB_RESULT_QUEUE_CAP 16
#define NB_RECEIPT_NAME_MAX 64

/* 结果回执状态。数值与 IEMS 执行端口的完成状态一致（0=OK,1=FAILED,2=TIMEOUT）；
 * 线路值（`<name>.result` 的 payload）= 1=OK / 0=FAILED / 2=TIMEOUT。 */
typedef enum {
    NB_RESULT_STATUS_OK = 0,
    NB_RESULT_STATUS_FAILED = 1,
    NB_RESULT_STATUS_TIMEOUT = 2
} nb_result_status_t;

typedef enum {
    NB_RECEIPT_ACCEPTED = 0,
    NB_RECEIPT_RESULT = 1
} nb_receipt_kind_t;

typedef struct {
    uint32_t tag_id;
    int status; /* NB_RESULT_STATUS_*（result 回执） */
    bool ok;    /* accepted 回执 */
    uint8_t kind;
    char name[NB_RECEIPT_NAME_MAX];
} nb_receipt_entry_t;

/* 发布回调：point 已是线路 metric 名（`<name>.ack` / `<name>.result`，空名回落
 * `ack.<tag>` / `result.<tag>`），value 已是线路值。normal 路径在队列所属 loop
 * 线程调用；publish_accepted_now 在调用方线程调用（兼容旧的同步 ack 入口）。
 * 返回非 0 表示未发布（未连接/编码失败），drain 忽略返回值。 */
typedef int (*nb_receipt_publish_fn)(void *ctx, const char *point, uint32_t tag_id, double value);

typedef struct {
    uv_async_t async;
    uv_mutex_t lock;
    nb_receipt_publish_fn publish;
    void *publish_ctx;
    nb_receipt_entry_t entries[NB_RESULT_QUEUE_CAP];
    uint32_t head;
    uint32_t count;
    bool async_init;
    bool stopped;
} north_result_queue_t;

/* 初始化 mutex 与发布回调；调用方结构需已清零。 */
void north_result_queue_init(north_result_queue_t *q, nb_receipt_publish_fn publish, void *ctx);

/* loop 线程：初始化 async、清掉上一轮残留条目、解除 stopped。 */
void north_result_queue_attach(north_result_queue_t *q, uv_loop_t *loop);

/* 任意线程：拒绝新的投递（已在队列里的条目由 detach 丢弃）。 */
void north_result_queue_stop(north_result_queue_t *q);

/* loop 线程：关闭 async 并丢弃未 drain 的条目；重开前不得重放旧回执。 */
void north_result_queue_detach(north_result_queue_t *q);

/* 调用方保证已 stop/detach 且无生产者后再销毁。 */
void north_result_queue_destroy(north_result_queue_t *q);

/* 任意线程投递，编码/发布由 loop 做。返回 0 已入队；<0 未启动/已停止/队列满。 */
int north_result_queue_post_accepted(north_result_queue_t *q, uint32_t tag_id, const char *name,
                                     bool ok);
int north_result_queue_post_result(north_result_queue_t *q, uint32_t tag_id, const char *name,
                                   int status);

/* 同步 ack：调用方线程直接发布，不经过 loop。仅保留给无 loop 的兼容入口。 */
int north_result_queue_publish_accepted_now(north_result_queue_t *q, uint32_t tag_id,
                                            const char *name, bool ok);

#ifdef __cplusplus
}
#endif

#endif /* NORTH_RESULT_QUEUE_H */
