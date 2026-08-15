# AGY 第三轮审核反馈（Codex Review Round 3）

> **Mr. WKT** — 评审基准：`20cd4da..feature/modular-refactor`（提交 f6278f1 “ADR-0004 + Leader R2 整改”、f453493 “Storage Subsystem Hardening”）。方法：静态差异审查 + 临时 worktree 构建/全量测试 + Omnis 实测。

---

## 0. 实测结果

| 检查项 | 结果 |
|---|---|
| 干净 checkout 直接 configure | ❌ **失败**：`third-party/cyclonedds` 是 gitlink 但 `.gitmodules` 无映射；`third-party/CMakeLists.txt` 无条件编译 `cyclonedds/src/cyclonedds.c`（目录为空） |
| configure（补 vendored CycloneDDS 链接后） | ✅ 通过，Validator 三产品 PASSED，生成 per-product tags 包 |
| `cmake --build`（补 IDL 头文件生成后） | ✅ 440/440 |
| `ctest` | ✅ **129/129**（含新增 multi-backend 与 IDL 序列化测试） |
| `go test ./...`（含 `pkg/tags/{dcu,iems,smart_box}`） | ✅ 全绿 |
| contract-compiler-v2 测试（Validator） | ✅ 通过 |
| `check_tag_consistency.sh` | ✅ PASSED（SmartBox 64 / IEMS 30 / DCU 7 ≤ 65，且逐 ID 匹配） |
| `check_repo_clean.sh` | ✅ PASSED（但脚本不检查 PDF/`.local` 二进制） |
| `./test.sh --omnis` | ✅ **10/10 通过（48s）**，T-101/T-102 相关场景均绿 |

说明：以上“补”的两处都是我在 /tmp 评审 worktree 里的临时 workaround（链接 vendored libddsc、手动运行 idlc），**仓库本身没有这两步**——干净克隆依然无法构建。

## 1. P0 阻断（本轮必须先修）

### P0-1 CycloneDDS 子模块未接线
- `git ls-files -s third-party/cyclonedds` 显示 mode 160000（gitlink），但 `.gitmodules` 只有 `third-party/zephyr`。
- `third-party/CMakeLists.txt` 仍写 `add_library(cyclonedds STATIC cyclonedds/src/cyclonedds.c ...)`。
- 干净 clone → 空目录 → configure 直接失败。
- 修复建议：二选一——
  a) 在 `.gitmodules` 补齐 CycloneDDS 仓库并让 CI 初始化子模块；
  b) 直接删除 gitlink，改为链接 vendored `.local/linux-x86_64/lib/libddsc.so` + include（你们已经 vendored，建议此方案，最小依赖）。

### P0-2 IDL 生成未接入构建
- `tests/test_idl_generated_serialization.c` include `build/idl_generated/*.h`，并 `#include "../build/idl_generated/*.c"`。
- `tests/CMakeLists.txt` 只加了 include 目录，**没有任何 custom command 调用 idlc** 生成这些文件。
- 干净构建在编译该测试时 `fatal error: dlt698_objects.h: No such file or directory`。
- 修复建议：在 CMake 中为 4 个 `contracts/idl/*.idl` 注册 `idlc` custom command（依赖 `.local/bin/idlc` 与 idl 文件），或提交生成的 `.h/.c` 并明确来源。

## 2. R2 反馈项闭环情况

