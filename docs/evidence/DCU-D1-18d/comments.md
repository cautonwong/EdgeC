# DCU-D1-18d 注释审计

范围：`13230256` 到工作树的 diff。

## 删除

`dcu_heartbeat.c`、`tests/test_dcu_advanced_modules.c`、`tests/test_dcu_router.c` 共删除 14 行注释，重写 1 行，新增 2 行约束说明，净 −12 行。

- `dcu_heartbeat.c`：`// [8xxx Protocol Verification] 随机触发跳闸控制，验证控制闭环`——叙述演示行为，且已不成立（不再是 8xxx 协议路径）。
- `test_dcu_advanced_modules.c`：4 条 `=====` 分隔条、`// 篡改后的 MAC 应拒绝通过`、`// GET 搜表状态，应为 IDLE (0)`、`/* OI F100 方法 1 = 复位… */`——复述断言。
- `test_dcu_router.c`：6 行叙述“以前 ret == 0，现在断言 -4”，结论已过期且与当前断言不符。

## 改写

- `test_dcu_advanced_modules.c`：“F200 的 A2/A3 不是标准属性”被类定义（Class 22 属性 1–4）与标准（A2 设备对象列表）否定，改为“A2/A3/A4 类已定义但未广告（DAR 5）”。
- `test_dcu_router.c`：约束改为“方法 1 是 Class 13 的复位，产品不广告，类已定义但未广告 → DAR 5；绝不拿演示跳合闸冒充复位”。

## 保留

- `dcu_698_object_internal.h:38`：私有入口不是 OI 8000 标准方法。
- `dcu_698_object_model.c:207`：类方法号上限，OCI 127+ 另算。
- `dcu_heartbeat.c:64`：奇偶周期跳闸/恢复含义。
- `test_dcu_698_axdr.c:162`：2400 的 double-long-unsigned 编码。
- `test_dcu_698_axdr.c:163`、`test_dcu_698_object_model.c:186`、`test_dcu_advanced_modules.c:157`：F200 没有真实串口重配路径，不对外。
- `test_dcu_advanced_modules.c:131`：鉴权是安全模块能力，OI F100 方法 1 是标准复位，不是鉴权。
- `foundation`/对象表侧的 DAR 语义注释未动。

## 说明

- 章节标题（`// 4. 验证下行自动搜表机制…`、`// 9. 验证影子抓拍引擎…`）不删：不作为“复述代码”，删除会让后续章节编号悬空。
- 注释改动不含代码；审计后重跑了增量构建、全量 ctest、ASAN、守卫与验收。
