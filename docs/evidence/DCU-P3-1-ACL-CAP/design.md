# DCU-P3-1-ACL-CAP 设计说明

对应 spec：`docs/evidence/DCU-P3-1-ACL-CAP/spec.md`（S1–S5）。

## 目标与口径

消除 Class 20 ACL 的两个每对象容量上限造成的 HOLD：属性 16 个（2 bit/属性压 32 位）、
方法 8 个。目标项按实际对象表核对后如下（`docs/plans/...-production-plan.md` §12.5）：

| 项 | 实际缺口 | 现状 |
| --- | --- | --- |
| Class 19 OI 4307/4308/4309 | 方法 127/128/129（表178 子设备方法） | 类方法 1..6 占满 8 槽，127..129 不广告，主站回 ACL DAR 3 |
| Class 21 OI F100 | 方法 9..12（表184） | 1..8 广告，9..12 不广告，DAR 3 |
| Class 23 OI 2301..2308 | 属性 17/18（表190） | 1..16 广告，17/18 超属性位宽，DAR 3 |
| Class 18 OI F001 | 方法 9/10（表A.11；任务描述的“属性 9/10”按 resolver 实际定义是方法） | 1..8 广告，9/10 不广告，DAR 3 |

Class 12 OI 2401 的 A16/A17 是标准自身归属冲突（表157 vs 表158），与本上限无关，
保持只广告 A1..A15，不在本单范围。

## 数据结构

驱动侧 ACL（`adapters/io_drivers/src/driver_dlt698.c`）改为“每对象 12 B 描述 +
会话级方法池”，而不是把每对象方法数组从 8 扩到 12、属性位宽从 32 扩到 64：

```c
typedef struct {
    uint16_t oi;
    uint8_t n_attrs;
    uint8_t n_methods;
    uint16_t method_off;                  /* 方法号在会话池中的偏移 */
    uint8_t attr_rights[(18 + 3) / 4];    /* 5 B：属性 1..18，2 bit/属性 */
} dlt698_acl_object_t;                    /* 12 B，原 16 B */

typedef struct {
    bool loaded;
    uint16_t count;
    uint16_t method_used;
    dlt698_acl_object_t objects[224];
    uint8_t method_ids[1024];             /* 会话级方法号池 */
} dlt698_acl_t;
```

- `DLT698_ACL_MAX_ATTRS` 16 → 18（Class 23 A18 的上界；5 B 恰好覆盖 1..18）；
- `DLT698_ACL_MAX_METHODS` 8 → 12（每对象解析上限，Class 21 表184 方法 1..12）；
- 方法号 127..134 仍是 `uint8_t`，池化不影响号段；
- 池满即 `_acl_parse` 失败、ACL 不可用（fail closed，既有口径：未加载 ACL 一律 DAR 3），
  不会截断或越界。

为什么不选“每对象 24 B（uint64 属性位 + method_ids[12]）”：那会把
`sizeof(dlt698_session_t)` 抬到约 5416 B，必须把 `DRIVER_PRIV_SIZE` 从 4096 提到 6144；
仓库内 `test_dlt698_appconn.c`、`test_dcu_698_axdr.c`、`test_dcu_698_record.c` 等
多处会话缓冲是按 D1-21 的 4096 硬编码的，`RELEASE` 的 `memset(s, 0, sizeof(*s))`
会写越界；本单文件边界不允许改这些测试。池化后会话反而比现状更省：

| 预算 | 现状 | 本单后 | 余量 |
| --- | --- | --- | --- |
| 每对象 ACL 描述 | 16 B × 224 = 3584 B | 12 B × 224 = 2688 B | — |
| 会话方法池 | 0（对象内 method_ids[8]） | 1024 B（产品实际用 296 B） | 728 B |
| `sizeof(dlt698_session_t)` | 3624 B | 3752 B（预期，实测见 budgets.txt） | 344 B（对 `DRIVER_PRIV_SIZE`） |
| `DRIVER_PRIV_SIZE` | 4096 B | 4096 B（不变） | — |

## 广告侧改动（cmd/dcu/dcu_698_appconn.c）

- `k_attrs_group` 增 A17/A18，2301..2308 的 `n_attrs` 16 → 18（只读）；
- 新增 `k_methods_devmgr_sub = {1..6,127,128,129}` 给 4307..4309（9 项）；
  4300 只定义类方法 1..6，保持 6 项，不广告 127..129；
- `k_methods_esam` 增 9..12，`n_methods` 8 → 12；
- 新增 `k_methods_file_chunk = {1..10}` 给 F001；F000/F002 保持 1..8
  （resolver 只为 F001 定义 9/10）。

访问语义不变：主站请求先过 ACL；进 ACL 后由 resolver 如实回 DAR
（GET/ACTION 类定义无来源 → DAR 5，SET 只读广告 → DAR 3）。未进 ACL 的请求仍是 DAR 3。

## 预算变化（预期，实测见 budgets.txt）

- 对象列表 A-XDR：5806 B → 5992 B（+31 项 × 6 B = +186 B）；加 22 B 帧 5828 B → 6014 B。
- `EDGE_SLAVE_AXDR_MAX` 10240 B：对当前 118 项，余量 4434 B → 4248 B；对 D1-21 的
  224 项投影 7354 B，余量 2886 B 不变。
- 从站响应池 `BUS_SLAVE_POOL_BUF_SIZE = EDGE_SLAVE_AXDR_MAX + 256 = 10496 B` × 32：
  对 6014 B 帧余量 4482 B，池总量不变（约 328 KB）。
- `DRIVER_PRIV_SIZE` 4096 B 不变，会话余量 472 B → 344 B；`driver_manager_storage_t.raw`
  （147456 B）与 north adapter storage 不变。

## 回归风险

- 方法池是所有对象共享的：解析器必须在追加前做容量检查，否则池满会写越界（fail closed 已覆盖）。
- 属性权限按 2 bit/属性跨字节读取/写入，17/18 号落在第 5 字节；单测覆盖 A17/A18 的读权限与
  只读广告（SET 仍 DAR 3）。
- 224 项 D1-21 长格式 ACL 测试（每项 1 属性、0 方法）必须继续可解析：12 B 描述 + 方法池
  仍在 4096 B 内。
- 对象列表长度变化会命中 `test_slave_object_list_fits_production_slave_pool` 的精确字节断言；
  该断言随广告项同步更新，不接受“只改断言不验证语义”。
