/**
 * @file iems_app.h
 * @brief IEMS Application Assembler and Lifecycle Coordinator (AEP-406)
 *
 * Implements Composition Root Exclusivity & Assembler Pattern:
 * 1. Encapsulates full assembly sequence (Node, Orchestrator, StreamEngine, Northbound, Timers)
 * 2. Owns iems_node_t aggregate root and exposes safe domain operations
 * 3. Enforces domain boundary for control write operations (AEP-400)
 */

#ifndef IEMS_APP_H
#define IEMS_APP_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "iems_node.h"
#include "iems_lc_user_cfg.h"
#include "iems_pcs_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iems_app_s iems_app_t;

typedef struct {
    const char *db_path;
    const char *shm_name;
    const char *wal_path;
    const char *snap_path;
    const char *mqtt_url;
    int rest_port;
    // REST 管理面绑定地址;NULL = 127.0.0.1。由组合根从 EDGE_REST_BIND 读一次传进来。
    const char *rest_bind;
    // 旧 /api/v1/apps/upload (raw wasm) 入口;默认 false,仅迁移期由
    // EDGE_REST_LEGACY_UPLOAD=1 显式开启。
    bool rest_legacy_upload;
    // 北向 Modbus 从站的 UDS 路径;NULL = 契约表里的 simulation_path(再空则退回
    // /tmp/iems_north_slave.sock,仅第一条北向总线;第二条无路径会报错跳过)。
    // 只覆盖**第一条** NORTHBOUND 总线,多条时其余按契约表各自的 simulation_path。
    // 由组合根从环境变量读一次传进来,深层模块不再 getenv
    const char *north_sock;
    // UI 伴随通道的 UDS 路径;NULL = /tmp/iems_ui.sock
    const char *ui_sock;
    // P6-04b: 转换产物配置根目录(device_config.json + port_config.json);
    // NULL/空串 = 不加载,沿用既有契约/数据库种子。由组合根从 EDGE_CONFIG_DIR
    // 读一次传进来。显式设置后加载失败会拒绝启动。
    const char *config_dir;
    // P1-08a c1: 影子模式。true 时所有下行控制写在聚合根与执行出口被拒绝，
    // 读/轮询/遥测照常。由组合根从 IEMS_SHADOW_MODE 解析一次传进来。
    bool shadow_mode;
    // P1-08a c1 round2: REST /api/v1/io/raw 原始帧写入口。默认关闭；只有
    // IEMS_REST_RAW_IO=1 显式开启且非影子模式才放行。由组合根读一次传进来。
    bool rest_raw_io_enabled;
} iems_app_config_t;

/**
 * @brief 解析 IEMS_SHADOW_MODE 启动开关（组合根边界，只在这里读 raw 值）
 *
 * 未设或 "0" = 正常；"1" = 影子；其它值返回 -1，调用方必须拒绝启动。
 *
 * @param raw 环境变量原值（可为 NULL）
 * @param out_shadow 出参：是否影子模式
 * @return 0 合法，-1 非法
 */
int iems_app_shadow_mode_parse(const char *raw, bool *out_shadow);

/**
 * @brief 解析 IEMS_REST_RAW_IO：只有精确 "1" 开启，其它值（含未设）一律关闭。
 *
 * @param raw 环境变量原值（可为 NULL）
 * @param out_enabled 出参：是否开启 REST 原始帧写入口
 * @return 0（非法值按关闭处理，不拒绝启动）
 */
int iems_app_rest_raw_io_parse(const char *raw, bool *out_enabled);

/**
 * @brief 启动 WAL：打开并回放；任一步失败时记录错误并关闭 WAL（明确降级）
 *
 * 只关闭持久化能力，不中断产品启动。
 *
 * @param hub DataHub 实例
 * @param wal_path WAL 文件路径
 * @return 0 WAL 已启用，-1 已降级为无 WAL
 */
int iems_app_wal_start(datahub_t *hub, const char *wal_path);

