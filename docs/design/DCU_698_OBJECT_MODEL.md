# DCU DL/T 698.45 对象模型设计

> 日期：2026-11-21　状态：草案，待评审
> 读者：架构、C 开发、协议与驱动、验证
> 关联：`docs/plans/2026-11-21-dcu-698-production-plan.md`、`docs/design/DCU_698_OI_CATALOG.md`、`docs/698.pdf`、`docs/698_parsed.md`

## 1. 目的与口径

本文规定 `cmd/dcu` 的 698.45 对象模型，覆盖标准 7.3 节的 26 个接口类。对象模型是“哪个 OI 支持哪些属性、哪些方法、由谁实现、存在哪里、谁能访问”的唯一来源。北向的 GET、SET、ACTION、Record、REPORT 五条路径都从这张模型取解释，不在各自文件里再写一遍。

一致性口径：

- 标准原文以 `docs/698.pdf` 为准。类定义在 7.3 节，对象标识在 7.4 节与附录 A。`docs/698_parsed.md` 是抽取件，OCR 错误一律回 PDF 纠正，不以抽取件为最终依据。
- `docs/design/DCU_698_OI_CATALOG.md` 由 `scripts/extract_dcu_698_catalog.py` 从原文生成，是属性、方法与 OI 的完整清单。
- 原文解析存在 OCR 列错位，尤其 3xxx 与 6xxx 范围。目录把“原文 IC 列”和“范围推断类”并列，二者不一致的行在进入代码前必须由 D1-01 逐行核对 7.3 与附录 A。
- 本文不猜报文格式。属性与方法的参数结构以原文表为准，编码以附录 H 的字节为准。

## 2. 术语与寻址

| 术语 | 含义 | 代码表达 |
| --- | --- | --- |
| IC | 接口类，类标识码 `class_id` 取值 0 到 255 | `uint8_t class_id` |
| OI | 对象标识，接口类实例的唯一逻辑名，2 字节 | `uint16_t oi` |
| OAD | 对象属性描述符，4 字节，OI 加属性标识与元素索引 | `uint32_t oad` |
| OMD | 对象方法描述符，4 字节，OI 加方法标识与操作模式 | `uint32_t omd` |
| 属性号 | 类定义表中的属性编号 | `uint8_t attr_id` |
| 元素索引 | 数组属性的元素下标，0 表示整体或第一元素 | `uint8_t elem_idx` |
| 方法号 | 类定义表中的方法编号，如 127 表示 Add | `uint8_t method_id` |
| DAR | 数据访问结果，0 成功，4 对象未定义，5 对象接口类不符合，6 对象不存在，7 类型不匹配，2 暂时失效等 | `uint8_t dar` |
| 质量 | good、bad、uncertain 三态，不接受厂商自定义码 | `uint8_t quality` |

OAD 的拆包规则固定为 `oi = oad >> 16`、`attr_id = (oad >> 8) & 0xFF`、`elem_idx = oad & 0xFF`。OMD 的拆包规则固定为 `oi = omd >> 16`、`method_id = (omd >> 8) & 0xFF`、`mode = omd & 0xFF`。拆包只在一个函数里做，其他模块不重复位运算。

## 3. 源与生成

`scripts/extract_dcu_698_catalog.py` 做三件事：

1. 从 `docs/698_parsed.md` 的 7.3 节抽取 26 个类的属性与方法。
2. 从 7.4 节与附录 A 抽取 OI 行，当前得到 513 行。
3. 生成 `docs/design/DCU_698_OI_CATALOG.md` 与 `.json`，并提供 `--check` 模式检测过期。

脚本的断言：

- 类数必须等于 26。
- OI 行数不得少于 400。
- `--check` 发现生成物与原文不一致时退出非零。

构建与 CI 的运行方式：

```bash
python3 -I scripts/extract_dcu_698_catalog.py --check
```

目录不是运行期输入。运行期只读 `const` 静态表。目录的用途是生成静态表、生成测试向量、评审时核对覆盖。

## 4. C 数据模型

### 4.1 对象表

```c
typedef enum {
    DCU_OBJ_SHARED,     /* 全设备一个实例，例如 6000 档案表 */
    DCU_OBJ_PER_METER,  /* 每块表一个实例，例如 2000 电压 */
    DCU_OBJ_SINGLETON   /* 全设备唯一，例如 4000 日期时间 */
} dcu_obj_scope_t;

typedef struct dcu_698_object_s dcu_698_object_t;

struct dcu_698_object_s {
    uint16_t          oi;
    uint8_t           class_id;
    dcu_obj_scope_t   scope;
    dcu_attr_read_fn  read;       /* NULL 表示属性不可读 */
    dcu_attr_write_fn write;      /* NULL 表示只读 */
    dcu_method_fn     method;     /* NULL 表示无方法 */
    const void       *instance;   /* 聚合根或实体指针 */
    uint8_t           attr_mask[32];   /* 属性号 0..255 位图 */
    uint8_t           method_mask[32]; /* 方法号 0..255 位图 */
};
```

`attr_mask` 与 `method_mask` 是 256 位位图，不是 16 位掩码。698 的方法号有 127 到 134，Class 23 的属性号到 18，16 位放不下。用 16 位再让门面绕过 mask，是把根因留成 workaround，禁止。

