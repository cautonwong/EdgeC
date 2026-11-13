# 公共结构体审计

状态：2026-11-13。扫描范围：`modules/*/include/**/*.h` 和 `include/**/*.h` 中的具名
`struct X { ... }` 定义。当前共 10 个。

分类：

- **value**：纯值/配置，按值或只读指针传递，公开字段就是契约。
- **interface**：虚表或驱动容器，公开函数指针字段是设计本身。
- **handle**：调用方持有或零分配节点，公开布局用于 `sizeof` 或内联存储。
- **legacy**：当前不进构建或没有实现，不新增消费者。

| 结构体 | 文件 | 分类 | 结论 |
|---|---|---|---|
| `storage_driver_s` | `modules/edge_infra/include/storage_ops.h` | interface | ops 虚表 + `priv_ctx` + 名称，驱动容器，保留 |
| `npu_device` | `modules/edge_infra/include/npu_hal.h` | handle | 调用方存储创建设备（`npu_device_create_in`）；`priv_data`/`is_priv_allocated` 是后端私有状态，属于布局暴露。改成不透明存储需要改所有 `npu_device_t` 调用方，暂缓 |
| `pipeline_stage_s` | `modules/edge_stream/include/stream_engine.h` | handle | 零分配 stage 节点，`storage_cfg`/`storage_state` 是内联存储，公开布局是设计的一部分，保留 |
| `pipeline_loader_ctx_s` | `modules/edge_stream/include/stream_engine.h` | value | 加载器依赖注入配置，保留 |
| `mcp_transport_s` | `modules/edge_mcp/include/edgeos/mcp_transport.h` | interface | 传输虚表，保留 |
| `ui_hal_s` | `include/edge_ui.h` | interface | UI HAL 虚表，保留 |
| `ui_page_s` | `include/edge_ui.h` | interface | 页面回调虚表，保留 |
| `ui_service_s` | `include/edge_ui.h` | legacy | `ui_dashboard.c` 不在构建里，`ui_service_create` 没有实现。删除或先改成不透明再复用，当前不改 |
| `bus_engine_s` | `include/bus_engine.h` | interface | 总线引擎虚表 + `user_data`，保留 |
| `uv_periodic_s` | `include/sched_time.h` | handle | 调用方持有的周期定时器，字段是配置和运行态。改成不透明存储需要改所有嵌入方，暂缓 |

## 结论

- 没有发现需要立即改不透明指针的领域聚合泄漏。两个 handle 的私有字段暴露是有意的
  调用方存储设计，代价是布局变化会破坏 ABI；项目当前 `abi_stability=source-only`，可接受。
- `ui_service_s` 是唯一明确的遗留项，但代码不在构建里，先标记，不删。
- `pipeline_stage_s` 的公开布局和零分配绑定，改动它要同时改加载器、池和测试，不划算。

## 门禁

`ci/scripts/check_public_structs.py` 扫描同一组头文件，并与
`ci/baseline/public_structs.txt` 比对：

- 新增公共 struct 没有登记，检查失败。
- 删除 struct 允许，基线可以在同一次改动里收缩。
- 登记格式 `path|name|class`，分类与本文一致。

新增公共 struct 时，先在这里写结论，再更新基线。
