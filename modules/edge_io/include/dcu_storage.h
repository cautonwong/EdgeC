#ifndef DCU_STORAGE_H
#define DCU_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <sqlite3.h>
#include <pthread.h>
#include <uv.h>

#define STORAGE_CACHE_MAX 128
#define STORAGE_ADDR_LEN 32

/**
 * @brief 实时最新数据缓存记录
 */
typedef struct {
    char meter_address[STORAGE_ADDR_LEN];
    char oad[16];
    double value;
    uint64_t ts;
} telemetry_cache_entry_t;

/**
 * @brief 电表历史断面冻结记录结构体 (供北向查询返回)
 */
typedef struct {
    char meter_address[STORAGE_ADDR_LEN];
    char freeze_oad[16];          // 冻结 OAD，如 "50040200" (日冻结)
    uint64_t freeze_time;         // 断面时间戳 (对应 OAD 20210200)
    char source_oad[16];          // 数据源 OAD，如 "00100200" (累计电能)
    double frozen_value;          // 快照值
} dcu_freeze_record_t;

/**
 * @brief 集中器生产级 SQLite 存储服务主体 (事务批量合并与 Flash 寿命保护)
 */
typedef struct {
    sqlite3 *db;                  // SQLite 3 数据库句柄
    uv_loop_t *loop;              // libuv 事件循环
    
    // 双缓冲队列与写入互斥锁
    telemetry_cache_entry_t cache[STORAGE_CACHE_MAX];
    uint16_t cache_count;
    pthread_mutex_t lock;         // 线程安全锁
    
    uv_timer_t flush_timer;       // 10s 闲时合并批量写入刷盘定时器
    bool timer_active;
} dcu_storage_t;

/* --- 生产级 API 函数原型声明 --- */

/**
 * @brief 初始化 SQLite 存储服务，加载建表 SQL 并开启 WAL 寿命优化模式
 */
int dcu_storage_init(dcu_storage_t *storage, uv_loop_t *loop, const char *db_path);

/**
 * @brief 写入实时遥测数据进入内存缓冲队列 (双缓冲 Flash 保护)
 */
int dcu_storage_write_telemetry(dcu_storage_t *storage, const char *addr, const char *oad, double value);

/**
 * @brief 直接写入历史断面日冻结/月冻结快照数据 (Class 9)
 */
int dcu_storage_write_freeze(dcu_storage_t *storage, const char *addr, const char *freeze_oad, uint64_t freeze_time, const char *src_oad, double value);

/**
 * @brief 强制强行刷盘：将内存缓冲区的所有数据在单个批量事务内落盘
 */
int dcu_storage_flush(dcu_storage_t *storage);

/**
 * @brief 核心物理查询：根据 RSD 时间区间与指定的 OAD，利用复合索引快速检索历史冻结断面 (Class 9)
 */
int dcu_storage_query_freeze(dcu_storage_t *storage, const char *addr, const char *freeze_oad, uint64_t start_time, uint64_t end_time, dcu_freeze_record_t *out_records, uint16_t max_records, uint16_t *out_count);

/**
 * @brief 销毁存储服务，干净释放互斥锁并关闭 SQLite 指针
 */
void dcu_storage_destroy(dcu_storage_t *storage);

#endif // DCU_STORAGE_H
