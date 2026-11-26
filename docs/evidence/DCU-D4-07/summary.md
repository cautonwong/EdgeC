# DCU-D4-07：Class 21 ESAM 只读/DAR 诚实骨架

分支 `dcu/D4-07-esam`，工作树 `/tmp/edgec-land/d4-07-esam`，基线 `feat/dcu`
`88e1e550`（D4-04 已合入）。

## 目录与标准核对（含一处纠正）

- 表184（`docs/698_parsed.md:5070-5108`）：Class 21 属性 A1..A15、方法 1..12。
- `docs/design/DCU_698_OI_CATALOG.md`：Class 21 的实例只有 **F100**；
  `698_parsed.md:9082-9083` 明确 **F101 是 Class 8 安全模式参数**。本单注册 F100；
  F101 不按 Class 21 注册（否则与目录、类定义测试冲突），主站 GET F101 按未知 OI 回
  DAR 6（主站未进 ACL 时先 DAR 3）。任务单里“F101 属于 Class 21”与标准/目录不符，已纠。
- 产品没有 ESAM 硬件/安全会话来源；`dcu_security_*` 是软件模拟（XOR 假密钥），只供
  demo/单测的 C API，不再从 698 面暴露。

## 实现

- `cmd/dcu/dcu_698_object_model.c`：F100 行改为空属性/方法掩码（原来接
  `dcu_698_security_f100_execute` 方法 1）；类上限表 `{21,15}`/`{21,12}` 本就正确。
- 删除 698 适配器 `dcu_698_class_security.c`、其声明与 CMake 条目；软件 ESAM C API
  保留。
- `cmd/dcu/dcu_698_appconn.c`：F100 只读 ACL：A1..A15 rights=1；方法 1..8 广告，
  9..12 因 ACL 每对象方法上限 8 记 HOLD。
- 单测：`test_dcu_advanced_features_dispatch` 改为要求 698 方法 1 回
  `EDGE_SLAVE_SRC_CLASS_MISMATCH`，且软件 ESAM 会话/密钥保持不变；不再断言 698 复位成功。
- 设计文档：Class 21 行与 §8.6 更新；`DCU_698_DEVIATIONS.md` 增 Class 21 行。

## DAR 行为（实测断言）

| 请求 | resolver | 主站（先过 ACL） |
| --- | --- | --- |
| GET F100 A1..A15（类定义、无来源） | DAR 5 | DAR 5（ACL 广告） |
| GET F100 A16（类未定义） | DAR 4 | DAR 3（ACL 未广告） |
| GET F101 A2 / 未知 OI | DAR 6 | DAR 3 |
| ACTION 方法 1..12（复位/执行/数据读取/更新/协商/钱包/密钥/证书/时效/钱包初始化/红外认证） | DAR 5，且绝不执行 | 1..8 DAR 5；9..12 DAR 3（ACL 上限） |
| ACTION 方法 13（类未定义） | DAR 4 | DAR 3 |
| SET F100 A2 | DAR 5（空掩码无写处理器） | DAR 3（ACL 只读） |

不返回序列号/版本/会话/MAC/密钥；698 路径不会清软件 ESAM 状态。

## 计数与预算

- 对象表 **120** 行（F100 行原本已存在，本单只改掩码，行数不变）。
- Class 20 ACL **107** 项（96 手写 + 11 参数宏）。
- 对象列表实测 **4455 B / 107 项**，帧后 **4477 B**；< `EDGE_SLAVE_AXDR_MAX 10240`，
  也 < 从站池 usable 10432 B（10496 − 64 headroom）。
- ACL 会话：107 × 16 B + header < 4096；107 < 224。

## 红先绿后

- 测试提交 `5dcbe1a3`：6 个测试文件；旧码红：advanced `0 != -21`、object mask 不等、
  object `-20 != -21`、axdr `-20 != -21`、scheme/driver `106 != 107`、driver DAR
  `3 != 5`。
- 修复提交 `d335de15`：F100 空掩码、删 698 reset 适配器、只读 ACL、文档；
  `ctest -R 'dcu|dlt698'` 45/45 绿。

## 变异守卫

`ci/mutation_guards.tsv` 新增 `DCU-D4-07-esam-attr15-advertised`：把 F100 ACL 的
15 项属性改成 14 项；`run_mutation_guards.py --only` 证明变异后红（rc=8）、恢复后绿
（rc=0）。该用例钉住 A1..A15 全部只读广告。

## 验证命令与结果

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DPython3_EXECUTABLE=/home/edge/EdgeC/.venv/bin/python3
cmake --build build --clean-first -j4
ctest --test-dir build -R 'dcu|dlt698' --output-on-failure
python3 ci/scripts/check_build_warnings.py --log <clean-build.log> --root .
python3 ci/scripts/check_build_freshness.py --root . --build build
python3 ci/scripts/check_doc_references.py --root .
python3 ci/scripts/check_no_dynamic_memory.py --root .
python3 ci/scripts/check_public_structs.py --root .
python3 ci/scripts/check_stack_usage.py --root .
python3 ci/scripts/run_mutation_guards.py --build build --only DCU-D4-07-esam-attr15-advertised
TESTPLATFORM=/home/edge/test-platform PYTHON=/home/edge/EdgeC/.venv/bin/python3 \
      ./acceptance/run.sh dcu
```

结果：clean build 0 第一方 warning；45/45 绿；六项守卫 0；mutation 1/1 敏感；验收
连续 3 次 `35 passed`。

## 未决项

- 方法 9..12 因 ACL 每对象方法上限 8 未广告（主站 DAR 3）；要广告需扩
  `method_ids` 并重算 D1-21 会话预算或裁减方法广告。
- 无 ESAM 硬件/会话来源；有真实来源后补 A2/A3 序列号/版本、处理器与验收。
- 任务单“F101 属于 Class 21”与标准/目录不符：本单按 F100（Class 21）落地，F101 保持
  Class 8 安全模式参数。
- 计划/交接文字由 verifier 更新。
