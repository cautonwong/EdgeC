# DCU-P3-1-ACL-CAP：消除 Class 20 ACL 每对象属性/方法槽 HOLD

分支 `dcu/P3-1-acl-cap`，工作树 `/tmp/edgec-land/p3-1-acl-cap`，基线 `feat/dcu`
`9d065b6d`。spec：`docs/evidence/DCU-P3-1-ACL-CAP/spec.md`（S1–S5）；设计：
`design.md`；预算实测：`budgets.txt`。

## 目标项（按实际对象表核对）

| 项 | 实际缺口（不是任务单笔误） | 改前 | 改后 |
| --- | --- | --- | --- |
| Class 19 4307/4308/4309 | 方法 127/128/129（表178 子设备） | 不广告，主站 DAR 3 | 只读广告，ACTION DAR 5 |
| Class 21 F100 | 方法 9..12（表184） | 不广告，DAR 3 | 只读广告，ACTION DAR 5 |
| Class 23 2301..2308 | 属性 17/18（表190） | 不广告，DAR 3 | 只读广告，GET DAR 5 |
| Class 18 F001 | 方法 9/10（表A.11；任务描述写“属性”，resolver 实际定义为方法） | 不广告，DAR 3 | 只读广告，ACTION DAR 5 |

4300 无 OI 专属方法、F000/F002 无 9/10：保持不广告（未定义，DAR 3）。
Class 12 2401 的 A16/A17 是标准自身归属冲突，不在本单范围，保持只广告 1..15。

## 实现

- `adapters/io_drivers/src/driver_dlt698.c`：每对象 ACL 描述 16 B → **12 B**
  （oi/n_attrs/n_methods/method_off + 5 B 属性权限，2 bit × 18）；方法号改为
  **会话级 1024 B 池**，单对象最多 12 个；解析累计超池即整单拒绝（fail closed），
  不截断、不越界。`DRIVER_PRIV_SIZE` 保持 4096（会话 3624→3752 B，余 344 B）。
- `cmd/dcu/dcu_698_appconn.c`：Group 2301..2308 广告 A1..A18；4307..4309 广告方法
  1..6+127..129；F100 广告方法 1..12；F001 广告方法 1..10；4300/F000/F002 不变。
- `ci/mutation_guards.tsv`：追加 `DCU-P3-1-acl-cap-method-pool`；同步更新
  `DCU-D4-07`/`DCU-D4-09` 两行的 find/replace（原锚点被本单的新广告表取代，
  不改则守卫 manifest 失效）。`run_guard_selftest.py` 的 mutation probes 只针对
  检查脚本自身，不含产品测试，故本守卫登记在 `mutation_guards.tsv`。

## 红/绿

- 红提交 `b7402a41`（只含测试）：`ctest -R '^(test_dcu_698_object_model|test_dcu_698_appconn|test_driver_dlt698)$'`
  → 退出 8，2/3 失败；原始断言 `3 != 5`（Class 19/21/23/18 四处）、
  `5828 != 6014`（对象列表帧长）。日志 `red.log`。
- 绿：同一命令 3/3 通过；最终 `ctest --test-dir build -R 'dcu|dlt698'`
  **45/45 通过**（127.40 s），日志 `tests.log`；定向日志 `tests-focused.log`。

## DAR 行为（主站，先过 ACL）

| 请求 | 改前 | 改后 |
| --- | --- | --- |
| GET 2301 A17/A18（类定义无来源） | 3 | **5** |
| GET 2301 A19（类未定义） | 3 | 3 |
| ACTION 4307..4309 方法 127..129 | 3 | **5** |
| ACTION 4300 方法 127（未定义/未广告） | 3 | 3 |
| ACTION F100 方法 9..12 | 3 | **5** |
| ACTION F100 方法 13（未定义） | 3 | 3 |
| ACTION F001 方法 9/10 | 3 | **5** |
| ACTION F000/F002 方法 9（未定义） | 3 | 3 |
| SET 2301 A17（只读广告） | 3 | 3（ACL 只读，正确口径） |
| SET 4300/F100/F001 只读属性 | 3 | 3 |

