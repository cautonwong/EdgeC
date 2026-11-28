# DCU-D6-02 规格：DL/T 698.45 真帧向量测试

| 业务规则 | 不变量 | 可观测输出 | 验收判据 | 非目标 |
| --- | --- | --- | --- | --- |
| 1. 新增 `tests/test_dcu_698_wire_vectors.c`，用真实 DL/T 698.45 帧向量覆盖 GET、SET、ACTION、GetRecord、REPORT、RSD 与 DAR 六类。<br>2. 每条向量在测试里注明 `docs/698_parsed.md` 的章节/行号；正例断言响应字节或 DAR，反例断言拒绝。<br>3. 至少覆盖：GET Normal 单项、GET NormalList 逐项 DAR、SET 成功与类型错误、ACTION 6012 与 8000、GetRecord Selector1/2、REPORT 帧布局与确认匹配、未知 OI/未定义属性 DAR 4/5/6。 | 1. 期望值来自标准原文，不从实现输出反推。<br>2. 向量使用现有 codec/驱动入口，不复制帧解析逻辑。<br>3. 不修改产品源码；若向量暴露真实缺陷，停下记 finding 并报告，不在本单顺手改语义。 | 1. 新测试文件在 `tests/CMakeLists.txt` 注册，`ctest -R test_dcu_698_wire_vectors` 运行。<br>2. 每条向量有正例与至少一条反例；错误码按标准（DAR 4/5/6/7）断言。<br>3. 测试在 Release 与 ASAN 构建下均通过。 | 1. 新测试目标单独运行通过，且全量 `ctest` 通过。<br>2. `git diff` 只含测试、CMake 与证据；产品代码零改动（除非发现缺陷并单独报告）。<br>3. `check_build_warnings.py` 0 new、`git diff --check` 干净。<br>4. 证据写入 `docs/evidence/DCU-D6-02/`，列出每条向量的标准来源。 | 1. 不重写已有测试；不复制 `test_driver_dlt698` 的整套用例。<br>2. 不引入新依赖或测试框架。<br>3. 不改协议实现来迁就向量。 |
