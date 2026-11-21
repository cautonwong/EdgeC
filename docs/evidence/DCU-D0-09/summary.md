# DCU-D0-09 对齐对象表 class_id 与 698 目录

基线 `438ed230`（D0-08a 已合入），分支 `dcu/D0-09-class-ids`。

## 问题

手写对象表 `cmd/dcu/dcu_698_object_model.c` 的 `class_id` 与 `docs/design/DCU_698_OI_CATALOG.json` 对不上四处：

| OI | 表里的 class_id | 标准 | 说明 |
| --- | --- | --- | --- |
| F100 ESAM | 10 | 21 | 目录里根本没有这行，因为生成器按“名字含中文”过滤，`F100 21 ESAM` 被丢掉 |
| F200 串口 | 10 | 22 | 目录 `inferred_class` 22 |
| 3000 失压 | 7 | 24 | 目录 24（分项事件对象接口类） |
| 3005 过流 | 7 | 24 | 同上 |

## 改动

- `cmd/dcu/dcu_router.h`：新增 `IC_ESAM 21`、`IC_PERIPHERAL 22`、`IC_SUB_EVENT 24`；修正 `IC_EVENT`、`IC_ACQUISITION` 的示例注释，F100 不再挂在采集监控类下。
- `cmd/dcu/dcu_698_object_model.c`：四个 OI 改用正确的 class_id。`dcu_698_snapshot` 原本按 `class_id == IC_EVENT` 分派，3xxx 改成 24 后必须换判据，改为 `obj->read == _oi_event_read`（处理器族），并保留一行说明为什么不用 class_id。快照行为不变，由 `test_dcu_advanced_modules` 的 3000/3005 断言覆盖。
- `scripts/extract_dcu_698_catalog.py`：`parse_ois` 不再用“名字含中文”过滤，改为“IC 列必须是已解析的 7.3 类之一”。这样 `F100 21 ESAM` 进入目录；唯一的 OCR 碎行 `3467 0 V ...`（IC 0，不是任何类）被拒。另加一条断言：F100 必须以 class 21 出现。
- `docs/design/DCU_698_OI_CATALOG.{md,json}`：重新生成。

## 目录 diff

只有一行新增与两处计数变化：

```
+ {"oi": "F100", "name": "ESAM", "parsed_class": 21, "inferred_class": 21, "source_line": 9082}
Classes: 26. OI rows: 513 -> 514. Rows where the parsed IC and the range mapping disagree: 79（不变）
Class 21 行：| 21 | ESAM 接口类 | 15 | 12 | 1 | F101 |  ->  | 2 | F100..F101 |
```

没有其他 OI 新增或消失；79 个已知 OCR 冲突留给 D1-01。

## 验证

| 项 | 命令 | 结果 |
| --- | --- | --- |
| 干净构建 | `cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release` + `cmake --build . -j8` | 退出 0，0 错误，0 告警 |
| 告警门禁 | `check_build_warnings.py --log build-warn/build.log --root .` | No new violations |
| 单测 | `ctest -R 'dcu\|dlt698' --output-on-failure` | 28/28，退出 0 |
| 目录门禁 | `check_dcu_698_catalog.py --root .` | catalog matches，退出 0 |
| 守卫自检 | `run_guard_selftest.py --root .` | 全部通过 |
| 验收 | `./test.sh --omnis dcu` | 2 个任务、6 条用例全合格，退出 0 |
| 格式 | `git diff --check` | 退出 0 |

`test_dcu_698_object_model` 现按新 class_id 断言 F100=21、F200=22、3000=24、3005=24；`test_dcu_advanced_modules` 继续验证 3000/3005 快照能抓到值。

## 注释审查

`pstack:no-comments` 删了 55 行叙述性注释与已被实现推翻的说明；恢复了 1 行（快照分派依据，派工要求）。三条 MUST KILL/HOLD 记录在 `comments.md`，其中 3000/3005 的 `attr_mask` 仍用 Class 7 属性号与 Class 24 定义冲突，本单冻结 mask 未改。

## 没做的

- 没有改 `attr_mask` / `method_mask`、没有新增 OI、没有碰驱动与北向适配。
- 没在 GitHub Actions 上重跑；以上为本机同命令结果。
- D1-10 的一致性测试尚未开始，等本次合并后再基于新基线做。
