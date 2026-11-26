# DCU-D5-06 Class 17 显示（DAR 诚实骨架）

分支 `dcu/D5-06-class17-display`，工作树 `/tmp/edgec-land/d5-06-class17`，基线 `feat/dcu` `c2eaa5ea`。

## 前提核查

标准 7.3.17 表172/173/174（`docs/698_parsed.md:4738-4801`）给 Class 17 定义属性 1（逻辑名）、
2（显示对象列表）、3（显示时间）、4（显示参数）与方法 1–6（复位、执行、下翻、上翻、显示查看、
全显，全部可选）；A.14（`docs/698_parsed.md:9313-9318`）定义两个实例 F300 自动轮显与 F301 按键轮显。
目录 `docs/design/DCU_698_OI_CATALOG.md` 已按标准给出 F300/F301（Class 17、4 属性、6 方法），
对象模型的类定义上限（`k_class_max_attr` 17→4、`k_class_max_method` 17→6）已与目录一致。

产品没有显示配置来源：没有轮显表、屏序号、显示时间或显示参数的运行期生产者，Class 8 的 4007
LCD 参数也不在 `DCU_698_PARAM_OIS`。广告任何显示值都会是假数据，因此本单只做 DAR 诚实骨架。

## 落地的数据形状

| OI | 接口类 | 范围 | 对象 mask | ACL |
| --- | --- | --- | --- | --- |
| F300 自动轮显 | IC_DISPLAY (17) | 终端级 | 空 | 属性 2..4 只读，方法 1..6 |
| F301 按键轮显 | IC_DISPLAY (17) | 终端级 | 空 | 属性 2..4 只读，方法 1..6 |

`dcu_router.h` 新增 `IC_DISPLAY 17`。对象 mask 为空，请求穿过驱动 ACL 后由对象表回答：

- 类定义、对象未声明的属性 → DAR 5（A1..A4）；类未定义的属性 → DAR 4（A5 起）
- 类定义、对象未声明的方法 → DAR 5（方法 1–6）；类未定义的方法 → DAR 4（方法 7 起）
- 表外 OI → DAR 6（resolver 层）；主站请求先被 ACL 拒 → DAR 3
- ACL 属性权限全部是 1（只读）；SET 无写处理器，主站 SET 回 DAR 3

没有 placeholder 值、空轮显表或返回成功的方法。逻辑名 A1 是静态标识，不放入 ACL；
主站 GET F300 A1 因此先回 DAR 3，resolver 层仍按类定义回 DAR 5（两者分别由单测钉住）。

### F301 一并登记的理由

标准 A.14 把 F300/F301 并列为 Class 17 的两个实例，计划 §7 的类表也写 Class 17 范围为
`F300..F301`、OI 行数 2。两行共用同一空掩码骨架与只读 ACL，额外成本只有对象/ACL 各一行；
只登记 F300 会让标准已定义的按键轮显在产品目录里无对象行，故一并登记。D5-06 的出口证据
仍按计划只要求 GET F300。

## 与计划原始出口的偏差

计划 §7 的功能点位表把 Class 17 的出口写成“GET F300（空轮显表）”，§8 D5-06 的出口写成
“GET F300，方法 DAR=5”。本单没有显示配置来源，按 D5-01/02、D4-02a 与 D5-05 的先例改为
DAR 5 诚实骨架：已定义但未广告的属性回 DAR 5，不返回伪造的空轮显表。计划状态与出口证据
由 verifier 更新；显示配置生产者缺口见下。

计划 §8 的交付物写 `cmd/dcu/dcu_698_display.c`。空掩码骨架没有处理器可落，按 D5-05
（同样未新建 `dcu_698_stat.c`）的先例，复用对象模型与 appconn 表，不新增单用途文件。

## 计数与容量

- 对象表 101 → 103（`dcu_698_object_count()`）。
- Class 20 ACL 87 → 89（78 个手写行 + 11 个 `DCU_698_PARAM_OIS` 生成行）。
- 对象列表编码 89 项 = 3141 B（实测 `dcu_698_appconn_object_list()` 返回 len=3141；
  D5-05 的 87 项为 3011 B，新增两项各 65 B）。
- ACL 会话存储 89 × 16 B = 1424 B + 表头，仍小于 `DRIVER_PRIV_SIZE 4096`；
  对象列表 3141 B 小于 `EDGE_SLAVE_AXDR_MAX 10240`；103 < `EDGE_SLAVE_ACL_MAX_OBJECTS 224`。

## 跟进项（不在本分支实现）

1. **显示配置 producer**：轮显表、屏序号、显示时间与显示参数的运行期来源与持久化；有来源后
   把属性加入对象 mask 与 resolver 编码，ACL 已就绪。
2. **计划状态更新**：D5-06 的出口证据由 verifier 按本单偏差改写为 DAR 5 骨架，并把
   “空轮显表”出口改为待 producer 的缺口记录。

## 验证

- 增量构建 36/36（含 5 个测试目标），第一方 0 warning。
- 红：test commit `49dc087a`（实现 stash 后）`ctest -R 'test_dcu_698_object_model|test_dcu_698_axdr|test_dcu_698_scheme_mgmt|test_driver_dlt698|test_dcu_698_appconn'`：5/5 测试可执行文件失败（0/5 通过），见 `tests.log`。
- 绿：同一 45 项命令 `ctest --test-dir build -R 'dcu|dlt698' --output-on-failure`：45/45 通过（42.79 s）。
- `check_dcu_698_catalog.py`：0；`check_doc_references.py`：0。
- 证据：`tests.log`、`runtime.log`、`diffstat.txt`、`comments.md`。

未合并，等 dcu-verifier 验证。