```c
static inline bool dcu_mask_test(const uint8_t mask[32], uint8_t id) {
    return (mask[id >> 3] >> (id & 7)) & 1u;
}
static inline void dcu_mask_set(uint8_t mask[32], uint8_t id) {
    mask[id >> 3] |= (uint8_t)(1u << (id & 7));
}
```

`attr_mask` 与 `method_mask` 让“不支持”变成显式事实。缺属性或方法返回 DAR=5（对象接口类不符合），缺 OI 返回 DAR=4（对象未定义），按表实例不存在返回 DAR=6（对象不存在），不静默成功。门面的 `get_attr/set_attr/action` 必须走 `dcu_698_read/write/call` 的同一套 mask 预检，不得绕过。

### 4.2 处理器签名

标准 7.3 的属性有标量，也有数组和结构。处理器统一走 A-XDR，标量只是 A-XDR 的一种。缓冲由调用方提供，不引入动态内存。

```c
typedef struct {
    uint8_t *buf;   /* 调用方提供的缓冲 */
    size_t   cap;
    size_t   len;   /* 返回时写入实际长度 */
} dcu_axdr_buf_t;

typedef int (*dcu_attr_read_fn)(const dcu_698_object_t *obj, uint8_t attr_id,
                                uint8_t elem_idx, dcu_axdr_buf_t *out);
typedef int (*dcu_attr_write_fn)(const dcu_698_object_t *obj, uint8_t attr_id,
                                 uint8_t elem_idx, const uint8_t *axdr, size_t len);
typedef int (*dcu_method_fn)(const dcu_698_object_t *obj, uint8_t method_id,
                             const uint8_t *param, size_t param_len,
                             dcu_axdr_buf_t *result);
```

`dcu_value_t` 是标量便捷结构，供 Class 1 到 6 的处理器内部使用，不是端口契约。类型标签由对象模型给，驱动不猜。

```c
typedef struct {
    uint8_t  type_tag;   /* 698 A-XDR 类型标签，例如 0x12 long-unsigned */
    int64_t  raw;
    double   value;
    uint64_t ts_ms;
    uint8_t  quality;    /* 0 good, 1 uncertain, 2 bad */
} dcu_value_t;
```

### 4.3 查表

```c
const dcu_698_object_t *dcu_698_find(uint16_t oi);
int dcu_698_read(uint32_t oad, dcu_axdr_buf_t *out);
int dcu_698_write(uint32_t oad, const uint8_t *axdr, size_t len);
int dcu_698_call(uint32_t omd, const uint8_t *param, size_t param_len,
                 dcu_axdr_buf_t *result);
```

`dcu_698_find` 用一次线性扫描或编译期排序后的二分。首版 OI 约 120 个，线性扫描足够。表是 `const`，查表无锁。

### 4.4 类处理器

处理器按类分组，不按 OI 分组。这样新增一个同类的 OI 只加一行表，不写新代码。

| 类组 | 处理器 | 覆盖 |
| --- | --- | --- |
| 变量类 | `dcu_698_var_read` | 1、2、3、4、5、6 |
| 事件类 | `dcu_698_event_*` | 7、24 |
| 参数类 | `dcu_698_param_*` | 8 |
| 冻结类 | `dcu_698_freeze_*` | 9 |
| 采集监控 | `dcu_698_acq_*` | 10 |
| 集合类 | `dcu_698_set_*` | 11 |
| 脉冲计量 | `dcu_698_pulse_*` | 12 |
| 负荷控制 | `dcu_698_ctrl_*` | 13 |
| 统计类 | `dcu_698_stat_*` | 14、15、16 |
| 显示类 | `dcu_698_display_*` | 17 |
| 文件传输 | `dcu_698_file_*` | 18 |
| 设备管理 | `dcu_698_devmgr_*` | 19 |
| 应用连接 | `dcu_698_appconn_*` | 20 |
| ESAM | `dcu_698_esam_*` | 21 |
| 输入输出 | `dcu_698_io_*` | 22 |
| 总加组 | `dcu_698_group_*` | 23 |
| 通信类 | `dcu_698_comm_*` | 25、26 |

### 4.5 从站端口的数据形状

标准本身没有模糊点。7.3 节给每个类的属性定义，附录 A 给每个 OI 的实例结构，附录 H 给字节样例。模糊点在现有代码的端口形状。

现状是 `foundation/include/edge_slave_plane.h:36` 的 `edge_point_value_t` 只带一个标量：`value`、`raw`、`width`、`type`、`quality`、`ts_ms`。`resolve` 回调只能返回这一个标量。698 的 Class 7、9、10、20 属性是数组或结构，Class 8 的参数也是结构。用标量端口传这些属性，只有两个结果：截断，或者由驱动猜结构。两者都错。

同一处还有第二个缺口。`driver_dlt698.c:223` 的从站交换是 `(void)session; /* 无跨帧状态 */`，只处理 `GET_REQ` 与 `ACTION_REQ`，没有 `SET_REQ` 分支，也没有 GetRecord、RSD 与 Report 路径。端口上的 `accept` 只收一个 `double`，收不了结构参数。

因此对象模型与从站端口之间必须有一条 A-XDR 通道。类型形状见 4.2 节的 `dcu_axdr_buf_t` 与处理器签名。

对象模型的 `read` 处理器把完整 A-XDR 写进 `out->buf`，包括类型标签。标量类写一个标量，Class 10 写 `array structure`，Class 9 写 `array structure{冻结时间, 数据}`。`dcu_value_t` 保留为标量便捷结构，不是端口契约。