## 预算（详见 budgets.txt）

| 项 | 改前 | 改后 |
| --- | --- | --- |
| `sizeof(dlt698_session_t)`（gdb `dlt698_driver.ctx_size`） | 3624 B | **3752 B** |
| `DRIVER_PRIV_SIZE` / 余量 | 4096 / 472 B | 4096 / **344 B** |
| 每对象 ACL 描述 × 224 | 16 B × 224 = 3584 B | **12 B × 224 = 2688 B** |
| 会话方法池 | 0（对象内 8 槽） | **1024 B**（产品用 296 B） |
| Class 20 列表数据 / 帧 | 5806 / 5828 B | **5992 / 6014 B** |
| `EDGE_SLAVE_AXDR_MAX` 余量（当前 118 项） | 4434 B | **4248 B** |
| 响应池 `BUS_SLAVE_POOL_BUF_SIZE` 帧余量 | 4668 B | **4482 B** |
| 224 项 / 10240 / 4096 三项 D1-21 预算 | — | 均不抬高 |

## 变异守卫

`DCU-P3-1-acl-cap-method-pool`：把 `driver_dlt698.c` 的
`#define DLT698_ACL_MAX_METHODS 12` 改回 8，`CMOCKA_TEST_FILTER=test_slave_class21_esam_dar`
必须转红；恢复后必须转绿。日志 `mutation-guard.log`。

## 对抗评审退回修订（测试假绿）

- 原溢出用例请求 GET `0x0001` 属性 2，但该属性从未被广告：池界放宽到 2× 时整单
  仍被 ACL 判 DAR 3，测试假绿。现用例让第一只对象广告属性 2（na=1），其余 99
  对象不广告属性、各 12 个方法号（共 1200 号 > 1024 B 池）；断言 DAR 3 且
  `resolve_axdr` 调用 0 次。池界放宽变异时整单被接受、resolver 被调用、响应
  DAR 6 → 用例红（`test_driver_dlt698.c:1860`，`red-pool-bound.log`）。
- 新守卫 `DCU-P3-1-acl-pool-bound`：find
  `(size_t)acl->method_used + (size_t)nm > sizeof(acl->method_ids)` → replace
  `2u * sizeof(...)`；`run_mutation_guards.py --only` mutated rc=8 红 / restored
  rc=0 绿，原文件字节与 hash 校验（`mutation-pool-bound.log`）。
- `ctest -R '^test_driver_dlt698$'` 全绿（`tests-driver.log`）。

## 验收

`flock /tmp/edgec-land/acceptance.lock` 下串行执行（撞锁等待，不绕过）：

    TESTPLATFORM=/home/edge/test-platform PYTHON=/home/edge/EdgeC/.venv/bin/python3 \
      EDGEC_BUILD_DIR=$PWD/build ./acceptance/run.sh dcu

退出 0；2/2 验收任务合格：dcu-acceptance 3 passed（8.28 s）、
dcu-northbound 35 passed（21.93 s）。日志 `acceptance-dcu.log`；报告在
`/tmp/edgec-land/p3-1-acl-cap/reports/acceptance`。

## 文档同步（本轮回写）

`DCU_698_OBJECT_MODEL.md` §4.2（每对象 12 B / attr 1..18 / 方法池 1024）、
Class 12/18/19/21/23 行与 §8.6，以及 `DCU_698_DEVIATIONS.md` 的
Class 12/18/19/21/23 行已改为新口径：F001 9/10、4307-09 127-129、F100 9-12、
2301-08 A17/A18 已广告，无来源走 resolver DAR 5；`check_doc_references.py
--root .` 通过。计划 §12.5 旧句仍留给 verifier 合入时改。

## 未决

- 无功能未决；HOLD 目标项外的 producer（无来源）仍按既有口径 DAR 5，不在本单。
- 本分支未合并、未 push。
