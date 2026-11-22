# DCU-D2-03 注释审计

范围：`4233a7c0..HEAD` 的实现 diff，含新文件 `dcu_task_store.{h,c}`、`tests/test_dcu_698_tasks.c`。

## 删除

26 行、12 处：枚举名复述（status 的 FREE/NORMAL/DISABLED）、成功计数块的守卫复述、章节横幅、测试意图叙述与断言复述、`_skip_axdr` 与两个测试 docstring 的重复描述、D1-20a 依赖讲解（运行时 skip 文本仍保留同一约束）。

## 保留

- 协议布局：A.7 6013 单元、TI `0x54|unit|be16`、date_time_s 零窗口、时段表 4 字节结构、元素索引 = task_id。
- DAR：形状错 DAR 7、产品不支持 DAR 2、任务 0 DAR 3、缺失 DAR 6、类定义未广告 5 / 未定义 4；解码先于 poller/SQL。
- 持久化：RAM 镜像、SQL 先写 RAM 后改的回滚快照、只播种一次、坏行跳过不删库。
- 时钟：周期变化重建时钟、DISABLED→NORMAL 不补拍、槽位不压缩、TI 0 与 C API interval 0 的区别。
- 运行期所有权：`task_op` 放在聚合体（~512B，uv 线程）。

## 说明

- 文档文件 `docs/design/DCU_698_OAD_STATUS.md` 属内容更新，不在注释审计范围。
- 注释改动后重跑了增量构建与 37 项聚焦测试。