/**
 * @brief 装配 IEMS 应用程序各个组件 (聚合根、总线编排器、流计算引擎、北向服务)
 *
 * @param loop libuv 事件循环
 * @param cfg 应用程序配置
 * @return 装配好的 iems_app_t 句柄，失败返回 NULL
 */
iems_app_t *iems_app_assemble(uv_loop_t *loop, const iems_app_config_t *cfg);

/**
 * @brief 启动南向物理总线(组合根调用)
 *
 * 谁发起:组合根。TCP 总线经 iems_tcp_link 状态机启动,首次连不上进入 BACKOFF
 * 并按指数退避重连;UART/UDS 仍只在启动时连一次。启动日志会写明缺哪几路。
 *
 * @param app 应用程序句柄
 * @param configured 出参:契约表里配置的南向总线数(可为 NULL)。用来区分「没配南向总线」和「配了但一路都没启动」
 * @return 已启动的南向总线数(TCP 的 CONNECTED 与 BACKOFF 都算);<0 参数非法或装配契约表缺失
 *
 * 必须在 iems_app_start() 之前调用一次,iems_app_start() 会拒绝未连过总线的半装配启动。
 * 语义:配了南向总线就要求至少启动一路;部分启动照常运行,缺的那几路只 WARN 不致命
 *       (缺 CAN 的机器不该起不来)。TCP 设备启动时离线只进 BACKOFF,不再让组合根 abort;
 *       一路都没启动才算启动失败,由组合根 abort。
 *       能拿到装配契约表时打一行摘要:[IEMS-INIT] southbound: <started>/<configured> started
 *       (契约表本身取不到属于装配坏了,由组合根打 FATAL,这里不出摘要)
 */
int iems_app_connect_buses(iems_app_t *app, int *configured);

/**
 * @brief 启动 IEMS 应用程序 (自律业务循环、内部总线、任务加载等;南向总线由组合根先 connect_buses)
 *
 * @param app 应用程序句柄
 * @return 0 成功, <0 失败
 */
int iems_app_start(iems_app_t *app);

/**
 * @brief 运行 IEMS 应用程序主服务循环
 *
 * @param app 应用程序句柄
 * @return 0 成功
 */
int iems_app_run(iems_app_t *app);

/**
 * @brief 停止并销毁 IEMS 应用程序，释放所有持有资源
 *
 * @param app 应用程序句柄
 */
void iems_app_destroy(iems_app_t *app);

/**
 * @brief 获取 IEMS 聚合根
 */
iems_node_t *iems_app_get_node(iems_app_t *app);

/**
 * @brief 启动期加载的 PCS/EMS 设置（只读）。
 *
 * 未配置 EDGE_CONFIG_DIR 或文件缺失时保持默认值；loaded 表示两份文件都成功加载。
 * 只读：调用方不得修改；策略接线（SOC/功率）属 P2-03/P5-03。
 */
const iems_pcs_settings_t *iems_app_get_pcs_settings(iems_app_t *app);

/**
 * @brief 启动期加载的 per-LC 点位覆盖（只读）。
 *
 * 未配置 EDGE_CONFIG_DIR 或无 lc_devs 时 count 为 0；每项 state 区分缺失文件与
 * 空文件。v3 的 125 默认合并留给消费方，加载器只保留源值。
 */
const iems_lc_user_cfgs_t *iems_app_get_lc_user_cfgs(iems_app_t *app);


/* DataHub 影子布局：产品 tag 在 app_base 以下，应用点位在其上；容量必须覆盖
 * 全部应用槽位，否则 host_publish 会被影子越界检查丢弃。 */
#define IEMS_APP_BASE 131072u
#define IEMS_APP_SLOT_TOTAL (16u * 256u)
#define IEMS_DATAHUB_CAPACITY (IEMS_APP_BASE + IEMS_APP_SLOT_TOTAL)

#ifdef __cplusplus
}
#endif

#endif  // IEMS_APP_H