端口新增两个可选回调，形状与现有 `accept_action` 一致。字段签名、回落语义与兼容性结论见 `docs/adr/0013-698-slave-axdr-port.md`。

```c
int (*resolve_axdr)(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
int (*accept_axdr)(void *ctx, uint32_t oad, const uint8_t *axdr, size_t len);
```

驱动对 698 的 GET 先调 `resolve_axdr`，回调为 NULL 时才回落到标量 `resolve`；SET 先调 `accept_axdr`，回调为 NULL 时才回落到标量 `accept`。回调已实现但返回非 0 是真实失败，不得再回落——否则结构属性会在一次解码错误后悄悄退化成截断值。这样 IEC104 与 Modbus 的标量路径不变，698 的结构路径有独立通道，驱动仍然不认识点位模型。

单条属性的 A-XDR 上限设为 `EDGE_SLAVE_AXDR_MAX`，首版 2048 字节。超过上限的记录型属性走 GetRecord 分页，不把整张表塞进一个响应。缓冲由调用方提供，不引入动态内存。

## 5. 编码与访问

### 5.1 类型标签

常用 A-XDR 类型标签固定为：

| 类型 | 标签 | 宽度 |
| --- | --- | --- |
| null | 0x00 | 0 |
| array | 0x01 | 变长 |
| structure | 0x02 | 变长 |
| bool | 0x03 | 1 |
| bit-string | 0x04 | 变长 |
| double-long | 0x05 | 4 |
| double-long-unsigned | 0x06 | 4 |
| octet-string | 0x09 | 变长 |
| visible-string | 0x0A | 变长 |
| long64 | 0x14 | 8 |
| long64-unsigned | 0x15 | 8 |
| enum | 0x16 | 1 |
| float32 | 0x17 | 4 |
| float64 | 0x18 | 8 |
| date_time_s | 0x1C | 7 |
| long-unsigned | 0x12 | 2 |
| long | 0x10 | 2 |
| unsigned | 0x11 | 1 |
| integer | 0x0F | 1 |

完整表以 `docs/698.pdf` 6.3 节为准，`docs/698_parsed.md` 只作检索。标签只在编解码器与对象模型出现，业务层用 `dcu_value_t`。

### 5.2 读写权限

权限来自三处，按顺序判定：

1. 对象表里的 `read` 与 `write` 是否为空。
2. Class 20 应用连接对象的“对象列表”给出的属性访问权限与方法访问权限。
3. 当前会话的认证级别，来自 Class 21 ESAM 协商结果。

写操作必须同时满足三处。只读属性即使有 `write` 处理器也不得写。Class 20 未授权的对象返回 DAR=3，不返回成功。

### 5.3 DAR 映射

| 情况 | DAR |
| --- | --- |
| 成功 | 0 |
| 硬件故障 | 1 |
| 暂时失效，例如目标表离线 | 2 |
| 读写被拒绝，例如权限不足或闭锁 | 3 |
| 对象未定义，例如 OI 不在对象表 | 4 |
| 对象接口类不符合，例如属性或方法未实现 | 5 |
| 对象不存在，例如按表实例不存在 | 6 |
| 类型不匹配 | 7，并记录具体原因到日志 |
| 越界 | 8 |
| 密码错或未授权 | 15 |
| 资源不足，例如记录表满 | 2 |

错误不静默。北向出错时回明确的 DAR，主站才不会只看到超时。驱动按 `edge_slave_plane.h` 的 `edge_slave_src_rc_t` 映射本表（含 `EDGE_SLAVE_SRC_DENIED` → 3）；旧回调返回 `-1` 按 6（对象不存在）处理。

## 6. 类逐项规定

属性与方法编号以标准 7.3 节为准。完整清单见 `docs/design/DCU_698_OI_CATALOG.md`。下表给出 DCU 的实现等级与落点。

等级定义：

- `full` 表示首版必须实现读写或方法。
- `read` 表示首版只读，写返回 DAR=3。
- `control` 表示只通过 Class 20 授权后执行，写与方法都要鉴权。
- `optional` 表示首版可只读或返回 DAR=5，按项目规范再开。

### Class 1 电能量接口类

实例数 0…n。属性：1 逻辑名（static）、2 总及费率电能量数组（dyn.）、3 换算及单位（static）、4 高精度总及费率电能量数组（dyn.）、5 高精度换算及单位（static）。方法：1 复位（可选）、2 执行（可选）。

DCU 等级 `read`。数据源是 DataHub 影子与冻结源，落点是 `dcu_698_var_read`。冻结时以本类属性 2 或 4 为源。

### Class 2 最大需量接口类

实例数 0…n。属性：1 逻辑名、2 总及费率最大需量数组、3 换算及单位。方法：1 复位、2 执行。

DCU 等级 `optional`。首版只读。最大需量值来自电表，DCU 不本地重算。

### Class 3 分相变量接口类

实例数 0…n。属性：1 逻辑名、2 分相数值组、3 换算及单位。方法：1 复位、2 执行。

DCU 等级 `read`。电压、电流、相角、波形失真度走本类。数据源是 DataHub 影子，落点 `dcu_698_var_read`。

### Class 4 功率接口类

