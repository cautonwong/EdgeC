# 应用资源限制贯通设计

日期：2026-11-14
状态：阶段一和阶段二已实现（heap/pids/cpu 贯通，calls/publish 在 Host ABI 限流，storage 明确拒绝），阶段三待排
相关代码：`modules/edge_mgmt/src/app_manifest.c`、`cmd/iems/iems_app_manager_adapter.c`、`cmd/iems/iems_app_runtime.c`、`cmd/edgeos-launcher/main.c`

## 1. 问题

Manifest 已经解析六个资源限制，但运行时没有完整执行。

| 限制 | 当前状态 |
|---|---|
| `limits.heapKb` | 新 schema 的字段被丢弃，运行时读旧平铺字段的默认值 256KB |
| `limits.pidsMax` | Launcher 写死 32 |
| `limits.storageKb` | 无执行点 |
| `limits.cpuMsPerSec` | 无执行点 |
| `limits.callsPerSec` | Host ABI 无频率限制 |
| `limits.publishPerSec` | 发布路径无频率限制 |

这会产生一个错误的契约。应用声明了限制，校验通过，安装成功，但运行时没有执行。安全审查把这条列为 P1。

## 2. 目标

每个声明的限制必须满足下面之一。

1. 运行时真正执行。
2. 明确拒绝清单，并给出具体原因。

不允许第三种状态，即校验通过但静默忽略。

## 3. 数据流

```
app.yaml limits
      |
      v
app_spec_t.limits          (解析层，已有)
      |
      v
app_manifest_t / app_limits  (注册表层，需持久化)
      |
      +--> App Runtime 配置
      |        |
      |        +--> WAMR heap
      |        +--> launcher spec: pidsMax, cpuMsPerSec, memMax
      |        +--> Host ABI: callsPerSec, publishPerSec
      |
      +--> storage quota (app data dir)
```

### 3.1 持久化

推荐新增 `app_limits` 表，不继续加宽 `app_registry`。

```sql
CREATE TABLE IF NOT EXISTS app_limits (
  app_id TEXT PRIMARY KEY,
  heap_kb INTEGER NOT NULL DEFAULT 0,
  storage_kb INTEGER NOT NULL DEFAULT 0,
  cpu_ms_per_sec INTEGER NOT NULL DEFAULT 0,
  calls_per_sec INTEGER NOT NULL DEFAULT 0,
  publish_per_sec INTEGER NOT NULL DEFAULT 0,
  pids_max INTEGER NOT NULL DEFAULT 0
);
```

理由：限制是应用生命周期属性，但和注册表热行分开，避免每次状态变更重写限制；卸载时和 `app_registry` 同一事务删除。

`edge_app_repo_t` 增加三个可选端口函数：`save_limits`、`get_limits`、`delete_limits`。没有实现这三个函数的测试仓库回退到默认值，保持现有测试兼容。

### 3.2 安装与升级

`_control_install_app` 和升级入口解析 `app_spec_t` 后，先把 `spec.limits` 写入 `app_limits`，再启动应用。写入失败则安装/升级失败，不允许注册表和限制表不一致。

### 3.3 启动

`_iems_start_done` 读取 `app_limits`，调用新的
`iems_app_runtime_start_ex(rt, app_id, wasm_path, heap_kb, stack_kb, pids_max, cpu_ms_per_sec)`。
旧的 `iems_app_runtime_start` 保留为默认参数的包装，避免测试大改。

`start_via_launcher` 使用清单值，不再写死：

- `spec.pids_max = limits.pids_max`，0 表示不限制。
- `spec.cpu_ms_per_sec = limits.cpu_ms_per_sec`，0 表示不限制。
- `spec.mem_max_bytes = heap_kb * 1024 + APP_HOST_OVERHEAD`，overhead 固定并在头文件说明。

### 3.4 Launcher

`launch_spec_t` 增加 `uint32_t cpu_ms_per_sec`。`setup_cgroup` 在值大于 0 时写 `cpu.max`：

```
quota = cpu_ms_per_sec * 1000
period = 1000000
"<quota> <period>\n"
```

`pids.max` 使用 `spec.pids_max`，0 时写 `max` 或跳过。写入失败按现有 `require_isolation` 策略处理。

### 3.5 Host ABI 频率

`callsPerSec` 和 `publishPerSec` 在 app-host 服务端执行。每个 app 一个固定窗口，窗口 1 秒，`point_publish` 同时计入调用总数和发布数。超过限制时返回明确的 `rate_limited` 错误，不静默丢弃，应用保持运行。`app_host_ports_t` 的 `calls_per_sec` 和 `publish_per_sec` 为 0 表示不限制。

### 3.6 存储配额

`storageKb` 没有现成的内核配额。候选方案：

1. 对应用数据目录使用文件系统 project quota 或独立 loop 设备。可靠，但部署复杂。
2. 定期扫描目录大小，超限时暂停应用并告警。实现简单，但不是硬上限。
3. 暂不支持，声明 `storageKb > 0` 的清单直接拒绝。

建议先选 3，把契约做实；需要硬配额的产品再排 1。不要选 2 后宣称是硬限制。

## 4. 拒绝策略

在安装和升级入口检查 `spec.limits`。未实现的字段大于 0 时拒绝，原因写清楚，例如：

```
unsupported limit: storageKb (declared 64, no storage quota on this platform)
```

拒绝发生在任务台账创建之前，调用方看到失败，不会留下半安装状态。

## 5. 迁移

新增 schema v15，创建 `app_limits` 表。旧应用没有行时使用默认值，即不限制，保持兼容。

`app_manifest_t` 不增加字段，避免扩大公共结构体和 `prev_manifest_json`。限制通过独立表和端口访问。

## 6. 测试

1. `test_app_manifest` 继续覆盖六个字段的解析和范围校验。
2. 新增 `test_app_limits`：写入、读取、删除、旧应用无行时默认值。
3. `test_edge_launcher` 的 `base_spec` 增加 `cpu_ms_per_sec`，断言临时 cgroup 的 `cpu.max` 和 `pids.max`。
4. `test_iems_app_runtime` 或新增用例断言 runtime 把清单值传给 launcher spec。
5. REST 层增加两个用例：支持的清单安装成功；声明 `storageKb` 或频率限制的清单被拒绝。
6. e2e 清单声明 `heapKb`、`pidsMax`、`cpuMsPerSec`、`callsPerSec`、`publishPerSec`，存储配额仍拒绝。

## 7. 分阶段

1. 阶段一：持久化、heap、pids、cpu。已完成，schema v15 与 `app_limits` 表已落地。
2. 阶段二：Host ABI 的 calls/publish 固定窗口限流。已完成，超限返回 `rate_limited`。
3. 阶段三：存储硬配额。待排。

阶段二完成前，审查里的 P1-02 仍不能关闭。阶段一解决了“声明了但静默忽略”。

## 8. 待确认

1. 频率超限的行为：已选拒绝单次调用，应用保持运行。
2. 存储配额是否允许先拒绝 `storageKb > 0`。建议允许。
3. `cpuMsPerSec` 的 period 固定 1 秒是否可以。cgroup v2 支持其他 period，但 1 秒最简单。
4. 是否需要为 trusted 应用保留 `best_effort` 模式。建议不保留，限制只有执行或拒绝两种。
