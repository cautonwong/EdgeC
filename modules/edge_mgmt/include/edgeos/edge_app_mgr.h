#ifndef EDGE_APP_MGR_H
#define EDGE_APP_MGR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "edgeos/edge_authz.h"
typedef struct uv_loop_s uv_loop_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_manifest_s app_manifest_t;

/**
 * @brief [DDD / Clean Architecture] 应用元数据仓储抽象契约 (Repository Pattern)
 */
typedef struct edge_app_repo_s {
    void *ctx;
    int (*save_app)(void *ctx, const app_manifest_t *manifest);
    int (*get_app)(void *ctx, const char *app_id, app_manifest_t *out_manifest);
    int (*delete_app)(void *ctx, const char *app_id);
    int (*list_apps)(void *ctx, app_manifest_t *out_apps, int max_count);
} edge_app_repo_t;

/**
 * @brief WASM 应用运行状态
 */
typedef enum {
    APP_STATE_UNINSTALLED = 0,
    APP_STATE_INSTALLED   = 1,
    APP_STATE_RUNNING     = 2,
    APP_STATE_STOPPED     = 3,
    APP_STATE_FAULTED     = 4,
} app_state_t;

/**
 * @brief 卸载数据处置策略。零值即 KEEP，未指定时默认保留数据。
 */
typedef enum {
    APP_UNINSTALL_KEEP   = 0, /**< 删代码包，注册表改只读墓碑，私有数据与历史映射保留 */
    APP_UNINSTALL_PURGE  = 1, /**< 删除私有数据；注册表留墓碑，历史默认保留 */
    APP_UNINSTALL_EXPORT = 2, /**< 先导出再删除；归档由组合根写出 */
} app_uninstall_disposition_t;

/**
 * @brief 卸载选项；全零结构体等同于 KEEP。
 */
typedef struct {
    app_uninstall_disposition_t disposition;
    bool purge_history;      /**< PURGE 时是否连历史一起删；默认 false */
    char export_path[256];   /**< EXPORT 目标路径（P0 预留） */
    char actor[64];          /**< 审计操作者，可为空串 */
} app_uninstall_opts_t;

/**
 * @brief WASM 应用故障恢复策略
 */
typedef enum {
    FAULT_POLICY_AUTO_RESTART      = 0,  /**< 默认：自动重载重启 */
    FAULT_POLICY_ISOLATE_AND_ALARM = 1,  /**< 隔离熔断并发送系统告警 */
} fault_policy_t;

/**
 * @brief WASM 应用描述符结构体 (App Manifest)
 */
typedef struct app_manifest_s {
    char id[64];              /**< 应用唯一标识 (e.g. "voltage_guard") */
    char name[128];           /**< 应用显示名称 */
    char version[32];         /**< 语义化版本 (e.g. "1.0.0") */
    char author[64];          /**< 开发者或组织 */
    char wasm_path[256];      /**< 磁盘物理 .wasm 文件存储路径 */
    uint32_t trigger_tag;     /**< 绑定的 DataHub 触发点位 ID */
    char entry_func[64];      /**< 导出的算法入口函数名 (e.g. "on_event") */
    fault_policy_t fault_policy; /**< 故障处理策略 */
    uint32_t heap_limit_kb;   /**< 堆内存限制 (KB) */
    uint32_t timeout_ms;      /**< 单次演算看门狗超时 (ms) */
    app_state_t state;        /**< 当前运行状态 */
    uint32_t restart_count;   /**< current fault-streak length inside the window */
    uint64_t last_fault_ms;   /**< epoch ms of the last fault; 0 when none yet */
    uint64_t created_at;      /**< 安装创建时间戳 */
    uint32_t priority;        /**< resources.priority 0..10; higher wins */
    bool keep_alive;          /**< core app; never pressure-reclaimed */
    bool desired_state;       /**< user intent: true = should be running */
    uint64_t last_active_ms;  /**< monotonic ms of the last start/activity */
    /* 上一次升级替换掉的版本快照，rollback 只能回退一层。 */
    char prev_version[32];         /**< 被替换的版本号；空串表示无可回退版本 */
    char prev_manifest_json[4096]; /**< 被替换的注册表行 JSON，字段与结构体同名 */
    char prev_wasm_path[256];      /**< 被替换的包路径；仅包被真正替换时非空 */
} app_manifest_t;

typedef struct edge_app_mgr_s edge_app_mgr_t;

/** 异步任务槽已被占用；调用方应稍后重试，不排队。 */
#define EDGE_APP_MGR_BUSY (-2)

/** 该处置策略尚未实现；调用方不应把它当作成功。 */
#define EDGE_APP_MGR_UNSUPPORTED (-3)

/** Faults closer together than this form one streak; the streak sizes the backoff. */
#define EDGE_APP_FAULT_WINDOW_MS 600000ULL

