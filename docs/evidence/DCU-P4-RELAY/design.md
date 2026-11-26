# P4 继电器 OAD 映射接口设计（DCU 698 OI 8000 方法 129/130/131）

> 目标：产品只有表地址/槽位身份与 DO 通道，没有继电器 OAD；本单给出“继电器 OAD →
> 表地址 + DO 通道”的产品侧映射数据形状与查表接口，空表诚实回 DAR 2，命中后仍不接
> 真实执行（回 DAR 5），不伪造成功。

## 1. 数据形状

新增 `cmd/dcu/dcu_relay_map.{h,c}`，只含固定容量、调用方持有的数据与纯查表函数：

```c
#define DCU_RELAY_MAP_CAPACITY 32

typedef struct {
    uint32_t relay_oad;            /* 表 A.9 的继电器 OAD，映射键 */
    char meter_addr[MAX_ADDR_LEN]; /* 目标表地址（dcu_inventory 的 TSA） */
    uint8_t do_channel;            /* 目标继电器 DO 通道号 */
} dcu_relay_entry_t;

typedef struct {
    dcu_relay_entry_t entries[DCU_RELAY_MAP_CAPACITY];
    size_t count;
} dcu_relay_map_t;
```

- 键是继电器 OAD；同一 OAD 重复填入按覆盖处理，不追加。
- 目标是**表地址 + DO 通道**：地址是 `dcu_node_execute_control` 现有下沉端口
  （`_node_control_sink` → `dcu_node_execute_control`）需要的身份；DO 通道是真实执行
  缺失的下半段。`slot_id` 不作键值，避免把可变的设备下标当身份（F5 的教训）。
- 129/130/131 的差异在命令语义（跳闸/合闸允许/密码合闸），目标字段相同，不另加字段。

## 2. 归属与注入

- `dcu_relay_map_t` 实例是 `dcu_node` 聚合根的成员（零初始化即空表），聚合根拥有数据。
- 新增 `dcu_node_set_relay(node, oad, meter_addr, do_channel)` 作为产品点位表/测试的
  最小注入点；`dcu_node_find_relay(node, oad)` 供未来执行层与测试观察解析结果。
- 聚合根装配时 `dcu_relay_map_bind(&node->relay_map)`，销毁时解绑。对象表是进程级
  单例，与既有 `dcu_698_object_model_set_control_sink` 同一模式；控制类处理器只持有
  模块的查表函数，不自己藏表。
- 不读文件、不读 DB、不做动态分配。

## 3. 查表与 DAR 语义

`dcu_698_control_execute` 在 129/130/131 参数解码后（现有
`dcu_698_decode_action_param` 已给出 `relay_oad`）：

| 条件 | 返回 | DAR | 理由 |
| --- | --- | --- | --- |
| 映射表为空 / OAD 查不到 | `EDGE_SLAVE_SRC_TEMPORARY` | 2 | 现场接线与点位表未给，无法识别目标，诚实拒绝 |
| OAD 命中映射 | `EDGE_SLAVE_SRC_CLASS_MISMATCH` | 5 | 目标已解析，但产品尚未接真实 DO 执行；按本仓库“类定义可广告、实现缺失回 5”的口径，区别于“配置缺失回 2” |
| 127/128（无参数） | 不变 | 2 | 本单不动 |

命中后**不得返回 0**：解析成功不等于执行成功。

## 4. 不改动面

- 8000 的 A2/A4/A5 只读 DAR、对象表行、ACL、`dcu_698_axdr.c` 的参数解码均不变。
- 未新增 foundation 端口；未改 `dcu_router.h`。

## 5. 备选方案

| 方案 | 取舍 |
| --- | --- |
| 表直接藏在 `dcu_698_class_control.c` 的 file-static 数组 | 控制处理器自己藏全局表，违反本单约束；测试无法通过聚合根注入 |
| 经 `dcu_698_object_t.instance` 挂到 8000 对象行 | 需改 `dcu_698_object_model.c`（D5-04 占用，本单禁改） |
| 扩展 `dcu_control_sink_t` 协议带 OAD 查询 | 需改受保护的 `dcu_router.h`，越界 |
| **独立 `dcu_relay_map` 模块 + 聚合根持有 + 模块级绑定（选定）** | 数据归聚合根，查表是纯函数，处理器不藏表，最小注入点；代价是一个进程级绑定指针，与对象表单例模型一致 |