实例数 0…n。属性：1 逻辑名、2 总及分相数值组、3 换算及单位。方法：1 复位、2 执行。

DCU 等级 `read`。有功、无功、视在功率与功率因数走本类。

### Class 5 谐波变量接口类

实例数 0…n。属性：1 逻辑名、2 A 相 n 次数值组、3 B 相 n 次数值组、4 C 相 n 次数值组、5 谐波次数 n、6 换算及单位。方法：1 复位、2 执行。

DCU 等级 `read`。`dcu_power_quality` 计算 2 到 31 次谐波与 THD，结果写入影子，再由本类读出。

### Class 6 数据变量接口类

实例数 0…n。属性：1 逻辑名、2 数值、3 换算及单位。方法：1 复位、2 执行。

DCU 等级 `read`。频率、温度、电池电压、状态字、需量、费率等非分相变量走本类。

### Class 7 事件对象接口类

实例数 0…n。属性：1 逻辑名、2 事件记录表、3 关联对象属性表、4 当前记录数、5 最大记录数、6 配置参数、7 当前值记录表、8 上报标识、9 有效标识、10 时间状态记录表。方法：1 复位、2 执行、4 添加一个事件关联对象属性、5 删除一个事件关联对象属性。

DCU 等级 `full`。事件记录落 `dcu_event_record_t`，存储走 `edge_event` 的端口。排序按事件发生时间倒序。上报标识与 601D 方案联动。

### Class 8 参数变量接口类

实例数 0…n。属性：1 逻辑名、2 参数。方法：1 复位、2 执行。

DCU 等级 `full`，按 OI 白名单。首版实现日期时间、通信地址、表号、客户编号、设备地理位置、互感器变比、限值等需要现场配置的参数。写操作走 Class 20 授权与审计。

已落地的白名单与形状（`cmd/dcu/dcu_698_param.c`）：

| OI | 属性 | A-XDR 形状 |
| --- | --- | --- |
| 4000 | 2 | date_time_s |
| 4000 | 3 | enum 校时模式（0/1/2/255） |
| 4000 | 4 | structure，5 × unsigned |
| 4001/4002/4003 | 2 | octet-string，最多 31 字节 |
| 4004 | 2 | structure{经度, 纬度, double-long-unsigned 高度}，经纬度各为 {enum 方位, unsigned 度/分/秒} |
| 401C/401D | 2 | double-long-unsigned，无换算 |
| 401E/4020 | 2 | structure，2 × double-long-unsigned，换算 −2 |
| 401F/4021 | 2 | structure，3 × double-long-unsigned，换算 −2，字段顺序按 A.5 |

4000 A2 的 GET/SET 走时钟端口 `dcu_clock_port_t`（`cmd/dcu/dcu_clock_port.c` 的默认实现读 `CLOCK_REALTIME`、写 `clock_settime`，在 `dcu_node.c` 组合根注入）。SET 只在端口返回 0 时回 DAR 0，GET 随后读到新时间；端口缺失或拒绝（EPERM/EINVAL）回 DAR 2，时间保持不变，不做影子写入。4000 A3/A4 与 4001 到 4021 的值写 `dcu_params`（主键 `(oi, attr)`）并在 `dcu_param_audit` 记 old/new 审计。

对象表只广告属性 2（生成的目录把 Class 8 上限钉在属性 2），4000 的 A3/A4 由参数模块在通用 mask 检查之前处理。产品对象表共 30 个 OI（0xF100 ESAM 不在 Class 20 ACL 里），Class 20 ACL 共 29 个对象，超过驱动原来的 28 对象上限；改用每属性 2 bit 的 16 字节紧凑对象后，`DLT698_ACL_MAX_OBJECTS` 抬到 30，属性 id 按 1..16 编码、越界即拒绝（fail-closed），`_Static_assert(sizeof(dlt698_session_t) <= 1024)` 守住会话私有不超 `DRIVER_PRIV_SIZE`。

方法 127 广播校时保持关闭：驱动的 ACTION 统一走 `dcu_north_adapter.c` 的 `accept_action`，该文件由 D1-20a 持有。

### Class 9 冻结数据接口类

实例数 0…n。属性：1 逻辑名、2 冻结数据表、3 关联对象属性表。方法：1 复位、2 执行、3 触发一次冻结、4 添加一个冻结对象属性、5 删除一个冻结对象属性、7 批量添加冻结对象属性、8 清除关联对象属性表。

DCU 等级 `full`。覆盖 5000 到 5011 共 13 个 OI。冻结记录落 `dcu_freeze_record_t`，查询走 `dcu_storage_query_freeze`，RSD 解析在 `dcu_698_freeze_read`。方法 3 触发一次冻结。

记录型对象的表计寻址：OAD 的元素索引字节（`oad & 0xFF`）选档案槽位，0 到 `count-1`；越界回 DAR 6。Class 20 的对象列表只做访问控制，不承担表计寻址。GetRecord 的行列由 RCSD 选列，时间范围由 RSD Selector1/2 选行。

### Class 10 采集监控接口类

实例数 0…n。属性：1 逻辑名、2 配置表（static）、3 记录表（dyn.）。方法：1 复位、2 执行、3 清空记录表。