/** Override the fault window, for tests and diagnostics. */
void edge_app_mgr_set_fault_window_ms(edge_app_mgr_t *mgr, uint64_t window_ms);

/** 应用安装根目录；由组合根从环境或产品配置注入。 */
void edge_app_mgr_set_apps_dir(edge_app_mgr_t *mgr, const char *dir);

/** 安装崩溃注入点，仅测试使用。 */
void edge_app_mgr_set_crash_after_package_write(edge_app_mgr_t *mgr, bool enabled);

/**
 * @brief 异步任务完成回调，在 loop 线程调用。
 * @param rc 0 成功，其他失败
 * @param app_id 任务目标应用
 * @param reason 失败原因；成功时为空串
 * @param ctx 提交任务时传入的上下文
 */
typedef void (*edge_app_job_cb_t)(int rc, const char *app_id, const char *reason, void *ctx);

/**
 * @brief 应用管理器自有日志端口；未注入时保持静默。
 */
typedef struct edge_app_logger_s {
    void (*log)(void *ctx, int level, const char *message);
    void *ctx;
} edge_app_logger_t;

/**
 * @brief 注入日志端口 (可选；组合根接 edge_log 或控制台)
 */
void edge_app_mgr_set_logger(edge_app_mgr_t *mgr, const edge_app_logger_t *logger);

/**
 * @brief 基于抽象仓储创建并初始化应用管理器 (IoC / Clean Architecture)
 */
size_t edge_app_mgr_storage_size(void);

edge_app_mgr_t *edge_app_mgr_create_with_repo_in(void *storage, size_t storage_size,
                                                 uv_loop_t *loop,
                                                 const edge_app_repo_t *repo,
                                                 void *stream_engine);

/**
 * @brief 读取管理器持有的仓储端口 (列表/持久化调用方使用)
 */
const edge_app_repo_t *edge_app_mgr_get_repo(const edge_app_mgr_t *mgr);

/**
 * @brief 销毁应用管理器
 *
 * 调用方必须保证没有进行中的异步任务；单槽任务持有本对象指针，
 * 销毁前把 loop 跑到 edge_app_mgr_is_busy() 为 false。
 */
void edge_app_mgr_destroy(edge_app_mgr_t *mgr);

/**
 * @brief 安装/部署新应用 (自动解析或默认配置)
 */
int edge_app_mgr_install(edge_app_mgr_t *mgr, const app_manifest_t *manifest, const void *wasm_data, size_t wasm_len);

/**
 * @brief 启动应用
 */
int edge_app_mgr_start(edge_app_mgr_t *mgr, const char *app_id);

/**
 * @brief 停止应用
 */
int edge_app_mgr_stop(edge_app_mgr_t *mgr, const char *app_id);

/**
 * Record an unexpected app-host exit: state becomes FAULTED and restart_count
 * increments. desired_state is left untouched. Returns 0 when persisted.
 */
int edge_app_mgr_report_fault(edge_app_mgr_t *mgr, const char *app_id, const char *reason);

/**
 * @brief 卸载应用，默认 KEEP。
 *
 * KEEP 删除 wasm 包，把注册表行改成 APP_STATE_UNINSTALLED 墓碑：
 * 点位映射与私有数据保留，历史查询可读，启动/停止被拒。
 */
int edge_app_mgr_uninstall(edge_app_mgr_t *mgr, const char *app_id);

/** Query or command over one app id; returns 0 on success. */
typedef int (*edge_app_control_fn)(void *ctx, const char *app_id);

/**
 * One desired-state reconciliation pass (8.19). Starts apps that should run
 * but have no live process, stops processes whose desired_state was cleared.
 * STOPPED apps with desired_state true are pressure-suspended and stay put;
 * FAULTED and UNINSTALLED apps are left to their own state machines.
 * `acted` receives the number of converged apps; it may be NULL.
 */
/**
 * Repair packages interrupted by a crash during an upgrade. The upgrade swaps
 * the package through `<wasm>.new` and `<wasm>.prev`; if the target is gone
 * but the `.prev` copy survived, move it back. Stale `.new` leftovers are
 * removed. Returns the number of repaired apps.
 */
int edge_app_mgr_recover_packages(edge_app_mgr_t *mgr);

int edge_app_mgr_reconcile(edge_app_mgr_t *mgr, edge_app_control_fn is_running,
                           edge_app_control_fn start, edge_app_control_fn stop, void *ctx,
                           int *acted);

/**
 * @brief 按处置策略卸载；opts 为 NULL 时按 KEEP。
 *
 * 本层对三种处置相同，私有数据与导出归档由调用方在回调里处理。
 */
int edge_app_mgr_uninstall_ex(edge_app_mgr_t *mgr, const char *app_id,
                              const app_uninstall_opts_t *opts);

