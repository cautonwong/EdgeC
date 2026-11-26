# DCU-D4-09：Class 23 总加组只读/DAR 诚实骨架

分支 `dcu/D4-09-group`，工作树 `/tmp/edgec-land/d4-09-group`，基线 `feat/dcu`
`cd4adcd0`（D4-07 已合入）。

## 目录与标准核对

- 表190（`docs/698_parsed.md:5253-5413`）：Class 23 属性 A1..A18、方法 1..5
  （清空总加配置、执行、添加一个单元、批量添加、删除一个单元）。
- `docs/design/DCU_698_OI_CATALOG.md`：Class 23 实例为 2301..2308（总加组 1..8，
  `698_parsed.md:6795-6802`），类归属 23，无 F/其它 OI 混淆。
- 产品没有总加组配置、总加值、滑差窗口、功控/电控轮次来源。

## 实现

- `cmd/dcu/dcu_router.h`：新增 `IC_GROUP 23`、`OI_GROUP_FIRST 0x2301`、
  `OI_GROUP_LAST 0x2308`。
- `cmd/dcu/dcu_698_object_model.c`：2301–2308 共 8 个对象行（空属性/方法掩码，
  SINGLETON）；类上限表 `{23,18}`/`{23,5}` 本就与表190 一致，未改。
- `cmd/dcu/dcu_698_appconn.c`：8 个只读 ACL 行：A1..A16 rights=1、方法 1..5。
  ACL 属性 schema 只到 16，A17/A18 不能广告（D4-04/D4-07 同口径，记 HOLD）。
- 设计文档：对象 120→128、ACL 107→115、列表 4455→5551 B；Class 23 行 none→DAR；
  `DCU_698_DEVIATIONS.md` 增 Class 23 行（producer 缺口 + A17/A18 的 ACL schema 限制）。

## DAR 行为（实测断言）

| 请求 | resolver | 主站（先过 ACL） |
| --- | --- | --- |
| GET A1..A16（类定义、无来源） | DAR 5 | DAR 5（ACL 广告） |
| GET A17/A18（类定义、超 ACL schema） | DAR 5 | DAR 3（ACL 不广告） |
| GET A19（类未定义） | DAR 4 | DAR 3 |
| GET 未知 OI 2309 | DAR 6 | DAR 3 |
| ACTION 方法 1..5（清空/执行/添加/批量添加/删除） | DAR 5，且绝不执行 | DAR 5 |
| ACTION 方法 6（类未定义） | DAR 4 | DAR 3 |
| SET A2 | DAR 5（空掩码无写处理器） | DAR 3（ACL 只读） |

不返回总加值、滑差或轮次，不执行总加配置变更。

## 计数与预算

- 对象表 **128** 行；Class 20 ACL **115** 项（104 手写 + 11 参数宏）。
- 对象列表实测 **5551 B / 115 项**，帧后 **5573 B**；< `EDGE_SLAVE_AXDR_MAX 10240`，
  < 从站池 usable 10432 B（10496 − 64 headroom）。
- ACL 会话：115 × 16 B + header < 4096；115 < 224。

## 红先绿后

- 测试提交 `d50e3395`：5 个测试文件；旧码红：object `120 != 128`、Class 23 行不存在、
  axdr `-20 != -21`、scheme/driver `107 != 115`、driver DAR `3 != 5`。
- 修复提交 `009e8fa1`：8 行 + ACL + 文档；`ctest -R 'dcu|dlt698'` 45/45 绿。

## 变异守卫

`ci/mutation_guards.tsv` 新增 `DCU-D4-09-group-attr16-advertised`：把 2301 ACL 的
16 项属性改成 15 项；`run_mutation_guards.py --only` 证明变异后红（rc=8）、恢复后绿
（rc=0），钉住 “A1..A16 全部只读广告”。

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
python3 ci/scripts/run_mutation_guards.py --build build --only DCU-D4-09-group-attr16-advertised
TESTPLATFORM=/home/edge/test-platform PYTHON=/home/edge/EdgeC/.venv/bin/python3 \
      ./acceptance/run.sh dcu
```

结果：clean build 0 第一方 warning；45/45 绿；六项守卫 0；mutation 1/1 敏感；验收
连续 3 次 `35 passed`。

## 未决项

- A17/A18 因 ACL 属性 schema 上限 16 未广告（主站 DAR 3）；要广告需扩 ACL 属性位宽
  （2 bit/属性 压 32 位的 schema）或标准裁剪。
- 无总加组配置/总加值/滑差/功控来源；有真实来源后补 mask、处理器与验收。
- 计划/交接文字由 verifier 更新。