DCU 等级 `full`。这是集中器的核心。覆盖 6000 到 6042 共 26 个 OI，包括档案、搜表、任务、普通采集方案、事件采集方案、透明方案、上报方案、采集规则、采集状态、任务监控与三个时标。配置表落 `dcu_acq_plan_t` 与档案仓储，记录表落冻结与状态表。方法 127 到 134 的 Add、AddBatch、Update、Delete、Clear 在 `dcu_698_acq_method` 实现，参数按附录 A.7 解。

### Class 11 集合接口类

实例数 0…n。属性：1 逻辑名、2 集合、3 当前元素个数、4 最大元素个数。方法：1 复位、2 执行。

DCU 等级 `read`。7000 文件集合与 7010 脚本集合按只读处理。集合元素是 OAD 列表，读取时按对象表逐个取值。

### Class 12 脉冲计量接口类

实例数 0…n。属性：1 逻辑名、2 通信地址、3 互感器倍率、4 脉冲配置、5 有功功率、6 无功功率、7 当日正向有功电量、8 当月正向有功电量、9 当日反向有功电量、10 当月反向有功电量、11 当日正向无功电量、12 当月正向无功电量、13 当日反向无功电量、14 当月反向无功电量、15 正向有功电能示值、16 正向无功电能示值、17 反向有功电能示值、18 反向无功电能示值、19 换算及单位。方法：1 复位、2 执行、3 添加脉冲输入单元（必选）、4 删除脉冲输入单元（必选）。

DCU 等级 `optional`。2401 到 2408 八个脉冲计量对象。首版只读属性，方法返回 DAR=5，等硬件与项目规范确认后再开。

### Class 13 负荷控制对象接口类

实例数 0…n。属性：1 逻辑名、2 控制方案集、3 控制投入状态、4 控制输出状态、5 越限告警状态。方法：1 复位、2 执行、3 添加控制单元（必选）、4 删除控制单元（必选）、5 更新控制单元（必选）、6 控制投入（必选）、7 控制解除（必选）。

DCU 等级 `control`。8000、8103、8104、8105、8106、8107、8108、810C、8110 等 OI 走本类。控制方法必须过 `edge_control` 的选择执行、闭锁与审计。参数按附录 A.9 解。

### Class 14 区间统计接口类

实例数 0…n。属性：1 逻辑名、2 统计结果表、3 关联对象属性表。方法：1 复位、2 执行、3 添加一个统计对象属性、4 删除一个统计对象属性。

DCU 等级 `read`。2100 到 2104 五个区间统计对象。统计值来自本地流分析或电表。

### Class 15 累加平均接口类

实例数 0…n。属性：1 逻辑名、2 运算结果、3 关联对象属性表。方法：1 复位、2 执行、3 添加一个关联对象属性、4 删除一个关联对象属性。

DCU 等级 `read`。2110 到 2114 五个平均对象。

### Class 16 极值工具接口类

实例数 0…n。属性：1 逻辑名、2 极值结果表、3 关联对象属性表。方法：1 复位、2 执行、3 添加一个关联对象属性、4 删除一个关联对象属性。

DCU 等级 `read`。2120 到 2124 五个极值对象。

### Class 17 显示接口类

实例数 0…n。属性：1 逻辑名、2 显示对象列表、3 显示时间、4 显示参数。方法：1 复位、2 执行、3 下翻、4 上翻、5 显示查看、6 全显。

DCU 等级 `optional`。F300 自动轮显与 F301 按键轮显。首版只读配置，方法返回 DAR=5。WebUI 存在时，显示对象可以指向 WebUI 页面，但协议侧仍按标准返回。

### Class 18 文件传输接口类

实例数 0…n。属性：1 逻辑名、2 文件信息、3 命令结果。方法：1 复位、2 执行、3 删除、4 校验、5 代发、6 代收、7 上传、8 下载。

DCU 等级 `full`。F000、F001、F002 三个文件传输管理对象。DFU 引擎已有分块传输与校验，接入本类。上传与下载必须校验签名与版本，失败回滚。

### Class 19 设备管理接口类

实例数 0…n。属性：1 逻辑名、2 设备描述符、3 版本信息、4 生产日期、5 子设备列表、6 支持规约列表、7 允许跟随上报、8 允许主动上报、9 允许与主站通话、10 上报通道。方法：1 复位（必选）、2 执行、3 数据初始化（必选）、4 恢复出厂参数（必选）、5 事件初始化（必选）、6 需量初始化（可选）。

DCU 等级 `full`。4300、4307、4308、4309 等设备管理 OI。方法是设备级动作，必须二次确认并写审计。恢复出厂参数保留参数里的 OAD 列表。

### Class 20 应用连接接口类

实例数 0…n。属性：1 逻辑名、2 对象列表、3 应用语境信息、4 当前连接的客户机地址、5 身份验证机制。方法：1 复位、2 执行。

DCU 等级 `full`。4400 一个应用连接对象。对象列表是访问控制的事实来源，属性访问权限分不可访问、只读、只写、可读写四档，方法访问权限是布尔。北向每次请求先查本对象。

### Class 21 ESAM 接口类

实例数 0…n。属性：1 逻辑名、2 ESAM 序列号、3 ESAM 版本号、4 对称密钥版本、5 会话时效门限、6 会话时效剩余时间、7 当前计数器、8 证书版本、9 终端证书序列号、10 终端证书、11 主站证书序列号、12 主站证书、13 ESAM 安全存储对象列表、14 红外认证时效门限、15 红外认证剩余时间。方法：1 复位、2 执行、3 ESAM 数据读取、4 数据更新、5 协商失效、6 钱包操作、7 密钥更新、8 证书更新、9 设置协商时效、10 钱包初始化、11 红外认证请求、12 红外认证指令。

