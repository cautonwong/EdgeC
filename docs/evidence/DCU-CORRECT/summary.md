# DCU-CORRECT：把重复的代理错误变成仓库约束

日期：2026-11-24。分支 `dcu/correct-guards`（基于 D1-02b 326aa02e，含修复后的对象模型文档）。

## 类 1：设计文档引用不存在的文件/符号（出现 2+ 次）

证据：
- `docs/design/DCU_698_OBJECT_MODEL.md` @0a401712：§8 落点表列 13 个不存在的
  `cmd/dcu/dcu_698_*.c`（`dcu_698_event.c`、`dcu_698_acq.c`、`dcu_698_control.c` 等），
  另引用已删除的 `dcu_698_freeze_read` 与不存在的 `tests/dcu_698_catalog_vectors.h`。
- D1-02 首轮评审 #4 已集中列出；D1-02b 第二轮/第三轮又发现 Class 5/11/22 等归属错误。
- 这类错误是“文档与代码两份事实漂移”，lint 层可完全消除。

采用层级：2（可执行检查）。新增 `ci/scripts/check_doc_references.py`：
- 扫 `docs/design/DCU_698_OBJECT_MODEL.md` 的仓库路径与 `dcu_*()` 调用；
- 文件必须存在；符号必须在 `cmd/adapters/foundation/modules/tests` 的 C/H 中出现；
- 目标态/设计态/不存在/未实现/计划/旧稿/另立 的行豁免（只豁免该行）。

证明：
- 对 0a401712 的旧文档运行（`/tmp/docref-proof.log`）：报出 13 个不存在的
  `cmd/dcu/dcu_698_*.c` 及旧 fixture 引用，退出 1。
- 反例夹具 `tests/guards/fixtures/bad_doc_references`：报 `dcu_698_event.c` 与
  `dcu_698_freeze_read`，退出 1；正例夹具退出 0。
- `run_guard_selftest.py`：112/112（新增正反夹具）；live 检查通过。
- 为什么不用层级 1：文档没有类型/编译边界；把 586 行的“落点表”改成生成物是更大重构，
  且目标态仍需手写；可执行扫描已能阻止同类错误进入合并。

## 类 2：隐式函数声明只在干净构建暴露（出现 2 次）

证据：
- D1-21 合并树 clean build：`tests/test_dcu_698_axdr.c:1509` 调用
  `dcu_698_archive_encode_axdr` 未包含声明，增量分支构建漏过，`check_build_warnings`
  在合并后的 clean log 报 1 条 `-Wimplicit-function-declaration`。
- 同轮修复只加 include；根因是“警告默认不阻断”，换一个测试文件还会复发。

采用层级：2（编译器强制）。`edge_common_interface` 增加
`-Werror=implicit-function-declaration -Werror=implicit-int`。

证明：
- `gcc` 对无声明调用（`/tmp/implicit-bad.c`）加该选项直接报错（退出 1）。
- 分支 clean-first 全量构建通过（`/tmp/correct-build.log`），
  `check_build_warnings.py` 0 new violations，说明现有树无隐式声明。
- 为什么不用层级 1：C 没有模块级导入边界能强制声明；编译器选项是唯一与构建增量性
  无关的强制点。为什么不用层级 3/4：测试/文档不改变“能编译就通过”的事实。

## 规则表更新

`AGENTS.md` 的“规则 → 执行者”新增两行，分别指向 `check_doc_references.py`（含正反夹具
与 live）和上述编译选项。

## 评估过但未固化的类

- **验收/单元测试依赖固定窗口或缓冲假设**（000002 的 DAR2 缓冲、D0-15 的 30s+90s 窗口）：
  两次都由产品行为边界变化触发。当前只能靠“测行为不测内部”判断；没有可执行的通用
  检查（窗口是否足够取决于产品调度）。已保留为 review 判断项，未新增文本规则。
- **共享台架并发**：已发生一次双 agent 同时跑 acceptance。`run.sh` 自带锁，但两个 agent
  仍可同时启动；加“运行前 pgrep”守卫只能提示、不能避免竞态，且会误伤正常串行。
  未固化，建议 operator 侧一次只派验收。