| R2 要求 | 状态 | 证据/说明 |
|---|---|---|
| 聚合 tags.go 确定性 | ✅ 大部分完成 | `sorted(glob())` 已加；生成 per-product 包（`go/pkg/tags/{dcu,iems,smart_box}`）；根 tags.go 仍存在“同名不同 ID 静默覆盖”（如 VOLTAGE_PHASE_A 1001 vs 536871424），但排序后结果确定；建议根包改名或冲突时报错 |
| 聚合单点执行 | ⚠️ 未完成 | 每个产品的 custom command 仍各自调用 aggregate 写根 tags.go（并行下存在写竞争；实测内容收敛但属隐患） |
| T-002 readBuf 上限 | ✅ 完成 | 65536 上限已加 |
| T-002 客户端状态上限 | ✅ 完成（粗糙） | `len(clients)>=128` 时整体重置 map（有界但会丢状态；可接受，建议改为按连接生命周期删除） |
| T-002 HMAC 密钥外置 | ❌ 未完成 | 仍有 `EdgeOS_DoIP_UDS_SecretKey_2026_dev` 硬编码回退 |
| T-002 DataHub 回写 | ❌ 未完成 | `recordTagValue` 仍只写 SQLite `tag_history`，12031-12035 未走 DataHub |
| T-002 硬编码 DTC/ECU | ❌ 未完成 | `P09012F`、`EdgeC_ECU`、Routine 恒成功仍在 |
| T-002 新增测试 | ❌ 未完成 | f6278f1 未改 `main_test.go` |
| T-003 生产 fail-fast | ⚠️ 部分 | demo-algo/syslog/VPP 已按 `EDGE_ENV` 区分；但 DEV_ONLY 回退仍在，syslog 是“运行时拒签”而非启动失败，无测试 |
| 干净克隆 configure | ❌ 失败 | 被 P0-1 阻断；`config.cmake` include 仍被移除；EdgeContract 的 include 路径是否用 ARCH 需复查 |
| T-103 init 失败路径 | ⚠️ 部分 | `datahub_init` 失败已补 ring 清理；`datahub_init_from_fd` 泄漏未修；无失败注入测试 |
| T-104 fsync | ✅ 完成 | 已加 `fsync`；仍无 p99 与 kill -9 证据 |
| T-105 唤醒 E2E | ⚠️ 部分 | 测试有增加；仍无“C 先启动、Go 后挂载”真实唤醒用例 |
| T-106 清理移出锁 | ✅ 完成 | destroy 清理已移出锁外；仍无 executor double-free 回归与 TSan 记录 |
| T-107 ASan/TSan CI | ❌ 未完成 | CI 只新增 Go 与 Matrix 步骤；无 sanitizer job；`-DENABLE_COVERAGE=ON` 与 CMake `COVERAGE` 不匹配仍在；Go 1.21 vs `go.mod 1.25.5` 仍在 |
| T-201 Validator | ✅ 完成 | validator + 测试质量不错，三产品 strict 通过 |
| T-206 V1 编译器退役 | ✅ 完成 | `tools/contract-compiler/` 已删除 |
| 证据文档 | ⚠️ 部分 | 新增 ADR-0004/0005/0007 与两份 08-14 计划；无逐 Ticket DoD/测试证据文件 |

## 3. 范围与质量风险（新增内容未受审）

1. **超大范围扩张**：f453493 一次性带入 DDS/CycloneDDS 后端、MQTT 后端、DataHub 多后端拆分、4 份 IDL 套件、存储加固、V1 删除、PDF 导入等，总计 +5.3 万行。ADR 有背书，但全部混在一个提交里，无法逐 Ticket 验收；后续必须拆提交。
2. **IDL 真实性存疑**：`scripts/build_exact_5000_lines_per_idl.py`、`build_truly_exhaustive_all_idls.py`、`generate_5000_obis_master_idls.py` 等脚本名暗示“凑行数”；提交的 `dlms_bluebook_objects.idl`（4333 行）、`dlms_cosem_objects.idl`（5511 行）需要人工/工具抽查是否有填充内容，下一轮我会专项核查。
3. **仓库膨胀**：新增 Blue-Book PDF（约 8.6MB）、vendored CycloneDDS 头/库、`idlc/ddsperf/contract-compiler` 二进制入库；`check_repo_clean.sh` 不覆盖 PDF 与 `.local`，建议收紧规则或改走发布制品。
4. **Zephyr 目录删除**（`cmd/edge_node_zephyr`）：与“Zephyr 移出到 /workspaces/applications”决策一致，✅。
5. **子代理评审失败说明**：按 code-review 技能尝试并行 Standards/Spec 子代理，但任务投递被平台吞掉（对方只收到系统上下文），本轮双轴评审由主代理完成；下一轮可重试或改用本地双轴报告。

## 4. 结论与下一轮要求

**结论：暂不可合并。** 功能面上（在补齐 P0-1/P0-2 的临时 workaround 后）全部测试与 Omnis 10/10 通过，R2 反馈约 70% 闭环；但“干净克隆可构建”是硬门禁，目前失败。

下一轮必做：
1. 修 P0-1（CycloneDDS 接线或改连 vendored 库）与 P0-2（IDL 生成接入 CMake）。
2. T-002：HMAC 密钥外置、DataHub 回写、去硬编码 DTC/ECU/Routine、补 Go 测试。
3. T-107：补 ASan/TSan CI job，并修正 COVERAGE/Go 版本参数。
4. 大提交拆分为独立 Ticket 提交；IDL 填充嫌疑专项说明。
5. 附逐 Ticket DoD 证据与干净 clone 构建日志。

---

*Codex 复审口径：干净 clone 全链路绿 = 可合并；P0 未闭环前不进入下一阶段。*