DCU 等级 `full`，按项目规范裁剪。F101 与现有 ESAM 驱动对接。密钥、证书、钱包三类方法必须走安全边界，不能把明文留在普通日志里。

### Class 22 输入输出设备接口类

实例数 0…n。属性：1 逻辑名、2 设备对象列表、3 设备对象数量、4 配置参数。方法：1 复位、2 执行。

DCU 等级 `read`。F200 到 F210 的串口、红外、开关量、模拟量、继电器、告警、多功能端子、交采、载波、脉冲、蓝牙等接口。配置参数按项目白名单可写。

### Class 23 总加组接口类

实例数 0…n。属性：1 逻辑名、2 总加配置表、3 总加有功功率、4 总加无功功率、5 总加滑差平均有功、6 总加滑差平均无功、7 总加日有功电量、8 总加日无功电量、9 总加月有功电量、10 总加月无功电量、11 总加剩余电量、12 当前功率下浮控后总加有功冻结值、13 总加组滑差时间周期、14 功控轮次配置、15 电控轮次配置、16 控制设置状态、17 当前控制状态、18 换算及单位。方法：1 清空总加配置单元（必选）、2 执行、3 添加一个总加配置单元、4 批量添加总加配置单元、5 删除一个总加配置单元。

DCU 等级 `full`。2301 到 2308 八个总加组。总加计算与 Class 13 控制、Class 23 属性 3 到 12 联动。

### Class 24 分项事件对象接口类

实例数 0…n。属性：1 逻辑名、2 关联对象属性表、3 当前记录数、4 最大记录数、5 配置参数、6 到 9 事件记录表 1 到 4、10 当前值记录表、11 上报标识、12 有效标识、14 时间状态记录表。方法：1 复位、2 执行、4 添加一个事件关联对象属性、5 删除一个事件关联对象属性。

DCU 等级 `full`。3000 到 300B 的分项事件，包括失压、欠压、过压、断相、失流、过流、功率因数超下限、全失压、辅助电源掉电、电压逆相序、电流逆相序、无功需量超限。事件记录与 Class 7 共用存储与上报，但属性 3 是四张表的记录数结构，不是单个计数。

本仓库已落地的 3000/3005 行按表193编号暴露 A2 关联对象属性表（array OAD）、A3 当前记录数（四表 structure）、A4 最大记录数（long-unsigned）、A6 事件记录表 1（array structure）、A11 上报标识（enum 0 到 3）、A12 有效标识（bool）、A14 时间状态记录表（array structure）。写路径只接受 A11、A12，A11 拒绝 0 到 3 之外的值。方法 1 复位清空记录表、当前记录数与最近发生/结束时间。北向标量表已登记 A4（long-unsigned）、A11（enum）、A12（bool）。

事件时标在生成记录时从 `CLOCK_REALTIME` 取整秒，和 `uv_hrtime` 单调纳秒并列保存：记录里的单调值只作存在性判定与审计副本（监测阶段的延时判定一直用 `ev->ts`），`epoch_s` 用于 A6/A14 的 `date_time_s` 与 `event_history` 落库。表49 的 `date_time_s` 不带时区，主站看到的是设备本地民用时间，因此编码经 `localtime_r`、解码经 `mktime`，进程 TZ 即设备配置的时区；**部署时必须配置设备 TZ**，否则主站读到的是 UTC。没有墙钟（如时钟读取失败）时回 `TYPE_MISMATCH`（线上 DAR 7），不把 0 或单调值编成公历日期。A6 结束时间列没有 NULL 槽位：事件未结束时编表49 的非法时标（年 `FFFFH`，其余字段 `FFH`），已结束但墙钟不可编码则失败关闭。A14 的最近一次发生/结束时间按表194 明确允许 NULL，未发生/未结束编 `0x00`。A6 记录单元里的关联对象数据在生成记录时抓拍 A-XDR 值，取不到编 NULL；上报状态没有按通道存储，编空数组。`event_history` 从此写入墙钟整秒；旧行存的是单调纳秒，不迁移、不渲染：幅度不能可靠区分两种单位，未来读取必须先加单位/版本字段并丢弃旧行。

其余属性保持 mask 关闭：A5 配置参数（structure；3000 需要电压上限、电压下限、电流下限三路门限，结构只存单门限，不能填满）、A7 到 A9 事件记录表 2 到 4（未实例化）、A10 当前值记录表（array）、A13 失压统计（structure，次数与累计秒未存储）。关闭位在门面层按 DAR 5（类定义有、实例未声明）拒绝；类里没有的属性号回 DAR 4，未知 OI 回 DAR 6。

A4、A11、A12 的对象层测试在 `tests/test_dcu_698_object_model.c`，标量表测试在 `tests/test_dcu_698_value_table.c`，A6/A14 的时间编码、NULL 与非法时标在 `tests/test_dcu_698_axdr.c`，流式记录到线上 A6 的墙钟对拍在 `tests/test_dcu_advanced_modules.c`，类属性上限与目录一致在 `tests/test_dcu_698_catalog.c`。

### Class 25 无线公网通信接口类

