# DCU-D6-02 证据：DL/T 698.45 真帧向量

规格：[spec.md](spec.md)。测试：`tests/test_dcu_698_wire_vectors.c`。
所有期望值取自 `docs/698_parsed.md` 的标准原文（行号见下表），不是从实现输出反推。

## 向量 → 标准来源

| 用例 | 向量 | 标准来源（docs/698_parsed.md） | 期望 |
| --- | --- | --- | --- |
| `test_get_normal_single` | SET 4001 A2 = octet-string `123456789012` | H.4.2:10545；表83:2892 | `86 01 02 40 01 02 00 00 00 00` |
| `test_get_normal_single` | GET 4001 A2 | H.3.1:10204（请求）、:10210（响应）；表65:2560、表72:2676 | `85 01 01 40 01 02 00 01 09 06 12 34 56 78 90 12 00 00` |
| `test_get_normal_truncated_is_dropped` | GET 4001 截断（缺 OAD） | 表65:2560 | 丢弃，无应答 |
| `test_get_normal_list_per_item_dar` | GET-NormalList `[4510 A5, F200 A3]` | 表73:2700；表34:1935 | 第一项 Data（MAC），第二项 DAR 5 |
| `test_get_normal_list_count_mismatch_is_dropped` | 个数=2 只带 1 个 OAD | 表66:2575 | 丢弃，无应答 |
| `test_set_success_and_type_error` | SET 4000 A2 = date_time_s 2016-01-20 16:27:11 | H.4.1:10525（请求）、:10533（响应）；表83:2892；表34:1930 | DAR 0 |
| `test_set_success_and_type_error` | SET 4000 A2 用 long-unsigned 代替 date_time_s | 表34:1937 | DAR 7 |
| `test_dar_classes_undefined_class_notfound` | GET 0010 A6（类 1 只定义到 A5） | 表34:1934 | DAR 4 |
| `test_dar_classes_undefined_class_notfound` | GET F200 A3（类 22 定义但未接线） | 表34:1935 | DAR 5 |
| `test_dar_classes_undefined_class_notfound` | GET 1234 A2（对象表无此 OI） | 表34:1936 | DAR 6 |
| `test_dar_unadvertised_attr_denied` | GET 0010 A5（测试 ACL 未广告） | 表34:1933 | DAR 3 |
| `test_action_6012_and_8000` | ACTION 6012 方法 127（新增任务） | 表87:2970、表91:3047、H.5.1:10598、表34:1930 | DAR 0 |
| `test_action_6012_and_8000` | ACTION 8000 方法 129（跳闸，空继电器映射） | 表A.9:8737、表34:1932 | DAR 2 |
| `test_action_6012_truncated_dar7` | ACTION 6012 方法 127 参数截断 | 表34:1937 | DAR 7，任务未应用 |
| `test_get_record_selector1_and_2` | GetRecord OAD 5004 A2，RSD Selector1 | 表67:2588、表30:1809、表74:2716 | 1 行 `06 00 00 00 01` |
| `test_get_record_selector1_and_2` | GetRecord OAD 5004 A2，RSD Selector2 | 表30:1817、表74:2716 | 1 行 `06 00 00 00 01` |
| `test_get_record_truncated_is_dropped` | GetRecord Selector1 缺值/RCSD | 表67:2588 | 丢弃，无应答 |
| `test_report_frame_layout` | REPORT-Notification 体 + 封帧 | 5.1.1:403、5.1.5:583、表94:3109、表95:3124 | `08 01 07 01 30000400 01 12 00 01 00` |
| `test_report_confirm_matching` | REPORT-Response PIID 不匹配 / 匹配 | 表98:3172、表99:3183 | 不匹配忽略；匹配解除 pending |

## 结果

- Release：`ctest -R test_dcu_698_wire_vectors` 通过；全量 `ctest` 382/382 通过。
- ASAN（Debug + `ENABLE_ASAN=ON`）：13/13 通过，无 sanitizer/leak 报告。
- `check_build_warnings.py`：0 new；`git diff --check` 干净。
- 产品源码零改动；未发现需要停工的缺陷。
