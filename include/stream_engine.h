#ifndef STREAM_ENGINE_H
#define STREAM_ENGINE_H

#include <stdint.h>
#include <uv.h>

#include "datahub.h"
#include "edge_event.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for external components
typedef struct sqlite_svc_s sqlite_svc_t;
typedef struct js_engine_s js_engine_t;
typedef struct battery_limit_s battery_limit_t;
typedef struct edge_params_s edge_params_t;

// 时间源接口 (用于测试 Mock)
typedef struct {
    uint64_t (*get_monotonic_ms)(void);
    uint64_t (*get_calendar_ms)(void);
} engine_time_source_t;

// Pipeline 处理结果枚举
typedef enum {
    STAGE_OK = 0,     // 继续执行下一个 Stage
    STAGE_DROP = 1,   // 丢弃数据，终止 Pipeline
    STAGE_ASYNC = 2,  // 异步处理中，挂起 Pipeline
    STAGE_ERROR = -1  // 处理出错
} stage_ret_t;

// Forward declaration
typedef struct pipeline_stage_s pipeline_stage_t;

// Stage 处理函数原型
typedef stage_ret_t (*stage_proc_fn)(pipeline_stage_t *stage, pipe_msg_t *ev);

// Stage 清理函数原型
typedef void (*stage_cleanup_fn)(void *priv_data);

// Stage 结构体
struct pipeline_stage_s {
    char name[32];
    stage_proc_fn process;
    void *priv_state;                // 算子私有状态 (如累加值)
    void *priv_cfg;                  // 算子私有配置 (如阈值)
    stage_cleanup_fn cleanup_state;  // 状态清理回调
    stage_cleanup_fn cleanup_cfg;    // 配置清理回调
    struct pipeline_stage_s *next;
};

// Stream Engine 上下文
typedef struct stream_engine_s stream_engine_t;
typedef struct pipeline_s pipeline_t;  // [新增] 暴露 Pipeline 类型

// Pipeline 加载上下文 (用于注入依赖)
typedef struct {
    sqlite_svc_t *db;
    js_engine_t *js_engine;
    edge_params_t *params;
} pipeline_loader_ctx_t;

/**
 * @brief 创建流处理引擎
 */
stream_engine_t *stream_engine_create(datahub_t *hub, engine_time_source_t *time_src);

/**
 * @brief 销毁流处理引擎
 */
void stream_engine_destroy(stream_engine_t *engine);

// API: 动态添加 Pipeline (供 loader 使用)
pipeline_t *stream_engine_add_pipeline(stream_engine_t *engine, const char *id,
                                       uint32_t trigger_tag);
void pipeline_add_stage(pipeline_t *p, const char *name, stage_proc_fn proc, void *priv_state,
                        stage_cleanup_fn cleanup_state, void *priv_cfg,
                        stage_cleanup_fn cleanup_cfg);

/**
 * @brief 从 JSON 配置文件加载 Pipelines
 * @param engine 引擎实例
 * @param json_path 配置文件路径
 * @param ctx 加载上下文 (DB/JS/Params 句柄)
 * @return 0 成功
 */
int stream_engine_load_config(stream_engine_t *engine, const char *json_path,
                              pipeline_loader_ctx_t *ctx);

/**
 * @brief 导出当前 Pipeline 状态为 JSON (用于 Web 展示)
 */
char *stream_engine_dump_json(stream_engine_t *engine);

/**
 * @brief Standard Stage Library Factories
 */
pipeline_stage_t *stage_filter_create(uint32_t tag_id);
pipeline_stage_t *stage_filter_range_create(double min, double max);
pipeline_stage_t *stage_transform_create(double alpha, double beta);
pipeline_stage_t *stage_accumulate_create(void);
pipeline_stage_t *stage_periodic_freeze_create(int kind);
pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table);
pipeline_stage_t *stage_js_rule_create(js_engine_t *engine, const char *script);
pipeline_stage_t *stage_battery_guard_create(battery_limit_t *limits);

#ifdef __cplusplus
}
#endif

#endif  // STREAM_ENGINE_H