实例数 0…n。属性：1 逻辑名、2 通信配置、3 主站通信参数表、4 短信通信参数、5 版本信息、6 支持规约列表、7 SIM 卡 ICCID、8 IMSI、9 信号强度、10 SIM 卡号码、11 拨号 IP。方法：1 复位。

DCU 等级 `full`。4500 与 4501 两个公网模块。配置写入后持久化，信号强度等动态属性来自模块。

### Class 26 以太网通信接口类

实例数 0…n。属性：1 逻辑名、2 通信配置、3 主站通信参数表、4 终端 IP、5 MAC 地址。方法：1 复位。

DCU 等级 `full`。4510 到 4517 八个以太网模块。IP、端口、MAC 与主站参数可读写，写操作需授权。

### 关键方法参数

下列参数结构决定编解码，实现时逐字节对照 `docs/698.pdf` 的类定义与附录 A，`docs/698_parsed.md` 只作检索。参数名用原文术语，代码里用同义英文名并在注释写中文。

Class 10 采集档案相关方法：

| 方法 | 参数 | 说明 |
| --- | --- | --- |
| 127 Add | `Basic_object` | 添加一个配置单元，基本信息必须完整 |
| 128 AddBatch | `array Basic_object` | 批量添加 |
| 129 Update | `structure { 配置序号 long-unsigned, 基本信息 Basic_object }` | 更新基本信息 |
| 130 Update | `structure { 配置序号, 扩展信息, 附属信息 }` | 更新扩展与附属信息，NULL 表示不更新 |
| 131 Delete | `配置序号 long-unsigned` | 按序号删除 |
| 132 Delete | `基本信息 Basic_object` | 按基本信息删除 |
| 133 Delete | `structure { 通信地址, 端口号 }` | 按地址与端口删除 |
| 134 Clear | `NULL` | 清空档案表 |

`Basic_object` 至少包含通信地址 TSA、波特率、端口号、规约类型、表地址、接线方式。完整字段见附录 A.7 表 A.7。

Class 13 控制方法：

| 方法 | 参数 | 说明 |
| --- | --- | --- |
| 3 添加控制单元 | `控制单元` | 控制单元结构由对象实例定义 |
| 4 删除控制单元 | `总加组对象 OI` | 按总加组删除 |
| 5 更新控制单元 | `控制单元` | 更新控制参数 |
| 6 控制投入 | `总加组对象 OI` | 投入控制，需鉴权 |
| 7 控制解除 | `总加组对象 OI` | 解除控制，需鉴权 |

Class 19 设备管理方法：

| 方法 | 参数 | 说明 |
| --- | --- | --- |
| 1 复位 | `NULL` | 设备重启 |
| 3 数据初始化 | `NULL` | 清空设备数据区 |
| 4 恢复出厂参数 | `array OAD` | 列表内 OAD 保持现状 |
| 5 事件初始化 | `NULL` | 清空事件存储区 |
| 6 需量初始化 | `NULL` | 当前需量清零 |

Class 21 ESAM 方法：

| 方法 | 参数 | 应答 |
| --- | --- | --- |
| 3 ESAM 数据读取 | `SID` | `octet-string` |
| 4 数据更新 | `structure { 参数内容 octet-string, 数据验证码 SID_MAC }` | 无 |
| 5 协商失效 | `NULL` | `date_time_s` |
| 6 钱包操作 | `structure { 操作类型 integer, 购电金额, 购电次数, 户号, 数据验证码, 表号 }` | 无 |
| 7 密钥更新 | `structure { 密钥密文 octet-string, 数据验证码 SID_MAC }` | 无 |
| 8 证书更新 | `structure { 证书内容 octet-string, 安全标识 SID }` | 无 |
| 9 设置协商时效 | `structure { 参数内容 octet-string, 安全标识 SID }` | 无 |
| 10 钱包初始化 | `structure { 预置金额, 数据验证码 SID_MAC }` | 无 |
| 11 红外认证请求 | `RN` | `structure { 表号, ESAM 序列号, 随机数 1 密文, 随机数 2 }` |
| 12 红外认证指令 | `随机数 2 密文 octet-string` | 无 |

## 7. 存储映射

| 类 | 运行期主存 | 持久化 | 说明 |
| --- | --- | --- | --- |
| 1 到 6 | DataHub 影子与 `dcu_value_t` 缓存 | `dcu_telemetry_latest` | 实时值按 OAD 与表地址索引 |
| 7、24 | `dcu_event_record_t` 环形缓冲 | `edge_event` 端口或 `dcu_events` 表 | 排序键 `(meter_addr, oi, start_ms)` |
| 8 | 参数字典 | `dcu_params` 表 | 写操作审计 |
| 9 | `dcu_freeze_record_t` | `dcu_freeze_history` | 按 `(meter_addr, freeze_oad, freeze_ts_ms, src_oad)` 主键 |
| 10 | `dcu_acq_plan_t`、档案、规则表 | `dcu_meters`、`dcu_plans`、`dcu_rules` | 601F 规则唯一来源 |
| 11 | OAD 列表 | 配置表 | 只读集合 |
| 12 | 脉冲计数缓存 | `dcu_pulse` | 可选 |
| 13 | `dcu_control_op_t` | `dcu_control_audit` | 选择执行与闭锁 |
| 14 到 16 | 统计结果缓存 | `dcu_stats` | 本地流分析产出 |
| 17 | 显示配置 | 配置表 | 可选 |
| 18 | 文件块缓冲 | 文件系统 | DFU 与 OTA |
| 19 | 设备信息 | 配置表 | 二次确认与审计 |
| 20 | 对象列表与权限 | `dcu_app_acl` | 访问控制事实来源 |
| 21 | ESAM 状态 | ESAM 芯片 | 密钥与证书不出芯片 |
| 22 | 接口配置 | 配置表 | 串口、GPIO、载波 |
| 23 | 总加组状态 | `dcu_groups` | 与 Class 13 联动 |
| 25、26 | 通信配置 | 配置表 | APN、IP、端口 |

