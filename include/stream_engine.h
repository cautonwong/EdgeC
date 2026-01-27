#ifndef STREAM_ENGINE_H
#define STREAM_ENGINE_H

#include <stdint.h>
#include <uv.h>

#include "datahub.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sqlite_svc_s sqlite_svc_t;
typedef struct js_engine_s js_engine_t;
typedef struct battery_limit_s battery_limit_t;
typedef struct edge_params_s edge_params_t;

typedef struct {
    uint64_t (*get_monotonic_ms)(void);
    uint64_t (*get_calendar_ms)(void);
} engine_time_source_t;

typedef enum { STAGE_OK = 0, STAGE_DROP = 1, STAGE_ASYNC = 2, STAGE_ERROR = -1 } stage_ret_t;

// [新增] S17: Stage 健康状态
typedef enum {
    STAGE_HEALTH_GOOD = 0,
    STAGE_HEALTH_TIMEOUT = 1,    // 耗时过长
    STAGE_HEALTH_EXCEPTION = 2,  // 逻辑报错
    STAGE_HEALTH_DISABLED = 3    // 被手动禁用
} stage_health_t;

typedef struct pipeline_stage_s pipeline_stage_t;

typedef stage_ret_t (*stage_proc_fn)(pipeline_stage_t *stage, pipe_msg_t *ev);
typedef void (*stage_cleanup_fn)(void *priv_data);

struct pipeline_stage_s {
    char name[32];
    uv_loop_t *loop;              // Loop for scheduling async work
    struct pipeline_s *pipeline;  // Back-pointer to parent
    stage_proc_fn process;
    void *priv_state;
    void *priv_cfg;
    stage_cleanup_fn cleanup_state;
    stage_cleanup_fn cleanup_cfg;

    // [新增] S17: 运行统计
    stage_health_t health;
    uint64_t last_proc_us;       /**< 上次处理耗时 (微秒) */
    uint64_t avg_proc_us;        /**< 平均处理耗时 (微秒) */
    uint64_t total_events;       /**< 处理总数 */
    uint64_t total_errors;       /**< 错误总数 */
    uint64_t circuit_breaker_us; /**< 触发熔断的阈值 (0 表示不熔断) */

    struct pipeline_stage_s *next;
};

typedef struct stream_engine_s stream_engine_t;
typedef struct pipeline_s pipeline_t;

typedef struct {
    sqlite_svc_t *db;
    js_engine_t *js_engine;
    edge_params_t *params;
    struct bus_orchestrator_s *orch; /**< [新增] 用于指令触发 (Sprint 3) */
} pipeline_loader_ctx_t;

stream_engine_t *stream_engine_create(datahub_t *hub, uv_loop_t *loop,
                                      struct bus_orchestrator_s *orch);

/**
 * @brief 在指定 Loop 上创建流引擎 (用于物理隔离 L2 线程)
 */
stream_engine_t *stream_engine_create_ex(datahub_t *hub, uv_loop_t *loop,
                                         struct bus_orchestrator_s *orch);
void stream_engine_destroy(stream_engine_t *engine);

void stream_engine_start(stream_engine_t *engine);

pipeline_t *stream_engine_add_pipeline(stream_engine_t *engine, const char *id,
                                       uint32_t trigger_tag);
pipeline_stage_t *pipeline_add_stage(pipeline_t *p, const char *name, stage_proc_fn proc,
                                     void *priv_state, stage_cleanup_fn cleanup_state,
                                     void *priv_cfg, stage_cleanup_fn cleanup_cfg);

int stream_engine_load_config(stream_engine_t *engine, const char *json_path,
                              pipeline_loader_ctx_t *ctx);

/**
 * @brief [S17] 从注入的静态元数据装配逻辑管线
 */
int stream_engine_load_static_metadata(stream_engine_t *engine, const edge_pipeline_def_t *metadata,
                                       int count, pipeline_loader_ctx_t *ctx);

void stream_engine_push(stream_engine_t *engine, pipe_msg_t *msg);
char *stream_engine_dump_json(stream_engine_t *engine);

/**
 * @brief [SE-06] Resumes a suspended pipeline.
 * This is called by an asynchronous stage's callback when its long-running task is complete.
 * @param p The pipeline to resume.
 */
void stream_engine_resume(pipeline_t *p);

/**
 * @brief 获取流引擎的异步通知句柄
 */
uv_async_t *stream_engine_get_notify_handle(stream_engine_t *engine);

/**
 * @brief [S17] 获取指定算子的健康状态
 */
stage_health_t pipeline_stage_get_health(pipeline_stage_t *stage);

// --- Standard Stage Factories ---
pipeline_stage_t *stage_filter_create(uint32_t tag_id);
pipeline_stage_t *stage_filter_range_create(double min, double max);
pipeline_stage_t *stage_transform_create(double alpha, double beta);
pipeline_stage_t *stage_ema_create(double alpha);
pipeline_stage_t *stage_clamp_create(double min, double max);
pipeline_stage_t *stage_peak_shaving_create(double threshold_w, uint32_t ctrl_tag);
pipeline_stage_t *stage_accumulate_create(void);
pipeline_stage_t *stage_periodic_freeze_create(int kind);
pipeline_stage_t *stage_time_guard_create(void);
pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table);
pipeline_stage_t *stage_js_rule_create(js_engine_t *engine, const char *script_path, bool is_async);
pipeline_stage_t *stage_wasm_create(const char *wasm_path, const char *entry_func);
pipeline_stage_t *stage_battery_guard_create(battery_limit_t *limits);
pipeline_stage_t *stage_reflex_logic_create(struct bus_orchestrator_s *orch, uint32_t bus_id,
                                            const char *proto, uint32_t target_tag,
                                            double threshold, bool greater_than, double action_val);

// --- Sprint 3: 工业级算法库扩展 ---
pipeline_stage_t *stage_moving_average_create(uint32_t window_size);
pipeline_stage_t *stage_rbe_create(double threshold_pct);

#ifdef __cplusplus
}
#endif

#endif
