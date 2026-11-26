# DCU-D4-06：Class 18 文件传输只读/DAR 诚实骨架

分支 `dcu/D4-06-file`，工作树 `/tmp/edgec-land/d4-06-file`，基线 `feat/dcu`
`814f1de9`（D4-09 已合入）。

## 目录与标准核对

- 表175/177（`docs/698_parsed.md:4804-4909`）：Class 18 属性 A1..A3、方法 1..8
  （复位/执行/删除/校验/代发/代收/上传/下载）。
- 表A.11（`698_parsed.md:8958-9070`）：F000 文件分帧传输管理（A4 文件内容、
  A5 当前指针）、F001 文件分块传输管理（A4 传输块状态字，另有方法 9 读文件、
  10 软件比对）、F002 文件扩展传输管理（A4 服务器信息）。目录实例为 F000..F002。
- 产品没有文件传输存储/DFU 激活/校验/回滚来源。

## 实现

- `cmd/dcu/dcu_router.h`：新增 `IC_FILE_TRANSFER 18`、`OI_FILE_FRAME/CHUNK/EXT`。
- `cmd/dcu/dcu_698_object_model.c`：F000/F001/F002 三个对象行（空掩码，SINGLETON）；
  `k_oi_max_attr` 登记 F000 A1..A5、F001/F002 A1..A4；`_class_defines_method` 增
  `OI_FILE_CHUNK` 方法 9/10 的实例定义（号段在 127 以下，沿用既有 per-OI 例外写法）。
  类上限表 `{18,3}`/`{18,8}` 本就与标准一致，未改。
- `cmd/dcu/dcu_698_appconn.c`：3 个只读 ACL 行：F000 A1..A5、F001/F002 A1..A4，
  方法 1..8。F001 方法 9/10 已计入 resolver 定义，但 ACL 每对象方法槽 8 已满，记 HOLD。
- 设计文档：对象 128→131、ACL 115→118、列表 5551→5806 B；Class 18 行 none→DAR；
  `DCU_698_DEVIATIONS.md` 增 Class 18 行（存储/DFU 缺口 + 方法槽限制）。

## DAR 行为（实测断言）

| 请求 | resolver | 主站（先过 ACL） |
| --- | --- | --- |
| GET F000 A1..A5 / F001/F002 A1..A4（定义、无来源） | DAR 5 | DAR 5（ACL 广告） |
| GET F000 A6 / F001/F002 A5（未定义） | DAR 4 | DAR 3 |
| GET 未知 OI F003 | DAR 6 | DAR 3 |
| ACTION 方法 1..8（复位/执行/删除/校验/代发/代收/上传/下载） | DAR 5，且绝不传输 | DAR 5 |
| ACTION F001 方法 9/10（定义、无处理器） | DAR 5 | DAR 3（ACL 方法槽 8 已满，HOLD） |
| ACTION 方法 11 / F000/F002 方法 9（未定义） | DAR 4 | DAR 3 |
| SET F000 A2 | DAR 5（空掩码无写处理器） | DAR 3（ACL 只读） |

不接收分块、不校验、不激活、不回滚，不伪造传输结果。

## 计数与预算

- 对象表 **131** 行；Class 20 ACL **118** 项（107 手写 + 11 参数宏）。
- 对象列表实测 **5806 B / 118 项**，帧后 **5828 B**；< `EDGE_SLAVE_AXDR_MAX 10240`，
  < 从站池 usable 10432 B（10496 − 64 headroom）。
- ACL 会话：118 × 16 B + header < 4096；118 < 224。

## 红先绿后

- 测试提交 `df20cf2c`：5 个测试文件；旧码红：object `128 != 131`、Class 18 行不存在、
  axdr `-20 != -21`、scheme/driver `115 != 118`、driver DAR `3 != 5`。
- 修复提交 `5eef4736`：3 行 + per-OI 属性/方法定义 + ACL + 文档；`ctest -R 'dcu|dlt698'`
  45/45 绿。

## 变异守卫

`ci/mutation_guards.tsv` 新增 `DCU-D4-06-file-oi-methods`：把 `_class_defines_method`
的 F001 9/10 例外改成 `0 && ...`；`run_mutation_guards.py --only` 证明变异后红
（rc=8）、恢复后绿（rc=0），钉住 “F001 OI 方法 9/10 计入定义”。

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
python3 ci/scripts/run_mutation_guards.py --build build --only DCU-D4-06-file-oi-methods
TESTPLATFORM=/home/edge/test-platform PYTHON=/home/edge/EdgeC/.venv/bin/python3 \
      ./acceptance/run.sh dcu
```

结果：clean build 0 第一方 warning；45/45 绿；六项守卫 0；mutation 1/1 敏感；验收
连续 3 次 `35 passed`。

## 未决项

- F001 方法 9/10 因 ACL 每对象方法槽 8 已满未广告（主站 DAR 3）；要广告需扩
  `method_ids` 并重算 D1-21 会话预算或裁减类方法广告。
- 无文件传输存储/DFU 激活/校验/回滚来源；有真实来源后补 mask、处理器与验收。
- 计划/交接文字由 verifier 更新。
