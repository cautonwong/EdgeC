# ADR 0014: 698 从站记录型读的取数端口

## 状态

Proposed（2026-11-22，待架构评审）。

## 上下文

D1-15a 已把 GetRecord/RSD 的编解码放进 `foundation/src/protocol/dlt698_codec.c`。驱动现在能把请求解成 `dlt698_record_sel_t` 与 `dlt698_rcsd_t`，也能按表74 逐行编码响应；缺的是"产品怎么把记录行交给驱动"。

标准依据：

- 表74（`docs/698_parsed.md:2716-2740`）：`A-ResultRecord ∷= SEQUENCE { OAD, RCSD, CHOICE { [0] DAR, [1] SEQUENCE OF A-RecordRow } }`，一行是 N 列 `Data`。
- 表76（`:2760-2778`）才是分帧续传（GetResponseNext）；当前驱动没有这条服务。

约束：

- `foundation/include/edge_slave_plane.h` 保持协议中立：`dlt698_record_sel_t` 与 DAR 不得进入公开端口。
- 组件不用动态内存，缓冲由调用方给。
- 单帧记录条数是一字节，上限 255 行。

## 决策

### 1. 端口追加一个记录取数子结构

```c
struct {
    int (*begin)(void *ctx, uint32_t oad, const uint8_t *selection, size_t selection_len,
                 size_t *row_count);
    int (*row)(void *ctx, uint32_t oad, size_t index, uint8_t *row, size_t cap, size_t *row_len);
    int (*end)(void *ctx, uint32_t oad);
} record;
```

- `selection` 是请求里 RSD+RCSD 的原始字节（不含时间标签），由 `dlt698_decode_get_record_request` 的 `consumed` 给出。哪个选择方法、选中什么区间，归产品适配层用 codec 再解，端口不出现 698 类型。
- `row` 写一行的 N 列 `Data`（含各自类型标签），`cap` 由调用方给；返回非 0 或写不下都算失败。
- 三个回调全为 NULL 表示产品没有记录读能力。

### 2. 分页：按绝对行号重查，不保存游标

`begin` 返回总行数，`row(index)` 按绝对下标取行。续页 = 用同一 `selection` 从更大的下标重查。理由：无游标可失效、无动态状态、重复行不可能出现；代价是产品可能要重复定位，记录表规模小且查询在本地存储上，可接受。

驱动一帧最多取 255 行。当前没有 GetResponseNext，超过 255 行的查询只能回 DAR 2（暂时失效），**不得**截断成 255 行让主站误以为完整；分帧续传另立任务。

### 3. 行失败 = 整条结果失败

表74 只有整条 `A-ResultRecord` 的 DAR，没有行级 DAR。某行取不到时，驱动丢掉已写的半页（把 `out->b.len` 恢复到流起点），按 `EDGE_SLAVE_SRC_*` 映射回 `CHOICE [0] DAR`。**不构造行级错误字段**：那会发明标准里不存在的字节。

`begin` 失败同理：整条结果回 DAR。产品返回 `EDGE_SLAVE_SRC_NOT_FOUND` → DAR 6，`UNDEFINED` → 4，其余按 ADR-0013 既有映射。

### 4. DAR 选择

- 端口回调为 NULL（功能缺失）→ DAR 2。
- RSD 选择方法超出 codec 子集 → DAR 2（对象存在，选择方法暂不支持）。
- 产品返回码 → `_dar_for_source_rc`，与 GET/SET 同一套。

## 后果

- DCU 在 D1-07 接上记录存储之前，从站对记录请求回 DAR 2；不再出现"空成功"。
- 端口多三个函数指针，`edge_slave_source_ops_t` 的既有字段顺序不变，产品用指定初始化器，编译不受影响。
- Selector1/2 之外的 RSD 仍返回 DAR 2，与 codec 的 `DLT698_RECORD_UNSUPPORTED_SELECTOR` 对齐。