持久化表名是设计名，实现时以迁移脚本与 `dcu_storage` 的 schema 为准。表结构变更必须走存储硬化 ADR-0007 的 WAL 与掉电保护要求。

## 8. 实现落点

| 文件 | 职责 |
| --- | --- |
| `cmd/dcu/dcu_698_object_model.{h,c}` | 对象表、查表、类处理器注册 |
| `cmd/dcu/dcu_698_var.c` | Class 1 到 6 的读取 |
| `cmd/dcu/dcu_698_event.c` | Class 7 与 24 |
| `cmd/dcu/dcu_698_param.c` | Class 8 |
| `cmd/dcu/dcu_698_freeze.c` | Class 9 |
| `cmd/dcu/dcu_698_acq.c` | Class 10 |
| `cmd/dcu/dcu_698_set.c` | Class 11 |
| `cmd/dcu/dcu_698_pulse.c` | Class 12 |
| `cmd/dcu/dcu_698_control.c` | Class 13 |
| `cmd/dcu/dcu_698_stat.c` | Class 14 到 16 |
| `cmd/dcu/dcu_698_display.c` | Class 17 |
| `cmd/dcu/dcu_698_file.c` | Class 18 |
| `cmd/dcu/dcu_698_devmgr.c` | Class 19 |
| `cmd/dcu/dcu_698_appconn.c` | Class 20 |
| `cmd/dcu/dcu_698_esam.c` | Class 21 |
| `cmd/dcu/dcu_698_io.c` | Class 22 |
| `cmd/dcu/dcu_698_group.c` | Class 23 |
| `cmd/dcu/dcu_698_comm.c` | Class 25 与 26 |

文件按类组切，单文件不超过 600 行。对象表本身集中在 `dcu_698_object_model.c`，各文件只提供处理器。`dcu_router.c` 保留为兼容门面，内部转调 `dcu_698_*`，P1 完成后删除重复注册。

## 9. 一致性与验证

完整测试矩阵、验收场景 S1 到 S18 与发布判据见 `docs/plans/2026-11-21-dcu-698-production-plan.md` 第 7.4 到 7.7 节。

每个类的属性与方法用例至少覆盖正常值、边界值、负路径、目标离线或资源不足四类，见计划 7.4 节。每个类还至少有三类结构测试：

1. 对象表一致性测试。表里每个 OI 的 `class_id` 与目录一致，`attr_mask` 与目录的属性列表一致，`method_mask` 与方法列表一致。
2. 属性读写测试。每个首版支持的属性有正常值、边界值、越权、未定义四类用例。
3. 方法测试。每个必选方法有合法参数、缺参、错类型、越权、目标离线五类用例。

北向验收按类覆盖：

- Class 1 到 6，GET 实时值与批量 GET。
- Class 7 与 24，GET Record 与事件上报。
- Class 8，SET 参数并回读。
- Class 9，冻结、GET Record、RSD 过滤。
- Class 10，档案增删改查、任务配置、方案下发、状态召测。
- Class 11 到 17，按首版等级读或返回 DAR=5。
- Class 18，文件分块上传、校验、激活。
- Class 19，重启、对时、恢复出厂二次确认。
- Class 20，未授权对象返回 DAR=3。
- Class 21，安全会话建立与带 MAC 请求。
- Class 22，接口配置读写。
- Class 23，总加组配置与总加值。
- Class 25 与 26，通信参数读写。

一致性测试使用 `docs/design/DCU_698_OI_CATALOG.json` 生成用例参数，避免手写 513 行。生成器输出 `tests/dcu_698_catalog_vectors.h`，测试文件包含它。生成物进构建，手改即失败。

## 10. 未决问题

1. 513 行 OI 中有 79 行的 IC 列与范围推断不一致。这是 `docs/698_parsed.md` 的 OCR 列错位，不是标准歧义。以 `docs/698.pdf` 原文与附录 A 表号为准重新抽取，D1-01 修生成器与 override 表，不需要产品决策，也不需要协议负责人“裁决语义”。
2. 从站端口的 `resolve_axdr` 与 `accept_axdr` 是对 `foundation/include/edge_slave_plane.h` 的公开 ABI 扩展。标准给了数据形状，设计给了通道形状。实现前补一份 ADR，写清向后兼容与 IEC104、Modbus 标量路径不变。
3. Class 12 脉冲计量与 Class 17 显示是否首版必须支持，取决于项目规范。
4. Class 21 的密钥、证书、钱包方法是否首版开放，取决于安全认证要求。
5. Class 25 与 26 的多模块并存策略，以及主备通道切换，需要与北向多主站方案一起定。
6. 对象表的存储布局：静态数组还是按类分组的多张表。首版用一张 `const` 表，OI 超过 256 时再按类分组。
