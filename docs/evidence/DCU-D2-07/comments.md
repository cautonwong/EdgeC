# Comment Sicko pass（D2-07 切片 1）

删除 14 行注释（13 块），无改写：

- `dcu_driver_adapter.c`：架构叙述（“唯一来源/本文件只做…”），委托调用本身已表达。
- `dcu_node.c`：init 处第一句架构叙述（LOG_WARN 已复述回退）；destroy 处 detach 注释（合同保留在头文件）。
- `dcu_698_rules.c`：种子来源的历史叙述。
- `tests/test_dcu_698_rules.c`：9 处测试叙述/历史/算术复述。

保留：

- `dcu_698_rules.h` 单线程所有权与 detach-before-free 合同：签名无法表达。
- include guard 尾注：相邻头文件格式。

MUST KILL（未采纳，交 verifier 裁决）：`dcu_698_rules.c` 的 `static dcu_rule_cache_t *s_cache` 是模块绑定的调用方内存指针。这是 verifier 决定 B 的指定形状（cache 内嵌 dcu_node、无 mutex），与 D2-06-r 接受的 report store 绑定同型；生命周期由 `dcu_698_rules_detach()` 在 `dcu_node_destroy` 解绑。若改成就实例传参需要把 cache 穿过 `dcu_bus_source`/`dcu_node`/`dcu_698_axdr`/`dcu_scheme_store` 全部调用点，超出本切片授权。

## 切片 2

删除 16 处：601E/601F 段首横幅、测试逐字节向量行内注释与测试叙述、测试 helper 参数复述；1 处改写：`_ensure_frame_column` 的注释改为“旧表已存在时 CREATE TABLE IF NOT EXISTS 不补列，缺 frame 需单独 ALTER”（保留唯一非显然的 why）。保留 8 处：`dcu_698_rules.h` 的线缆/投影合同与 frame 上限来源、`dcu_698_axdr.h` 的 DAR/元素索引公共合同、`dcu_router.h` 的 OI 标准出处。无 MUST KILL。

## 切片 2 评审修复

新增两处短注释：`dcu_rule_unit_t.oad`（内部消费侧 tag）与 `.csd`（主站可见 698 CSD OAD）——两者在相别行不同，是本次评审缺陷的根因，代码本身无法表达。无其它注释变化。

## 切片 3

删除 5 处：`retried` 字段复述、`_on_done` 重试块叙述、return 旁注、两处测试意图叙述。保留 1 处：`dcu_698_rules_retry_with_spare` 声明处的合同（仅 645、只读快照不碰 SQLite）。另恢复被前次改动吞掉的 `_submit` 前置空行。无 MUST KILL。

## 切片 3 端到端补充

`dcu_product.py` 新增：控制文件 opt-in 注释（默认无文件 = 原行为）、645 异常应答构造注释（驱动只认 0x91，C=0xD1 即判失败）。验收用例保留两处 why：docstring 说明台架行为与判据；窗口头部可能在途成功请求的容差注释。无其它注释。

## 切片 3 端到端窗口修复

新增注释：窗口从静止点开始（控制生效后先走完一个轮询周期）与全局计数不作次数判据（快照边界是块中间态，每块表严格配对才是判据）——两者都是测试不能从代码看出的时序 why。
