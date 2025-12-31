#ifndef EDGE_STREAM_ENGINE_H
#define EDGE_STREAM_ENGINE_H

#include <uv.h>

#include "datahub.h"
#include "edge_common.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stream_engine_s stream_engine_t;
typedef struct pipeline_s pipeline_t;
typedef struct pipeline_stage_s pipeline_stage_t;

// Forward declarations for external components
typedef struct sqlite_svc_s sqlite_svc_t;
typedef struct js_engine_s js_engine_t;
typedef struct battery_limit_s battery_limit_t;

// --- 时间源抽象：用于 cmocka 打桩测试抗跳变逻辑 ---
typedef struct {
    uint64_t (*get_monotonic_ms)(void);
    uint64_t (*get_calendar_ms)(void);
} engine_time_source_t;

typedef enum { EVT_DATA_POINT = 1, EVT_TIME_PULSE = 2, EVT_SYS_CONFIG = 3 } pipe_evt_type_t;

typedef struct {
    pipe_evt_type_t type;
    uint64_t monotonic_ms;
    uint64_t calendar_ms;
    uint32_t tag_id;
    union {
        double val;
        int tick_kind;
    } payload;
} pipe_event_t;

typedef enum { STAGE_OK, STAGE_DROP, STAGE_ASYNC, STAGE_ERROR } stage_ret_t;

typedef stage_ret_t (*stage_proc_fn)(pipeline_stage_t *stage, pipe_event_t *ev);

struct pipeline_stage_s {
    char name[32];
    stage_proc_fn process;
    void *priv_state;
    void *priv_cfg;
    void (*cleanup_state)(void *state);
    void (*cleanup_cfg)(void *cfg);
    struct pipeline_stage_s *next;
};

/**
 * @brief 创建流引擎
 * @param time_src 传入时间源。生产环境传 NULL 使用默认 libuv/RTC 方案，测试环境传 Mock 指针。
 */
stream_engine_t *stream_engine_create(datahub_t *hub, engine_time_source_t *time_src);

pipeline_t *stream_engine_add_pipeline(stream_engine_t *engine, const char *id,
                                       uint32_t trigger_tag);
void pipeline_add_stage(pipeline_t *p, const char *name, stage_proc_fn proc, void *init_state,
                        void (*cleanup_state)(void *), void *init_cfg, void (*cleanup_cfg)(void *));
void pipeline_resume(pipeline_t *p);

int stream_engine_start(stream_engine_t *engine);
int stream_engine_push_event(stream_engine_t *engine, pipe_event_t *ev);
/**
 * @brief Context for loading pipelines, providing necessary service handles.
 */
typedef struct {
    sqlite_svc_t *db;
    js_engine_t *js_engine;
} pipeline_loader_ctx_t;

/**
 * @brief Load pipeline configurations from a JSON file.
 */
int stream_engine_load_config(stream_engine_t *engine, const char *json_path,
                              pipeline_loader_ctx_t *ctx);

/**
 * @brief Clear all existing pipelines (for hot reload)
 */
void stream_engine_clear_pipelines(stream_engine_t *engine);

/**
 * @brief Dump current pipelines status to JSON string.
 * Caller must free the returned string.
 */
char *stream_engine_dump_json(stream_engine_t *engine);

/**
 * @brief 将当前引擎的所有 Pipeline 拓扑导出为 JSON 字符串
 */
char *stream_engine_dump_json(stream_engine_t *engine);

void stream_engine_destroy(stream_engine_t *engine);

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

#endif  // EDGE_STREAM_ENGINE_H