/**
 * @brief 异步安装：字节拷贝在调用线程完成，文件写与仓储提交在 worker 线程完成。
 *
 * 单槽执行器：已有任务时立即返回 EDGE_APP_MGR_BUSY。回调在 loop 线程调用。
 */
int edge_app_mgr_install_async(edge_app_mgr_t *mgr, const app_manifest_t *manifest,
                               const void *wasm_data, size_t wasm_len,
                               edge_app_job_cb_t cb, void *ctx);

int edge_app_mgr_start_async(edge_app_mgr_t *mgr, const char *app_id,
                             edge_app_job_cb_t cb, void *ctx);

int edge_app_mgr_stop_async(edge_app_mgr_t *mgr, const char *app_id,
                            edge_app_job_cb_t cb, void *ctx);

/**
 * One capability's requested scope list for an upgrade. scope_json is a JSON
 * array of exact scopes.
 */
typedef struct {
    char capability[EDGE_AUTHZ_CAPABILITY_SIZE];
    char scope_json[EDGE_AUTHZ_SCOPE_JSON_SIZE];
} app_capability_scope_t;

/**
 * Upgrade an installed app: apply grant inheritance per capability, then take
 * the new manifest's identity and limits while preserving runtime state. New or
 * wider scopes become pending, narrowed or unchanged scopes inherit, revoked
 * grants stay revoked.
 *
 * When wasm_data is non-NULL the package is swapped atomically: the bytes go to
 * <wasm_path>.new, the old file moves to <wasm_path>.prev, and the old registry
 * row is kept as JSON in prev_manifest_json. Any swap failure leaves the
 * registry untouched and the old package in place. The caller owns the restart.
 * Returns 0, or -1 with a reason.
 */
int edge_app_mgr_upgrade(edge_app_mgr_t *mgr, const app_manifest_t *manifest,
                         const void *wasm_data, size_t wasm_len,
                         const app_capability_scope_t *capabilities, size_t capability_count,
                         const edge_authz_store_t *grants, char *reason, size_t reason_cap);

/**
 * Async upgrade on the single-slot worker; package write, rename, registry
 * commit, and grant inheritance stay off the loop. Grants are validated before
 * any mutation and applied last; a store failure rolls the package and registry
 * back. Same BUSY and callback rules as install_async.
 */
int edge_app_mgr_upgrade_async(edge_app_mgr_t *mgr, const app_manifest_t *manifest,
                               const app_capability_scope_t *capabilities,
                               size_t capability_count, const edge_authz_store_t *grants,
                               const void *wasm_data, size_t wasm_len,
                               edge_app_job_cb_t cb, void *ctx);

/**
 * Roll back one upgrade: restore registry fields from prev_manifest_json, put
 * <wasm_path>.prev back when prev_wasm_path is set, clear prev_* and save.
 * Grant rows are intentionally NOT versioned, so grants approved after the
 * upgrade stay as they are. Fails with "no previous version" when prev_version
 * is empty. Runs off the loop.
 */
int edge_app_mgr_rollback_async(edge_app_mgr_t *mgr, const char *app_id,
                                edge_app_job_cb_t cb, void *ctx);

/** Pressure reclaim: stop without clearing the user's desired_state. */
int edge_app_mgr_reclaim_stop_async(edge_app_mgr_t *mgr, const char *app_id,
                                    edge_app_job_cb_t cb, void *ctx);

int edge_app_mgr_uninstall_async(edge_app_mgr_t *mgr, const char *app_id,
                                 edge_app_job_cb_t cb, void *ctx);

/** 异步卸载，默认 KEEP；opts 在提交时拷贝，调用方无需保活。 */
int edge_app_mgr_uninstall_async_ex(edge_app_mgr_t *mgr, const char *app_id,
                                    const app_uninstall_opts_t *opts,
                                    edge_app_job_cb_t cb, void *ctx);

/** 是否有异步任务在队列中或执行中。 */
bool edge_app_mgr_is_busy(const edge_app_mgr_t *mgr);

/**
 * @brief 开机自动恢复所有持久化的 RUNNING 状态应用
 */
int edge_app_mgr_boot_restore(edge_app_mgr_t *mgr);

/**
 * @brief 获取应用状态与列表
 */
int edge_app_mgr_get_app(edge_app_mgr_t *mgr, const char *app_id, app_manifest_t *out_manifest);

/**
 * @brief 从 app.json 文本解析 Manifest 结构体
 */
int edge_app_mgr_parse_manifest_json(const char *json_str, app_manifest_t *out_manifest);

/**
 * @brief 为单 .wasm 字节码文件构造默认 Manifest 描述符
 */
int edge_app_mgr_construct_default_manifest(const char *filename_or_id, app_manifest_t *out_manifest);

#ifdef __cplusplus
}
#endif

#endif // EDGE_APP_MGR_H
