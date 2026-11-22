# DCU-D5-01/02 Class 2 最大需量 + Class 5 谐波（partial by design）

分支 `dcu/D5-01-02-readonly`，工作树 `/tmp/dcu-d5-01-02`，基线 `a3f8d40f`，已 rebase 到含 D2-04 的 main。

## 前提核查

两条数据链都没有产品侧生产者，广告任何值都会是假成功：

- Class 2：`dcu_vertical_tasks` 的 `max_demand_kw` 只在测试里更新，模块没有实例、没有喂数；驱动的三张点表（DLT698/645/Modbus）都没有需量 OAD/DI；DataHub 无影子标签。设计本来就要求“最大需量值来自电表，DCU 不本地重算”（`docs/design/DCU_698_OBJECT_MODEL.md` Class 2）。
- Class 5：`dcu_power_quality` 能算 2..31 次谐波与 THD，但只存一份 A 相电压分析，B/C 相与电流没有槽位；该文件只编进两个测试可执行文件，产品路径没有实例；没有谐波影子标签或写入者。

## 本次落地的骨架

| OI | 接口类 | 范围 | 对象 mask | ACL |
| --- | --- | --- | --- | --- |
| 1010/1011/1012/1013 | IC_MAX_DEMAND (2) | 每表 | 空 | 属性 1..3 只读，方法 1/2 |
| 200D/200E | IC_HARMONIC (5) | 每表 | 空 | 属性 1..6 只读，方法 1/2 |

对象 mask 为空，所以请求穿过驱动 ACL 后由对象表回答：

- 类定义、对象未声明的属性 → DAR 5（1010 A1/A2/A3，200D A1..A6，200E A1..A6）
- 类未定义的属性 → DAR 4（1010 A4，200D A7）
- 类定义、对象未声明的方法 → DAR 5（方法 1/2）；类未定义的方法 → DAR 4（方法 3）
- 表外 OI → DAR 6（模块层）

没有 placeholder 值、零数组或返回成功的方法；ACL 不授予任何写权限。

计数变化（含已合并的 D2-04 的 6014/6015）：对象表 32 → 38，Class 20 ACL 31 → 37，`EDGE_SLAVE_ACL_MAX_OBJECTS` 32 → 40（37 × 16 = 592 B，仍在 `DRIVER_PRIV_SIZE` 1024 内，`_Static_assert` 继续把关）。

## 跟进项（不在本分支实现）

1. **需量 producer**：给三张点表之一加经核实的最大需量 OAD/DI，并把值写进 DataHub 影子；或引入本地区间需量 producer。设计等级是 `optional`，所以 DAR 5 符合首版约定。
2. **谐波 producer**：FFT/频谱 producer 调 `dcu_analyze_harmonics_thd`，补齐 B/C 相与电流槽位，并把结果写入 200D/200E 影子；Class 5 设计状态因此从 `read` 改为“read，DAR 5 until a producer exists”。

## 验证

- 干净构建（`ninja clean` + 全量重建）+ `check_build_warnings.py`：0 warning
- 全量 `ctest -j8`：342/342
- ASAN：`test_dcu_698_axdr`、`test_dcu_698_appconn`、`test_dcu_698_object_model`、`test_driver_dlt698`、`test_dcu_698_catalog` 全过
- `run_guard_selftest.py`：全部通过
- `./test.sh --omnis dcu`：2/2 任务，`dcu-northbound` 21/21（含新用例 `D_00_00_02_000023`），`ACCEPTANCE_TASKS` 2/2
- `git diff --check`：干净

测试覆盖：`tests/test_dcu_698_axdr.c::test_product_class2_and_5_have_no_value_yet` 覆盖 DAR 5/4/6、方法 1/2/3；`test_dcu_698_object_model` 的类/掩码一致性；`test_dcu_698_catalog` 的目录类号一致性；验收用例对 10 个 OAD 断言 DAR 5，拒绝 Data 或空数组。

## verifier 复核（rebase 到含 D2-04 的 main）

- 冲突解决：保留 D2-04 的 6014/6015 行与宏生成的 Class 8 ACL 行，叠加本单 6 行；
  `EDGE_SLAVE_ACL_MAX_OBJECTS` 取 40；对象数 38、ACL 37；驱动测试改为接受 37 个对象、
  拒绝 41 个（上限 +1）。
- 独立门禁：干净构建 + 告警门禁 0、全量 342/342、ASAN 13/13、守卫自测、
  `./test.sh --omnis dcu` 2/2（北向 22 例，含 000023）、`/no-comments` 0 删除。
- 设计文档计数与类号注释同步为 38/37/40。
