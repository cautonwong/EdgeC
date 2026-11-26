# FORMAL-P2 残余与限制

## 1. 工具链

- **vendored jar 体积**：`formal/tools/tla2tools.jar` 4.5 MB 入库（nightly 无稳定下载 URL）。升级需同时改 `formal/tools/`、manifest 的 `tool`/`jar_sha256`、`formal/README.md` 与证据。
- **Java 只 pin major**：CI 用 Ubuntu 22.04 的 `openjdk-17-jre-headless`，本机为 17.0.21；门禁校验 major 为 17，不 pin 补丁级哈希。Java 非 17 的实机安装未测（代码路径为 `java -version` 提取 major + 比较，CI 步骤另有 grep 兜底）。
- 未在本环境触发 GitHub Actions 本身；本机只能证明同构命令与 vendored 回退。

## 2. Runner 与预算

- 所有分片在同一次 `check_formal.sh` 串行执行并共享 300 s 总预算；新增 owner / 大 harness 推过预算时需先扩预算或增加按 owner 选择（当前未做）。
- CBMC 行按 `(artifact, mode)` 唯一；同一 artifact 的 proof/cover/function 复用 `covers`/`sync`，漂移告警会重复出现（sync 已去重）。
- 未登记扫描只覆盖 `git ls-files` 的 tracked 文件；`git add` 前的未跟踪文件不会报红（CI 只跑已提交代码，符合设计）。

## 3. 负例与断言

- 负例与 `assertions` 依赖 CBMC/TLC 日志文本（如 `_get_crc.pointer_dereference.*`、`Invariant <name> is violated`、`[main.coverage.1]`）；工具升级导致文本变化会先红，这是有意。
- 负例脚本按后缀 `.sh`/`.py` 选择解释器；当前唯一脚本 `modbus_guard_mutation.sh` 内部仍固定其 mutation 目标源码（harness 专属逻辑，由 manifest 声明调用）。

## 4. 模型范围（沿用 FORMAL-P1）

- TLA+ 仍是有界模型：单总线、两命令、`MaxTime=6`、`ASSUME LateBound < Timeout`；不覆盖 pause、帧间静默、队列满丢弃、多总线交错，不证明活性/公平性。
- CBMC 仍只覆盖 `in->len <= 260`、`hi_crc = false`、单次调用；负例锚定自动指针标签。

## 5. 未做

- 未创建任何 DCU 正式工件（按裁决只做机制；临时 probe 已删除）。
- 未接 nightly（沿用 FORMAL-P1 结论：当前全范围在 PR 预算内）。
