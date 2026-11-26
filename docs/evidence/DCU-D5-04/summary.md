# DCU-D5-04：Class 12 脉冲计量只读 DAR 诚实骨架

分支 `dcu/D5-04-class12-partial`，工作树 `/tmp/edgec-land/d5-04-class12`，基线 `feat/dcu`
`197d98b9`（D5-03 已合入）。

## 任务与出口

计划 §8 D5-04 的出口是“GET 2401，方法 DAR=5”。本单按 operator 同意的骨架口径执行：
2401 有对象行与只读 ACL，主站可观察 DAR 诚实，不伪造脉冲量。原出口“GET 2401 返回值”
改为“GET 2401 回 DAR 5 骨架”，计划文字由 verifier 更新；16/17 与 18/19 的限制见下。

## 标准依据与冲突

- `docs/698_parsed.md` 表157（约 4343–4373）：Class 12 属性 1–19、方法 1–4。
- 表158（约 4377–4427）对 16/17 的描述与表157 冲突：表157 写 16 正向无功电能示值、
  17 反向有功电能示值（`:4365-4366`）；表158 写 16 反向有功电能示值、17 正向无功电能示值
  （`:4423-4425`）。
- 按 operator 决定，以类定义表157 为默认；16/17 记 HOLD，等 PDF 所有者确认。
- 2401 是 Class 12 的 OI（计划 §8 D5-04，GET 2401）。

## 落地的数据形状

| OI | 接口类 | 范围 | 对象 attr mask | 对象 method mask | ACL |
| --- | --- | --- | --- | --- | --- |
| 2401 脉冲计量 | IC_PULSE (12) | 终端级 | 空 | 空 | A1..A15 只读（rights 1）；方法 1..4 |

对象行（`dcu_698_object_model.c`，`OI_PULSE_METER`）只广告对象存在性；`instance` 为 NULL，
没有 read/write/method 处理器。ACL（`dcu_698_appconn.c`）是只读子集：16/17 归属冲突
记 HOLD，17..19 超出 ACL 属性 schema（1..16），都不能广告。主站请求先过 ACL，再进对象表。
类定义表（`k_class_max_attr` 的 `{12,19}`、`k_class_max_method` 的 `{12,4}`）原本已按
表157 登记，本单未改。

## DAR 矩阵（resolver 层与主站层）

| 请求 | resolver 层 | 主站层（先过 ACL） |
| --- | --- | --- |
| GET 2401 A1..A15（类定义、无来源） | DAR 5 | DAR 5（ACL 广告） |
| GET 2401 A16/A17（类定义、HOLD） | DAR 5 | DAR 3（ACL 不广告） |
| GET 2401 A18/A19（类定义、超 ACL schema） | DAR 5 | DAR 3（ACL 不广告） |
| GET 2401 A20（类未定义） | DAR 4 | DAR 3（ACL 不广告） |
| GET 未知 OI（0x2402、0x9999） | DAR 6 | DAR 3（ACL 先拒） |
| ACTION 2401 方法 1..4（类定义、无处理器） | DAR 5 | DAR 5（ACL 广告） |
| ACTION 2401 方法 5（类未定义） | DAR 4 | DAR 3（ACL 不广告） |
| SET 2401 A1 | 无写处理器 | DAR 3（ACL 只读） |

没有任何路径返回空数组、空集合或伪造脉冲量；没有方法或 SET 返回成功。GET 走
`dcu_north_resolve_axdr` → `dcu_698_value_for_oad` → `_unadvertised_attr_rc`
（`dcu_698_oi_defines_attr`）；ACTION 走 `dcu_698_check_call` → `_class_defines_method`。

## 计数与容量（D1-21 上限内）

- 对象表：105 → **106** 行。
- Class 20 ACL：91 → **92** 个对象（81 手写 + 11 参数宏生成）。
- 对象列表编码：91 项 3211 B → 92 项 **3336 B**（实测 `len=3336 count_byte=92`）
  < `EDGE_SLAVE_AXDR_MAX 10240`。
- ACL 会话存储：`dlt698_acl_object_t` 16 B × 92 = 1472 B + header；驱动内
  `_Static_assert(sizeof(dlt698_session_t) <= 4096)` 随构建通过。
- ACL 对象数 92 < `EDGE_SLAVE_ACL_MAX_OBJECTS 224`。

## 红先绿后

- 测试提交 `6ae69e78`：新增/更新 5 个测试文件；在 197d98b9 上 5/5 目标测试二进制红
  （原文 `tests.log`）。
- 实现提交 `a105a4cb`：对象行/ACL/宏/设计文档；`ctest -R 'dcu|dlt698'` 45/45 绿。
- 注释审计提交 `05547042`：仅注释重写与补 why，无行为变化；重跑 45/45 绿。

## 验证命令

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DPython3_EXECUTABLE=/home/edge/EdgeC/.venv/bin/python3
cmake --build build --clean-first -j4
ctest --test-dir build -R 'dcu|dlt698' --output-on-failure
python3 ci/scripts/check_build_warnings.py --log <clean-build.log> --root .
python3 ci/scripts/check_build_freshness.py --root . --build build
python3 ci/scripts/check_dcu_698_catalog.py
python3 ci/scripts/check_doc_references.py --root .
```

结果：clean build 0 个第一方 warning（唯一 warning 在 third-party open62541）；
45/45 绿；四个守卫 0。详见 `runtime.log`。

## 未决项

- 16/17 的正/反向归属冲突（表157 vs 表158）仍 HOLD，等 PDF 所有者确认；确认后若要
  广告 16，ACL schema 仍支持（16 在 1..16 内）；17 仍不可广告（>16）。
- 18/19 超出 ACL 属性 schema（1..16），当前只存在于类定义，主站永远先 DAR 3。
- 2402..2408 未注册对象行；本单只做计划出口的 2401。
- 无脉冲量来源（脉冲输入单元/电能示值生产者不在本单范围）；resolver 只回 DAR 5/4。
- 计划 §8 的 D5-04 出口文字仍写“GET 2401”，需 verifier 更新为 DAR 5 骨架并记录
  16/17 与 18/19 的限制。
