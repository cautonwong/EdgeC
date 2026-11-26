# DCU-D3-08：Class 9/7/24/8 测试出口与判据矩阵

状态：交 `dcu-verifier` 复核。基线 `feat/dcu 09827718`，分支 `dcu/D3-08-class-tests`。
只加测试，不改产品行为。

## 1. 类 → 用例 → 判据矩阵

### Class 9（5000–5007 冻结，12 例）

| 用例 | 判据 |
| --- | --- |
| test_freeze_boundaries | 跨日/月/结算日/年边界写正确 freeze_time，启动不回补 |
| test_freeze_hour_boundary_uses_local_time | 非整小时时区按本地 tm 取整，UTC 取模会错 |
| test_next_boundary_aligns_to_second | 下一整秒边界对齐 |
| test_freeze_attr3_config_drives_records | 属性 3 增删后记录列/持久化随之变化 |
| test_freeze_attr3_rollback_and_dar | 失败整单回滚；延时>0 入池；未绑定 DAR 2 |
| test_freeze_per_entry_periods | 同 OI 两条目各自边界写 |
| test_freeze_delayed_trigger | 延时到期前不写、到期写一次；0 立即 |
| test_freeze_depth_trim | depth=2 只留最新 2 条，不误删其它源/表 |
| test_freeze_attr3_capacity_dar | 第 9 项 DAR 3 回滚；同帧 OAD 去重；截断 flag |
| test_freeze_pending_retries_after_failure | 写失败保留 pending 并重试 |
| **test_freeze_attr3_get_axdr_via_resolve（新）** | GET 属性 3 编码 array<3 项 structure{lu period, OAD, lu depth}>；A2 回 DAR 5 |
| **test_freeze_method_dar_shapes（新）** | 方法 4 形状错 DAR 7；方法 5 未知 OAD DAR 6；方法 8 非 NULL DAR 7 |

### Class 24（3000–300B 分项事件）

| 用例 | 判据 |
| --- | --- |
| test_event_class24_attrs_on_the_wire | A11 enum / A12 bool 线上类型；A8 未声明回 DAR 5 |
| test_event_class24_complex_attrs | A2 关联 OAD 数组、A3 四表计数、A5 无来源 DAR 5、A14 空记录 NULL、A6 无墙钟 DAR 7 |
| test_event_class24_record_and_status_times | 记录序号/发生/恢复时间线上结构 |
| test_product_accept_set_event_attrs | A11/A12 SET 后回读一致 |
| **test_event_catalog_ois_without_instance_dar6（新）** | 目录有 Class 7 OI（3009/300C/3011/3013）与未实现的 300B：产品无实例，GET 回 DAR 6，不造事件 |

### Class 7（3009.. 事件对象）

产品对象表没有 Class 7 实例（`IC_EVENT=7` 无对象行，事件 OI 全部按 IC_SUB_EVENT=24 建模）。
本单不伪造 Class 7 行为，新增 `test_event_catalog_ois_without_instance_dar6` 钉住现状：
3009/300C/3011/3013 四个目录 OI GET → DAR 6。**缺口**：Class 7 事件对象的属性/记录尚未实现，
需 D3-05 扩展（A2/A3/Record 行为）。

### Class 8（4000–4021 参数，复用 tests/test_dcu_698_param.c，9 例）

| 用例 | 判据 |
| --- | --- |
| test_defaults_and_round_trip_every_row | 每行 SET 后回读一致 |
| test_datetime_attrs_and_ranges | 时间 SET 生效；非法月份/小时 DAR 7 且值不变 |
| test_octet_string_lengths | 长度边界与 DAR 7 |
| test_geo_nesting_and_validation | 嵌套结构形状校验 |
| test_limit_structure_order | 限值结构字段顺序 |
| test_dar_5_4_6_7 | DAR 5/4/6/7 分类（类定义未暴露/未定义/未知 OI/类型错） |
| test_save_failure_leaves_value_unchanged | 保存失败值不变 |
| test_datetime_without_clock_port_reports_temporary | 无时钟端口 DAR 2 |
| test_persistence_reload_and_audit | 重开后值保持并写审计 |

越权 DAR 3 在 ACL/驱动层：验收 `D_00_00_02_000016`（Class 20 ACL）与 SET 越权用例覆盖；
本单不重复造产品内 ACL 单元。

## 2. 验收映射（现有 34 条已覆盖，不新增）

- Class 9：`D_00_00_02_000032`（跨零点 5004/5005）、`D_00_00_02_000033`（属性 3 配置→Record 列）。
- Class 24：`D_00_00_02_000017`（3000 A4 设备级读数）、`D_00_00_02_000011/000012/000013`（3000 A11/A12 SET 回读与 DAR 7）。
- Class 8：`D_00_00_02_000022`（SET 4000 A2 时钟：DAR 0 必真改、失败 DAR 2/3 且时间不变）。

## 3. 门禁

- clean Release warning 0（唯一 warning 为 third-party open62541）。
- `ctest -R dcu` 37/37（其中 freeze 12、axdr 39、param 9）。
- ASAN `detect_leaks=1` 3/3 零泄漏；SRP 干净；guard 110/110。
- 验收 2/2，34 passed / 46.21 s，无 DCU 残留（现有用例已覆盖，无新增 case）。

## 4. 未决项

- Class 7 事件对象（目录 3009..）无产品实例，只有 DAR 6 现状用例；实现与 A2/A3/Record 测试归
  D3-05 扩展。
- Class 8 越权 DAR 3 依赖 ACL/驱动层验收，无产品内单元；如需要可另立驱动级夹具。
