# EdgeC Sprint 计划 — 2026-07-07

> 源自 20 轮 Grilling 决策（Grill session 2026-07-07）
> 整体周期 6 周，3 个 Sprint，双产品线（IEMS + DCU）并行打磨

---

## 总体资源分配

```
IEMS + DCU 产品打磨    60%
Edge-Ops MVP + 架构地基  40%
```

## 测试覆盖目标

当前 ~22% → 6 周后 **35%**
重点：集成测试补强 + 核心模块单元测试 + DLMS/DLT698 fuzz

---

## Sprint 4 — 技术地基 🏗️（2 周，W1-W2）

> 核心：Metrics 落地、Pipeline 热重载、Go eventfd、单源校验、DCU 测试起步

### W1 任务

| 天 | 任务 | 产出 | 负责人 |
|---|------|------|--------|
| D1-D2 | **Metrics DataHub 接入** — 在 `datahub.c` 中埋入 4 个指标(12001-12004)，对接 `edge_metrics_register/report` | 首批指标可观测，`/metrics` 有实际数据 | - |
| D2-D3 | **编译期点表校验** — `build.sh` 中增加一步：编译后对比 `contracts.h` 中 tag_count vs Go `tags.go` 中 `len(Metadata)` | C/Go 标签数量一致性 CI 护栏 | - |
| D3-D5 | **Go eventfd 通用改造** — `shmring.c` 每次 write 后写入 eventfd；`watcher.go` 通过 epoll 等待 eventfd，实现 O(1) 唤醒 | SHM 轮询不再线性增长 CPU | - |

### W2 任务

| 天 | 任务 | 产出 | 负责人 |
|---|------|------|--------|
| D1-D4 | **Pipeline 双缓冲热重载** — 实现 `pipeline_prepare/switch/reload`，`contract_instance_activate()` 内部原子切换 Pipeline，使用 RCU 指针交换 | 契约加载→自动热重载，零停机 | - |
| D2-D3 | **DCU 端到端测试起步** — 编写单电表闭环测试：虚拟 DLT645 → 动态轮询 → DataHub → 协议桥接 → IEC104 APDU 验证 | DCU 首个端到端链路测试 | - |
| D4-D5 | **W1 回归验证** — ctest 80 测试全绿 + Go build + Matrix 2 个修复测试重跑 | Sprint 4 可交付 | - |

### Sprint 4 退出标准

- [ ] `curl localhost:8080/api/v1/system/metrics` 返回 DataHub 4 个实际指标（非零值）
- [ ] 修改 `.edgec` blob 后，IEMS 自动切换无须重启
- [ ] Go watcher 使用 eventfd，100Hz 轮询代码路径失效后走 eventfd 路径
- [ ] DCU 单电表闭环测试绿色通过
- [ ] ctest 80/80 pass

---

## Sprint 5 — 产品打磨 🛠️（2 周，W3-W4）

> 核心：IEMS Demo 剧本、DCU 可观测化、Matrix 测试全绿、协议 fuzz

### W3 任务

| 天 | 任务 | 产出 | 负责人 |
|---|------|------|--------|
| D1 | **修复 2 个 Matrix 测试** — 重跑 `test_adc_to_protection_e2e`（日前已修复 shadow 写，直接验证）；`test_iems_multi_tag_consistency` 增强日志后定位根因修复 | Matrix 9/9 通过 | - |
| D2-D4 | **IEMS Demo 主线 — 峰谷套利** | 完整演示剧本+脚本 | - |
| | — 准备电价曲线数据（24h 波峰波谷 JSON） | | |
| | — 配置 MPC 调度器（`mpc_dispatcher_stage`）绑定 DataHub tag | | |
| | — 编写一键 Demo 启动脚本（`demo/peak_shaving.sh`） | | |
| | — 准备量化对比图："有 MPC vs 无 MPC" 日收益对比 | | |
| D3-D4 | **DCU Metrics 接入** — 在 `dcu_dynamic_poller.c` 中注册轮询成功率、超时率、活跃电表数三个指标；DCU 运行时状态在 Go Metrics 端立即可见 | DCU 可观测化 | - |
| D5 | **协议 fuzz 扩展 — DLMS** — 写 4-6 个畸形 DLMS 帧场景（短报文、长度不一致、非法 Tag、越界对象属性） | DLMS fuzz 测试绿色 | - |

### W4 任务

| 天 | 任务 | 产出 | 负责人 |
|---|------|------|--------|
| D1 | **协议 fuzz 扩展 — DLT698** — 写 4-6 个畸形 DLT698 帧场景 | DLT698 fuzz 测试绿色 | - |
| D2-D3 | **IEMS Demo 子场景 — 过压保护** — 复用 Matrix `test_high_freq.py` 场景，包装为可视化步骤（注入→RMS→Relay 动作→恢复） | 安全演示子场景 | - |
| D3-D4 | **IEMS Demo 子场景 — 谐波分析** — Matrix 注入 3 次谐波，DSP RMS+THD 计算，展示波形对比 | 数学能力子场景 | - |
| D4-D5 | **DCU YAML 编译流验证** — 验证 `cmd/dcu/manifest.yaml` → Go compiler → `.h` / `.edgec` → DCU 加载的完整链路 | DCU 契约编译流闭环 | - |
| D5 | **Sprint 5 回归验证** — ctest 全绿 + Go build + Matrix 9/9 + fuzz 扩展 + DCU 编译验证 | Sprint 5 可交付 | - |

### Sprint 5 退出标准

