#ifndef STREAM_ENGINE_H
#define STREAM_ENGINE_H

#include <stdint.h>
#include <uv.h>

#include "contract_data_types.h"
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

typedef enum { 
    STAGE_OK = 0,      /**< 继续执行主分支 (next[0]) */
    STAGE_DROP = 1,    /**< 丢弃数据，终止当前流 */
    STAGE_ASYNC = 2,   /**< 异步挂起，等待 resume */
    STAGE_ERROR = -1,  /**< 逻辑错误，终止当前流 */
    
    // --- [Topology] 分支控制 ---
    STAGE_BRANCH_0 = 10, /**< 执行分支 0 */
    STAGE_BRANCH_1 = 11, /**< 执行分支 1 */
    STAGE_BRANCH_2 = 12, /**< 执行分支 2 */
    STAGE_BRANCH_3 = 13  /**< 执行分支 3 */
} stage_ret_t;

#define STAGE_NEXT(n) (STAGE_BRANCH_0 + (n))

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

/**
 * @brief [Checkpointing] 状态序列化回调
 * @param buf 输出缓冲区
 * @param max_len 缓冲区大小
 * @return 实际写入长度，失败返回 -1
 */
typedef int (*stage_save_state_fn)(pipeline_stage_t *stage, void *buf, size_t max_len);

/**
 * @brief [Checkpointing] 状态恢复回调
 */
typedef int (*stage_load_state_fn)(pipeline_stage_t *stage, const void *buf, size_t len);

struct pipeline_stage_s {
    char name[32];
    uv_loop_t *loop;              // Loop for scheduling async work
    struct pipeline_s *pipeline;  // Back-pointer to parent
    stage_proc_fn process;
    void *priv_state;
    void *priv_cfg;
    stage_cleanup_fn cleanup_state;
    stage_cleanup_fn cleanup_cfg;

    // --- [Checkpointing] 状态持久化接口 ---
    stage_save_state_fn save_state;
    stage_load_state_fn load_state;

    // [新增] S17: 运行统计
    stage_health_t health;
    uint64_t last_proc_us;       /**< 上次处理耗时 (微秒) */
    uint64_t avg_proc_us;        /**< 平均处理耗时 (微秒) */
    uint64_t total_events;       /**< 处理总数 */
    uint64_t total_errors;       /**< 错误总数 */
    uint64_t circuit_breaker_us; /**< 触发熔断的阈值 (0 表示不熔断) */

    // --- [Topology] 拓扑连接 ---
    struct pipeline_stage_s *next[4]; /**< 支持最多 4 路分支输出 (next[0] 为默认主路) */
};

typedef struct stream_engine_s stream_engine_t;
typedef struct pipeline_s pipeline_t;

typedef struct dag_node_s {
    pipeline_stage_t *stage;
    atomic_int pending_inputs;
    uint32_t in_degree;
    uint32_t child_count;
    struct dag_node_s *children[8];
} dag_node_t;

typedef struct {
    uint32_t node_count;
    dag_node_t nodes[16];
    bool is_acyclic;
} dag_graph_t;

int dag_validate_acyclic(dag_graph_t *graph);
int dag_execute_parallel(stream_engine_t *engine, dag_graph_t *graph, pipe_msg_t *msg);

// [Consumer-Defined Control Interface] 解耦底层总线模块
typedef int (*stream_control_cb)(void *ctx, int bus_id, uint32_t tag_id, double value, const char *proto);
typedef struct {
    void *ctx;
    stream_control_cb submit_control;
} stream_control_ops_t;

// pipeline_loader_ctx_t 的定义必须匹配 contract_data_types.h 中的前置声明
struct pipeline_loader_ctx_s {
    sqlite_svc_t *db;
    js_engine_t *js_engine;
    edge_params_t *params;
    stream_control_ops_t control_ops; /**< [IoC] 用于指令触发，消费者定义的接口 */
};

stream_engine_t *stream_engine_create(datahub_t *hub, uv_loop_t *loop,
                                      void *orch,
                                      const edge_stage_factory_entry_t *registry);

/**
 * @brief 在指定 Loop 上创建流引擎 (用于物理隔离 L2 线程)
 */
stream_engine_t *stream_engine_create_ex(datahub_t *hub, uv_loop_t *loop,
                                         void *orch,
                                         const edge_stage_factory_entry_t *registry);
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
 * @brief 算子工厂函数原型 (支持根据节点参数动态构造算子)
 */
typedef pipeline_stage_t *(*stage_factory_create_fn)(const char *name,
                                                      void *config_json,
                                                      pipeline_loader_ctx_t *ctx);

/**
 * @brief 注册自定义算子工厂 (支持二开产品插件化算子扩展)
 */
int stream_engine_register_stage_factory(const char *type_name, stage_factory_create_fn factory_fn);

/**
 * @brief 从声明式 YAML / JSON 文本直接装配流计算 DAG (包含无环拓扑检查与自动连线)
 */
int stream_engine_load_yaml_contract(stream_engine_t *engine,
                                     const char *yaml_content,
                                     pipeline_loader_ctx_t *ctx);

/**
 * @brief 从声明式 YAML / JSON 文件路径直接装配流计算 DAG
 */
int stream_engine_load_yaml_file(stream_engine_t *engine,
                                 const char *yaml_path,
                                 pipeline_loader_ctx_t *ctx);

/**
 * @brief 导出流处理引擎当前运行拓扑的 Graphviz DOT 结构 (用于前端可视化与拓扑审计)
 * @return 动态分配的 DOT 字符串 (调用方需 free)
 */
char *stream_engine_export_dot(stream_engine_t *engine);

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
pipeline_stage_t *stage_reflex_logic_create(stream_control_ops_t control_ops, uint32_t bus_id,
                                            const char *proto, uint32_t target_tag,
                                            double threshold, bool greater_than, double action_val);

// --- Sprint 3: 工业级算法库扩展 ---
pipeline_stage_t *stage_moving_average_create(uint32_t window_size);
pipeline_stage_t *stage_rbe_create(double threshold_pct);

#ifdef __cplusplus
}
#endif

#endif
