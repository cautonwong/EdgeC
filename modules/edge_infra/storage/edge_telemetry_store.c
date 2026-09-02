/**
 * @file edge_telemetry_store.c
 * @brief Unified Telemetry Storage, WAL Crash-Consistency & SnF Implementation (ARCH-DEEP-03)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include "edge_telemetry_store.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"
#include "timeseries_compressor.h"

#define WAL_MAGIC 0x57414C54  // "WALT" (Write-Ahead Log Telemetry)
#define WAL_FLAG_RAW 0x00
#define WAL_FLAG_GORILLA 0x01

#define MAX_MEM_RECORDS 65536
#define DEFAULT_SNF_CAPACITY 10000
#define COMPRESS_CHUNK_SIZE 128

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t payload_len;
    uint32_t flags;
    uint32_t crc32;
} wal_record_header_t;

struct edge_telemetry_store_s {
    edge_telemetry_store_cfg_t cfg;
    uv_loop_t *loop;
    int wal_fd;
    char resolved_wal_path[512];

    // 内存环形记录缓冲 (用于快速查询与批处理)
    pipe_msg_t *mem_records;
    size_t mem_capacity;
    size_t mem_count;
    size_t mem_head;

    // 离线 SnF 队列
    pipe_msg_t *snf_queue;
    size_t snf_capacity;
    size_t snf_head;
    size_t snf_tail;
    size_t snf_count;

    // Gorilla 压缩临时缓冲
    ts_sample_t sample_chunk[COMPRESS_CHUNK_SIZE];
    size_t sample_chunk_count;

    // 统计指标
    edge_telemetry_store_stats_t stats;

    // Libuv 异步定时器与锁
    uv_mutex_t lock;
    uv_timer_t flush_timer;
    bool timer_init;
    bool lock_init;
};

// --- CRC32 IEEE 802.3 Checksum ---

static uint32_t _calc_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

// --- WAL Write & Recovery ---

static int _wal_append_msg(edge_telemetry_store_t *store, const pipe_msg_t *msg) {
    if (store->wal_fd < 0) return 0;

    wal_record_header_t hdr;
    hdr.magic = WAL_MAGIC;
    hdr.payload_len = sizeof(pipe_msg_t);
    hdr.flags = WAL_FLAG_RAW;
    hdr.crc32 = _calc_crc32((const uint8_t *)msg, sizeof(pipe_msg_t));

    ssize_t written = write(store->wal_fd, &hdr, sizeof(hdr));
    if (written != (ssize_t)sizeof(hdr)) {
        return -1;
    }
    written = write(store->wal_fd, msg, sizeof(pipe_msg_t));
    if (written != (ssize_t)sizeof(pipe_msg_t)) {
        return -1;
    }

    store->stats.wal_file_size_bytes += sizeof(hdr) + sizeof(pipe_msg_t);
    store->stats.total_wal_flushed++;
    return 0;
}

static int _wal_recover(edge_telemetry_store_t *store) {
    if (store->wal_fd < 0) return 0;

    // 移到文件起始
    if (lseek(store->wal_fd, 0, SEEK_SET) < 0) {
        return -1;
    }

    int recovered = 0;
    while (1) {
        wal_record_header_t hdr;
        ssize_t r = read(store->wal_fd, &hdr, sizeof(hdr));
        if (r == 0) break; // 文件末尾
        if (r != (ssize_t)sizeof(hdr)) {
            LOG_WARN(LOG_CAT_CORE, "[WAL-Recovery] Truncated header at offset %lu, stopping recovery",
                     (unsigned long)lseek(store->wal_fd, 0, SEEK_CUR));
            break;
        }

        if (hdr.magic != WAL_MAGIC || hdr.payload_len != sizeof(pipe_msg_t)) {
            LOG_WARN(LOG_CAT_CORE, "[WAL-Recovery] Invalid magic 0x%08X or payload_len %u, stopping recovery",
                     hdr.magic, hdr.payload_len);
            break;
        }

        pipe_msg_t msg;
        r = read(store->wal_fd, &msg, sizeof(msg));
        if (r != (ssize_t)sizeof(msg)) {
            LOG_WARN(LOG_CAT_CORE, "[WAL-Recovery] Truncated payload, stopping recovery");
            break;
        }

        uint32_t expected_crc = _calc_crc32((const uint8_t *)&msg, sizeof(msg));
        if (hdr.crc32 != expected_crc) {
            LOG_WARN(LOG_CAT_CORE, "[WAL-Recovery] CRC32 mismatch (0x%08X != 0x%08X), stopping recovery",
                     hdr.crc32, expected_crc);
            break;
        }

        // 恢复入内存记录表
        if (store->mem_count < store->mem_capacity) {
            store->mem_records[store->mem_count++] = msg;
        }
        recovered++;
    }

    // 移动到文件尾部准备继续追加
    off_t end_pos = lseek(store->wal_fd, 0, SEEK_END);
    if (end_pos > 0) {
        store->stats.wal_file_size_bytes = (uint64_t)end_pos;
    }
    store->stats.total_wal_flushed = (uint64_t)recovered;
    LOG_INFO(LOG_CAT_CORE, "[WAL-Recovery] Successfully recovered %d records from %s",
             recovered, store->resolved_wal_path);
    return recovered;
}

// --- Gorilla Flush Helper ---

static void _flush_gorilla_chunk_locked(edge_telemetry_store_t *store) {
    if (!store->cfg.enable_gorilla_compression || store->sample_chunk_count == 0) return;

    uint8_t comp_buf[4096];
    size_t comp_len = 0;
    int rc = ts_compress_batch(store->sample_chunk, store->sample_chunk_count,
                               comp_buf, sizeof(comp_buf), &comp_len);
    if (rc == 0) {
        store->stats.compression_bytes_in += store->sample_chunk_count * sizeof(ts_sample_t);
        store->stats.compression_bytes_out += comp_len;
    }
    store->sample_chunk_count = 0;
}

// --- SnF Queue Drain Helper ---

static void _drain_snf_queue_locked(edge_telemetry_store_t *store) {
    if (!store->stats.is_online || !store->cfg.forward_cb || store->snf_count == 0) return;

    pipe_msg_t batch[64];
    while (store->snf_count > 0) {
        size_t take = store->snf_count > 64 ? 64 : store->snf_count;
        for (size_t i = 0; i < take; i++) {
            batch[i] = store->snf_queue[store->snf_head];
            store->snf_head = (store->snf_head + 1) % store->snf_capacity;
            store->snf_count--;
        }

        int rc = store->cfg.forward_cb(batch, take, store->cfg.forward_ctx);
        if (rc == 0) {
            store->stats.total_snf_forwarded += take;
            store->stats.current_snf_pending = store->snf_count;
        } else {
            // 发送失败，推回头部并停止本轮 drain
            for (size_t i = 0; i < take; i++) {
                store->snf_head = (store->snf_head + store->snf_capacity - 1) % store->snf_capacity;
                store->snf_queue[store->snf_head] = batch[take - 1 - i];
                store->snf_count++;
            }
            store->stats.current_snf_pending = store->snf_count;
            break;
        }
    }
}

static void _flush_timer_cb(uv_timer_t *handle) {
    edge_telemetry_store_t *store = (edge_telemetry_store_t *)handle->data;
    if (!store) return;

    uv_mutex_lock(&store->lock);
    _flush_gorilla_chunk_locked(store);
    _drain_snf_queue_locked(store);
    if (store->wal_fd >= 0) {
        fdatasync(store->wal_fd);
    }
    uv_mutex_unlock(&store->lock);
}

// --- Public API Implementation ---

edge_telemetry_store_t *edge_telemetry_store_create(uv_loop_t *loop,
                                                    const edge_telemetry_store_cfg_t *cfg) {
    edge_telemetry_store_t *store = calloc(1, sizeof(edge_telemetry_store_t));
    if (!store) return NULL;

    if (cfg) {
        store->cfg = *cfg;
    }

    if (store->cfg.batch_size == 0) store->cfg.batch_size = 500;
    if (store->cfg.flush_interval_ms == 0) store->cfg.flush_interval_ms = 1000;
    if (store->cfg.snf_max_offline_records == 0) store->cfg.snf_max_offline_records = DEFAULT_SNF_CAPACITY;

    store->loop = loop;
    store->wal_fd = -1;
    store->stats.is_online = true; // 默认在线

    uv_mutex_init(&store->lock);
    store->lock_init = true;

    // 分配内存历史记录环
    store->mem_capacity = MAX_MEM_RECORDS;
    store->mem_records = calloc(store->mem_capacity, sizeof(pipe_msg_t));

    // 分配离线 SnF 队列
    store->snf_capacity = store->cfg.snf_max_offline_records;
    store->snf_queue = calloc(store->snf_capacity, sizeof(pipe_msg_t));

    // 配置与打开 WAL
    if (store->cfg.wal_path && strlen(store->cfg.wal_path) > 0) {
        strncpy(store->resolved_wal_path, store->cfg.wal_path, sizeof(store->resolved_wal_path) - 1);
    } else if (store->cfg.db_path && strlen(store->cfg.db_path) > 0) {
        snprintf(store->resolved_wal_path, sizeof(store->resolved_wal_path), "%s.wal", store->cfg.db_path);
    } else {
        snprintf(store->resolved_wal_path, sizeof(store->resolved_wal_path), "/tmp/edge_telemetry_default.wal");
    }

    store->wal_fd = open(store->resolved_wal_path, O_RDWR | O_CREAT, 0644);
    if (store->wal_fd >= 0) {
        _wal_recover(store);
    } else {
        LOG_WARN(LOG_CAT_CORE, "[TelemetryStore] Could not open WAL file %s (errno: %d)",
                 store->resolved_wal_path, errno);
    }

    // 启动刷盘定时器
    if (store->loop) {
        uv_timer_init(store->loop, &store->flush_timer);
        store->flush_timer.data = store;
        uv_timer_start(&store->flush_timer, _flush_timer_cb,
                       store->cfg.flush_interval_ms, store->cfg.flush_interval_ms);
        store->timer_init = true;
    }

    LOG_INFO(LOG_CAT_CORE, "[TelemetryStore] Unified Telemetry Store Created (WAL: %s, Gorilla: %d, SnF Cap: %zu)",
             store->resolved_wal_path, store->cfg.enable_gorilla_compression, store->snf_capacity);
    return store;
}

int edge_telemetry_store_append(edge_telemetry_store_t *store, const pipe_msg_t *msg) {
    if (!store || !msg) return -1;

    uv_mutex_lock(&store->lock);

    // 1. 写入 WAL
    _wal_append_msg(store, msg);

    // 2. 存入内存历史环
    if (store->mem_records) {
        store->mem_records[store->mem_head] = *msg;
        store->mem_head = (store->mem_head + 1) % store->mem_capacity;
        if (store->mem_count < store->mem_capacity) {
            store->mem_count++;
        }
    }

    // 3. Gorilla 压缩缓冲
    if (store->cfg.enable_gorilla_compression && msg->type == PIPEMSG_TYPE_SCALAR) {
        store->sample_chunk[store->sample_chunk_count].timestamp_ms = msg->ts;
        store->sample_chunk[store->sample_chunk_count].value = (double)msg->payload.val.f;
        store->sample_chunk_count++;
        if (store->sample_chunk_count >= COMPRESS_CHUNK_SIZE) {
            _flush_gorilla_chunk_locked(store);
        }
    }

    // 4. SnF 转发处理
    if (store->stats.is_online && store->cfg.forward_cb && store->snf_count == 0) {
        // 在线且无堆积，直接透传
        int rc = store->cfg.forward_cb(msg, 1, store->cfg.forward_ctx);
        if (rc == 0) {
            store->stats.total_snf_forwarded++;
        } else {
            // 发送失败，降级入 SnF 队列
            if (store->snf_count < store->snf_capacity) {
                store->snf_queue[store->snf_tail] = *msg;
                store->snf_tail = (store->snf_tail + 1) % store->snf_capacity;
                store->snf_count++;
                store->stats.current_snf_pending = store->snf_count;
            }
        }
    } else {
        // 离线或有历史堆积，入 SnF 队列
        if (store->snf_count < store->snf_capacity) {
            store->snf_queue[store->snf_tail] = *msg;
            store->snf_tail = (store->snf_tail + 1) % store->snf_capacity;
            store->snf_count++;
            store->stats.current_snf_pending = store->snf_count;
        }
    }

    store->stats.total_appended++;
    uv_mutex_unlock(&store->lock);
    return 0;
}

int edge_telemetry_store_append_batch(edge_telemetry_store_t *store,
                                      const pipe_msg_t *msgs, size_t count) {
    if (!store || !msgs || count == 0) return -1;
    for (size_t i = 0; i < count; i++) {
        edge_telemetry_store_append(store, &msgs[i]);
    }
    return 0;
}

int edge_telemetry_store_flush_sync(edge_telemetry_store_t *store) {
    if (!store) return -1;
    uv_mutex_lock(&store->lock);
    _flush_gorilla_chunk_locked(store);
    _drain_snf_queue_locked(store);
    if (store->wal_fd >= 0) {
        fdatasync(store->wal_fd);
    }
    uv_mutex_unlock(&store->lock);
    return 0;
}

int edge_telemetry_store_query(edge_telemetry_store_t *store, uint32_t tag_id,
                               uint64_t start_ts, uint64_t end_ts,
                               telemetry_query_cb cb, void *user_ctx) {
    if (!store || !cb) return -1;

    uv_mutex_lock(&store->lock);
    if (!store->mem_records || store->mem_count == 0) {
        uv_mutex_unlock(&store->lock);
        return 0;
    }

    int match_count = 0;
    for (size_t i = 0; i < store->mem_count; i++) {
        const pipe_msg_t *msg = &store->mem_records[i];
        if (tag_id == 0 || msg->tag_id == tag_id) {
            if (msg->ts >= start_ts && msg->ts <= end_ts) {
                match_count++;
                int rc = cb(user_ctx, msg);
                if (rc != 0) break;
            }
        }
    }

    uv_mutex_unlock(&store->lock);
    return match_count;
}

void edge_telemetry_store_set_online(edge_telemetry_store_t *store, bool is_online) {
    if (!store) return;
    uv_mutex_lock(&store->lock);
    store->stats.is_online = is_online;
    if (is_online) {
        _drain_snf_queue_locked(store);
    }
    uv_mutex_unlock(&store->lock);
}

int edge_telemetry_store_get_stats(edge_telemetry_store_t *store, edge_telemetry_store_stats_t *stats) {
    if (!store || !stats) return -1;
    uv_mutex_lock(&store->lock);
    *stats = store->stats;
    uv_mutex_unlock(&store->lock);
    return 0;
}

void edge_telemetry_store_destroy(edge_telemetry_store_t *store) {
    if (!store) return;

    if (store->timer_init && !uv_is_closing((uv_handle_t *)&store->flush_timer)) {
        uv_timer_stop(&store->flush_timer);
        uv_close((uv_handle_t *)&store->flush_timer, NULL);
        store->timer_init = false;
    }

    edge_telemetry_store_flush_sync(store);

    if (store->wal_fd >= 0) {
        close(store->wal_fd);
        store->wal_fd = -1;
    }

    if (store->mem_records) {
        free(store->mem_records);
        store->mem_records = NULL;
    }

    if (store->snf_queue) {
        free(store->snf_queue);
        store->snf_queue = NULL;
    }

    if (store->lock_init) {
        uv_mutex_destroy(&store->lock);
        store->lock_init = false;
    }

    free(store);
    LOG_INFO(LOG_CAT_CORE, "[TelemetryStore] Unified Telemetry Store Destroyed");
}