- [ ] Matrix 9 个集成测试全部通过
- [ ] IEMS 峰谷套利 Demo 可一键运行，输出 MPC 策略前后收益对比
- [ ] IEMS 过压保护 Demo 可演示从浪涌注入到继电器动作的全过程
- [ ] DCU 轮询成功率/超时率/活跃电表数可在 `/metrics` 查看
- [ ] DLMS + DLT698 fuzz 测试绿色
- [ ] DCU YAML 编译→加载链路验证通过
- [ ] ctest 80/80 pass

---

## Sprint 6 — 商业化准备 🚀（2 周，W5-W6）

> 核心：Edge-Ops MVP、WASM 签名链、IEMS Demo 最终封装、架构收尾

### W5 任务

| 天 | 任务 | 产出 | 负责人 |
|---|------|------|--------|
| D1-D3 | **Edge-Ops MVP — agent_connector**（C 端） — 在 EdgeC 中增加 MQTT 管理通道（独立于业务 MQTT） | IEMS 和 DCU 节点可通过 MQTT 上报心跳 | - |
| | — 复用 `io_executor_linux.c` 中已有的 MQTT 实现 | | |
| | — 心跳 payload：设备 ID、产品类型、运行时长、`edge_metrics` 关键指标 | | |
| D2-D4 | **WASM 签名链原型** — `build.sh` 增加 `make-keys` 目标；编译器 `--sign` 参数签名 `.wasm`；`wasm_app_create()` 验签 | 可在边缘设备验证 WASM 来源可信 | - |
| D4-D5 | **Edge-Ops MVP — 管理后端**（Go） — 新服务 `edge-ops`：接收 MQTT 心跳、设备在线/离线状态、简单列表展示 | Edge-Ops 从 0→1 | - |

### W6 任务

| 天 | 任务 | 产出 | 负责人 |
|---|------|------|--------|
| D1-D2 | **IEMS Demo 最终封装** — 整合三合一的演示脚本（峰谷套利→过压保护→谐波分析），附带 README 和预期输出截图 | 面向客户的 Demo 包 | - |
| D3-D4 | **架构收尾** — 修复 Sprint 4-5 期间出现的任何回归；确保 80+ 测试全绿；补充 ADR-0001/0002 在 `docs/adr/` 中的实施记录 | 架构债务清理 | - |
| D5 | **最终验收** — 全量构建(C/Go/Web) + 全量测试(ctest + Matrix + Go) + Demo 一键运行 | 6 周计划整体交付 | - |

### Sprint 6 退出标准

- [ ] EdgeC 节点通过独立 MQTT 通道上报心跳到 Edge-Ops 管理后端
- [ ] Edge-Ops 后端可展示设备在线/离线状态列表
- [ ] WASM 签名/验签原型：`build.sh make-keys` → `--sign app.wasm` → `wasm_app_create()` 验签通过
- [ ] IEMS 三合一 Demo 一键运行，输出可复现的可视化结果
- [ ] 整体测试覆盖 ≥35%
- [ ] ctest + Matrix + Go build 全绿

---

## 20 轮决策索引

| # | 决策 | 类别 |
|---|------|------|
| 01 | 方案 B → 方案 A 接力（技术地基→商业化Demo） | 战略 |
| 02 | DataHub 优先接入 Metrics | 架构 |
| 03-A | Pipeline 热重载由契约加载自动触发切换 | 架构 |
| 03-B | IEMS + DCU 双产品并行打磨 | 战略 |
| 04 | DCU 打磨全都要（测试+Metrics+编译流） | 质量 |
| 05 | 编译期点表校验 | 质量 |
| 06 | eventfd 通用改造（C+Go） | 架构 |
| 07 | DataHub 首批 4 个指标 | 架构 |
| 08 | DCU 端到端测试：单电表闭环 | 质量 |
| 09 | IEMS Demo 三部曲（峰谷套利+过压+谐波） | 产品 |
| 10 | 保护测试重跑，多点一致性测试增强日志 | 质量 |
| 11 | 6 周三 Sprint 计划框架 | 管理 |
| 12 | Metrics 埋点位置确认（datahub.c 4 处） | 架构 |
| 13 | Pipeline 接口三件套（prepare/switch/reload） | 架构 |
| 14 | Edge-Ops MVP = agent_connector + 在线状态 | 产品 |
| 15 | WASM 签名链原点 | 产品 |
| 16 | DLMS + DLT698 手工 fuzz | 质量 |
| 17 | Edge-Ops 技术栈 = Go + MQTT | 架构 |
| 18 | 资源分配 60/40 | 管理 |
| 19 | 6 周测试覆盖目标 35% | 质量 |
| 20 | 此计划文档为最终交付 | 管理 |

---

## 风险登记册

| 风险 | 可能性 | 影响 | 缓解措施 |
|------|--------|------|----------|
| IEMS Matrix 测试修复时间超预期 | 中 | Sprint 5 延迟 | W3 D1 先重跑确认，如根因复杂可并行开分支修复 |
| Edge-Ops MVP 2 人周不够 | 中 | Sprint 6 延迟 | MVP 范围再做减法：先砍 Web UI，只留 MQTT+CLI 展示 |
| WASM 签名链与 WAMR 集成有坑 | 低 | Sprint 6 局部延迟 | 原型期先不集成到主 wasm_mgr.c，独立 demo 程序验证 |
| Go eventfd 线程安全问题 | 低 | Sprint 4 延迟 | watcher.go 已用 context 管理生命周期，eventfd 读写全用 syscall 原子操作 |
