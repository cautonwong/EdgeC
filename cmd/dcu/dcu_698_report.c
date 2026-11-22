#include "dcu_698_report.h"

#include <string.h>

#include "dcu_698_var.h"
#include "dlt698_codec.h"
#include "uv.h"

/* 一次上报最多带几个对象属性：与 GET-Response-NormalList 的上限一致。 */
#define DCU_698_REPORT_BATCH DLT698_GET_LIST_MAX
#define DCU_698_REPORT_QUEUE 32
/* 单个标量条目的 A-XDR 缓冲：标签 + 最多 8 字节值。 */
#define DCU_698_REPORT_AXDR 16

typedef struct {
    uint32_t oad;
    uint8_t type;
    int64_t raw;
} dcu_report_item_t;

static dcu_report_item_t s_queue[DCU_698_REPORT_QUEUE];
static size_t s_head;
static size_t s_tail;
static uint8_t s_piid;

/* 事件入库与出队分属两个线程（stream engine / IO loop），队列要加锁。
 * uv_once 让测试直接调用 enqueue/next 时也能安全初始化。 */
static uv_mutex_t s_lock;
static uv_once_t s_once = UV_ONCE_INIT;

static void _init_lock(void) {
    uv_mutex_init(&s_lock);
}

static void _lock(void) {
    uv_once(&s_once, _init_lock);
    uv_mutex_lock(&s_lock);
}

static void _unlock(void) {
    uv_mutex_unlock(&s_lock);
}

void dcu_698_report_enqueue(uint32_t oad, uint8_t type_tag, int64_t raw) {
    _lock();
    size_t next = (s_head + 1) % DCU_698_REPORT_QUEUE;
    if (next == s_tail) {
        /* 满了就丢最旧的，保留最新事件；上报是尽力而为，不阻塞采集线程。 */
        s_tail = (s_tail + 1) % DCU_698_REPORT_QUEUE;
    }
    s_queue[s_head] = (dcu_report_item_t){.oad = oad, .type = type_tag, .raw = raw};
    s_head = next;
    _unlock();
}

void dcu_698_report_reset(void) {
    _lock();
    s_head = 0;
    s_tail = 0;
    s_piid = 0;
    _unlock();
}

/* 宽度未知（没有编码器）就丢弃这一条，不猜字节序。 */
static int _encode_scalar(uint8_t type, int64_t raw, uint8_t *out, size_t cap, size_t *len) {
    uint8_t width = dcu_698_tag_width(type);
    if (width == 0 || (size_t)width + 1 > cap) return -1;
    out[0] = type;
    uint64_t u = (uint64_t)raw;
    for (uint8_t i = 0; i < width; i++) {
        out[1 + i] = (uint8_t)(u >> (8 * (width - 1 - i)));
    }
    *len = (size_t)width + 1;
    return 0;
}

int dcu_698_report_next(void *ctx, uint8_t *body, size_t cap, size_t *len) {
    (void)ctx;
    if (!body || !len) return -1;
    *len = 0;

    dcu_report_item_t items[DCU_698_REPORT_BATCH];
    size_t n = 0;
    uint8_t piid = 0;
    _lock();
    while (n < DCU_698_REPORT_BATCH && s_tail != s_head) {
        items[n++] = s_queue[s_tail];
        s_tail = (s_tail + 1) % DCU_698_REPORT_QUEUE;
    }
    if (n > 0) piid = ++s_piid;
    _unlock();
    if (n == 0) return 0;

    uint8_t axdr[DCU_698_REPORT_BATCH][DCU_698_REPORT_AXDR];
    dlt698_get_item_t results[DCU_698_REPORT_BATCH];
    size_t ok = 0;
    for (size_t i = 0; i < n; i++) {
        size_t alen = 0;
        if (_encode_scalar(items[i].type, items[i].raw, axdr[ok], sizeof(axdr[ok]), &alen) != 0) {
            continue;
        }
        results[ok] = (dlt698_get_item_t){
            .oad = items[i].oad, .dar = 0, .data = axdr[ok], .dlen = alen};
        ok++;
    }
    if (ok == 0) return 0;

    if (dlt698_encode_report_notification(body, cap, DLT698_REPORT_LIST, piid, results, ok, len) !=
        0) {
        return -1;
    }
    return 0;
}
