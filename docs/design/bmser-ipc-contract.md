# BMSER-BIN 北向 IPC 契约（P3-01b 冻结）

> 状态：已冻结，P3-01b。数据面契约与严格解析器已落地；外部进程实现、启动接线和产品是否采用不在本文冻结范围内。
> 来源：`/home/edge/iems_v3` commit `3b5c73ff55a532fb715104d03fbe4e09db807173`，`config/bmser_ipc_bin.json` 17,701 B，sha256 `5ff7a072a143d9eeba7ec4feea14988279a2d28654620f357478004ea1be3203`。
> 目标基线：`feat/iems` `f304bf44`。代码落点：`cmd/iems/iems_bmser_ipc.h`、`cmd/iems/iems_bmser_ipc.c`、`tests/test_iems_bmser_ipc.c`。
> 证据标记：【静态】源码或配置直接可见；【推导】按源码逐字节推出；【未知】本机无法确定；【提案】是建议，不是既有事实。

## 1. 范围

本文冻结四件事。

1. `bmser_ipc_bin.json` 的文件结构和每个字段的语义。
2. v3 消费这些字段的方式：tag 身份、量纲、周期、块与子项地址。
3. 能从 v3 源码导出的传输格式：MQTT 主题、帧头、块布局、四种消息的行为。
4. 能从 v3 源码导出的启动方式，以及不可导出的部分。

不在本文范围：外部 `bmser_trans` 等进程的实现（本体不在 repo）、P3-01c 的启动接线、产品是否采用这套契约（第 8 节，open decision）。本文不实现产品接线。

## 2. 文件模型

v3 固定从 `/app/config/bmser_ipc_bin.json` 读取【静态】（`src/ems/mqtt_north.h:20`）。`load_mqtt_north_info` 读文件（`src/ems/mqtt_north.c:178`），`paser_north_mqtt_file` 解析根和设备（`:125`），`paser_north_mqtt_blocks` 解析块（`:94`），`paser_north_mqtt_sub` 解析子项（`:65`）。

根是对象，只有两个键：

| 字段 | 类型 | 必填 | 语义 |
| --- | --- | --- | --- |
| `schema_version` | string | 否 | 若出现必须是 `"1.0.0"`。v3 文件没有该键；转换产物可以带。 |
| `dev` | array | 是 | 设备列表。v3 缺少该键时加载失败【静态】（`:137-144`）。 |

每个 `dev` 元素的字段：

| 字段 | 类型 | 必填 | 语义 |
| --- | --- | --- | --- |
| `dev_id` | string | 是 | 设备标识。用于主题第 4 段，入站按它做大小写敏感查找【静态】（`:264-274`、`:999`）。v3 结构是 `char[128]`【静态】（`mqtt_north.h:56-63`）。 |
| `module` | string | 是 | 主题第 3 段。入站不校验，回发和订阅用它。 |
| `type` | string | 是 | 主题第 5 段。入站不校验。 |
| `blocks` | array | 是 | 块列表。 |

每个 `block` 元素的字段：

| 字段 | 类型 | 必填 | 语义 |
| --- | --- | --- | --- |
| `block_id` | string | 是 | 块号。`0x` 前缀按十六进制解析，否则按十进制【静态】（`src/common.c:362-374`）。线上是 4 字节大端。 |
| `cycle` | integer | 是 | 周期推送间隔，单位 ms。`0` 表示不周期推送。v3 按整秒取模，只有 1000 的整数倍有意义【静态】（`:962-984`）。 |
| `sub_id` | array | 是 | 块内点位槽位，顺序就是线上字节顺序。数组下标加 1 就是线上 `StartSubId`。 |

每个 `sub_id` 元素的字段：

| 字段 | 类型 | 必填 | 语义 |
| --- | --- | --- | --- |
| `name` | string | 否 | 人读标签。v3 解析文件时根本不读它【静态】（`:65-90`）。 |
| `type` | string | 是 | 六个枚举之一：`U16`、`S16`、`U32`、`S32`、`FLOAT`、`BI16`【静态】（`:9-28`）。 |
| `tag` | string | 是 | 点位名。全局身份是 `<dev_id>.<tag>`，由 `find_tag_by_name` 精确匹配【静态】（`:81-83`，`src/proto_forward.c:1353-1367`）。 |
| `scale` | number | 是 | 量纲系数，必须有限且非零。读方向线上值 = 工程值 × scale；写方向工程值 = 线上值 / scale【静态】（`src/proto_forward.c:1146`、`:1208`）。 |

`config/schema/bmser_ipc_bin.schema.json` 已描述同一形状，但它按 v3 的宽松读取器写成，未要求 `blocks`、`block_id`、`cycle`、`scale`。本契约以第 5 节的严格规则为准，schema 对齐另行安排。`config/schema/file_families.json:80-86` 把该文件登记为 `bmser_ipc_bin` 族。

### 2.1 仓库样例的精确计数

两个设备，九个块，170 个子项槽位。

| dev_id | module | type | 块数 | 块（block_id, cycle, 子项数） |
| --- | --- | --- | --- | --- |
| `ems` | `env` | `emsctrl` | 3 | `0x8001, 1000, 8`；`0x8011, 1000, 13`；`0x8021, 1000, 74` |
| `pcs456` | `ems` | `pcsdata` | 6 | `0x8001, 0, 8`；`0x8011, 1000, 6`；`0x8012, 1000, 5`；`0x8013, 1000, 10`；`0x8021, 1000, 34`；`0x8031, 0, 12` |

类型分布：`U16` 95，`S32` 55，`U32` 11，`S16` 9，`FLOAT` 0，`BI16` 0。scale 分布：1 出现 93 次，10 出现 69 次，100 出现 1 次，1000 出现 7 次。75 个子项没有 `name`。25 个槽位的 tag 是 `place<N>` 占位符。170 个槽位对应 154 个不同的 `<dev_id>.<tag>` 身份；重复是文件允许的一种状态，见 3.2。

上述计数由 `tests/test_iems_bmser_ipc.c::test_real_file_counts` 对真实文件的逐字节副本断言。

## 3. 字段语义

### 3.1 身份与主题

设备身份由三个字符串组成，主题模板是 `ipc/<sn>/<module>/<dev_id>/<type>/<verb>/binaryFlow`【静态】（`src/ems/mqtt_north.c:571`、`:935`、`:1040-1044`）。入站只取第 4 段做设备查找，`module` 和 `type` 不校验【静态】（`:999-1009`）。因此 `module`、`type` 只影响订阅和回发，不影响收包匹配。

`dev_id` 匹配是大小写敏感的 `strcmp`【静态】（`:268`）。源码注释用 `PCS456`，样例文件用 `pcs456`，二者不匹配；现场以实际文件为准【静态+推断】。

### 3.2 tag 与占位符

子项的点位身份是 `sprintf("%s.%s", dev_id, tag)`【静态】（`:81-82`）。v3 把这个字符串写进 `mqtt_sub_info_t.name[64]`，再用同一个字符串查点表【静态】（`mqtt_north.h:37-43`、`mqtt_north.c:83`）。因此 `<dev_id>.<tag>` 必须能放进 63 字节，加结尾 NUL 共 64 字节。

tag 不要求唯一。真实文件在 `ems` 下重复 `GridPower` 两次，在 `pcs456` 下重复 `operation_mode` 两次，并且 `place<N>` 会重复出现很多次。v3 为每个槽位各自保存一个 tag 指针，重复的 tag 指向同一个点位，所以重复槽位的读值是同一个值，写方向是后写覆盖，顺序由配置顺序决定【静态+推断】（`:78-88`、`:484-508`）。

`place<N>` 是占位槽位，用来在块内留出线宽。它按普通 tag 查表，通常查不到；读方向 v3 对 NULL tag 返回 0 并记日志【静态】（`src/proto_forward.c:1136-1142`），写方向跳过 NULL tag【静态】（`:1186-1189`）。本契约只冻结“占位符按普通类型占线宽、不绑定点位”这一事实；外部进程是否也这样理解，见第 7 节【推导+未知】。

### 3.3 scale

scale 的语义容易读反。`read_data_form_tag_by_p` 把工程值乘以 scale 后返回【静态】（`src/proto_forward.c:1146-1166`），`get_tag_data_by_type` 再把这个值按大端写进 `GetResp`【静态】（`src/ems/mqtt_north.c:736-800`）。所以线上值 = 工程值 × scale。`Set` 方向相反，`write_tag_data_by_type` 解出线上值后，`write_data_to_tag_by_p` 除以 scale【静态】（`:343-414`，`src/proto_forward.c:1206-1212`）。scale 为 10 表示线上整数放大了十倍。

v3 的读取器在 `scale` 缺失时得到 calloc 的 0【静态】（`:78`），读方向乘 0，写方向除 0。本契约因此要求字段必填且非零。

### 3.4 cycle

周期任务每秒执行一次，`cycle != 0` 且 `time % (cycle/1000) == 0` 时推送该块【静态】（`:948-984`）。这带来三个可导出的结论。

1. `cycle = 0` 只响应显式 `Get`，没有主动推送。
2. `1..999` 时 `cycle/1000` 为 0，取模除零【静态】。本契约拒绝该区间。
3. 非 1000 整数倍的周期被整秒截断，例如 1500 实际每秒推送。本契约要求 1000 的整数倍。

### 3.5 block_id 与子项地址

`analysis_addr` 对 `0x` 前缀走 `%x`，否则走 `%u`【静态】（`src/common.c:362-374`）。`sscanf` 会接受前导空格、符号和尾部垃圾，本契约只接受规范写法：`0x` 加 1 到 8 个十六进制数字，或 1 到 10 个十进制数字，且值不超过 `0xFFFFFFFF`。

子项地址是 1 基位置。线上 `StartSubId=1` 对应数组下标 0，v3 用 `SUB_INDEX_ADDR_OFFSET=1` 换算【静态】（`mqtt_north.h:45`、`mqtt_north.c:484`）。`StartSubId=0` 会得到下标 -1 并越界【静态+推断】（`mqtt_north.c:830-833`），本契约在线上语义中要求 `StartSubId >= 1`。

## 4. 传输契约

### 4.1 承载

承载是 MQTT。v3 的内置 broker 默认值是 `localhost:1883`，用户名和密码为空串，QoS 0，keepalive 10 秒【静态】（`mqtt_north.h:12-19`、`mqtt_north.c:1086-1088`）。实际 broker 地址、端口、用户名、密码来自启动参数（4.5）。主题第 2 段是 `topic_sn`，为空时退化为 `"no_sn"`，本 HEAD 被硬编码为 `"E4B853F333"`【静态】（`mqtt_north.c:1080`、`src/ems/main.c:1047`）。

| 方向 | 主题 | 触发 |
| --- | --- | --- |
| 订阅 | `ipc/<sn>/<module>/<dev_id>/<type>/Get/binaryFlow` | 每个配置设备一次【静态】（`:1044`） |
| 订阅 | `ipc/<sn>/<module>/<dev_id>/<type>/Set/binaryFlow` | 每个配置设备一次【静态】（`:1040`） |
| 发布 | `ipc/<sn>/<module>/<dev_id>/<type>/GetResp/binaryFlow` | 收到 `Get`，或周期任务【静态】（`:935`） |
| 发布 | `ipc/<sn>/<module>/<dev_id>/<type>/SetResp/binaryFlow` | 收到 `Set`【静态】（`:571`） |

订阅 QoS、retain、clean session、LWT、TLS 由 `libmqtt_session` 决定，该库头文件不在本机，标【未知】。

### 4.2 帧头

所有消息共用同一帧头，线序大端【静态】（`mqtt_north.c:236-262`、`:843-872`）。

| 偏移 | 长度 | 字段 | 编码 |
| --- | --- | --- | --- |
| 0 | 4 | `TransId` | uint32，大端 |
| 4 | 1 | `DevLen` | uint8 |
| 5 | `DevLen` | `DevMes` | 不透明字节 |
| 5+DevLen | 1 | `CmdLen` | uint8 |
| 6+DevLen | `CmdLen` | `CMD` | 不透明字节 |
| 6+DevLen+CmdLen | 2 | `PayloadLen` | uint16，大端，等于帧头之后的全部字节数 |

`TransId` 规则：请求值大于 0 时原样回显；为 0 时用文件内静态计数器，初值 1 并自增【静态】（`:708`、`:847-855`）。响应原样回显请求的 `DevMes` 和 `CMD`【静态】（`:857-869`）。`PayloadLen` 是唯一强校验，读端要求它等于剩余字节数【静态】（`:256-260`）。没有校验和。

### 4.3 载荷

请求（`Get`、`Set`）和 `GetResp` 使用同一种块布局【静态】（`:276-309`、`:873-916`）。

| 载荷偏移 | 长度 | 字段 | 编码 |
| --- | --- | --- | --- |
| 0 | 1 | `block_num` | uint8 |
| 每块 0 | 4 | `block_id` | uint32，大端 |
| 每块 4 | 2 | `StartSubId` | uint16，大端，1 基 |
| 每块 6 | 2 | `SubNum` | uint16，大端 |
| 每块 8 | 2 | `Bytes` | uint16，大端，仅 `GetResp` 有意义；`Get` 取 0 |
| 每块 10 | `Bytes` | `data` | 按配置顺序拼接的点位字节 |

`GetResp` 的数据宽度由类型决定。

| 类型 | 宽度 | 线格式 |
| --- | --- | --- |
| `U16` | 2 | 大端无符号 |
| `S16` | 2 | 大端有符号 |
| `U32` | 4 | 大端无符号 |
| `S32` | 4 | 大端有符号 |
| `FLOAT` | 4 | 大端 IEEE-754 |
| `BI16` | 未知 | v3 编解码两段都被注释，当前实现返回 0 字节【静态】（`:416-432`、`:801-820`）。本契约接受该类型但不定义线宽，见第 7 节。 |

`SetResp` 的每块固定 12 字节内容，块头仍是 8 字节【静态】（`:470-565`）。

| 块内偏移 | 长度 | 字段 | 编码 |
| --- | --- | --- | --- |
| 0 | 4 | `block_id` | 回显请求 |
| 4 | 2 | `StartSubId` | 回显请求 |
| 6 | 2 | `SubNum` | 回显请求 |
| 8 | 2 | `ContentLen` | 成功路径恒为 10 |
| 10 | 2 | `CMD` | 恒为 0 |
| 12 | 4 | `error` | 0 表示成功，1 表示未知块 |
| 16 | 4 | reserved | 恒为 0 |

### 4.4 四种消息的行为

`Get`：主站发到 `.../Get/binaryFlow`。v3 按 `block_id/StartSubId/SubNum` 取点，忽略请求里的 data，截断到块内实际点数后回 `GetResp`【静态】（`:824-840`、`:918-946`）。请求里 `Bytes` 通常为 0，但 v3 不校验【静态】。设备不存在时不响应【静态】（`:1023-1026`）。

`GetResp`：块数取自请求，未知块被静默跳过，但 `block_num` 仍写成请求值，帧内块数可能与实际条目不一致【静态+推断】（`:880-909`）。周期推送构造 `block_num=1`、`StartSubId=1`、`SubNum=块内点数` 的单块响应【静态】（`:962-987`）。

`Set`：主站发到 `.../Set/binaryFlow`。v3 逐点解码，按 scale 除后写入点位；范围同样截断到块内点数【静态】（`:470-508`、`:1010-1016`）。短包会在检查前越界读最多 4 字节再报错【静态+推断】（`:484-498`）。

`SetResp`：成功路径每块 20 字节，块头 8 加内容 12。未知块路径调用 `get_write_error_resp_Conten_data(message)`，它把内容写到载荷起点而不是当前块位置，并把长度写成 1 字节【静态】（`:454-466`、`:559`）。结果 `PayloadLen` 与实际字节数不等，v3 自己的读端也会拒收。本契约把它登记为 v3 已知缺陷，新实现不应复制；主站是否容忍该帧标【未知】。样例帧见 `/home/edge/migration/p4-02b-binaryflow-contract-2026-11-23.md` 第 2.6 节。

### 4.5 启动与生命周期

v3 的北向开关只由命令行 `-m address:port:user:pass` 打开，默认关闭【静态】（`src/ems/main.c:1407`、`:1474-1476`）。`sscanf` 的格式要求四个字段都非空；空用户名或空密码的写法解析不了【静态+推断】。`north_mqtt_client_init` 在南向采集加载完成后调用，文件加载失败时整个北向客户端返回 NULL【静态】（`src/ems/main.c:1180-1181`、`mqtt_north.c:1051-1063`）。`topic_sn` 在本 HEAD 硬编码为 `"E4B853F333"`，读取真实序列号的行被注释【静态】（`src/ems/main.c:1046-1047`）。

外部进程由 `firmware/ipk_ems_core/data/opt/lvt/bin/start_trans.py` 拉起。它每秒检查 `/app/config/device_config.json`，只要有设备的 `channel` 含 `bmser_lib`，就启动 `/opt/lnxall_app/bin/bmser_trans`、`bmser_alarm`、`bmser_upgrade`，否则停止它们【静态】（`start_trans.py:9-85`）。进程本体不在 repo，它如何连接 broker、如何选择主题、是否有重试或认证，全部【未知】。

本 HEAD 的 `mqtt_north_start()` 没有调用者，`mqtt_session_start()` 只在其中被调用【静态】（`mqtt_north.c:1098-1108`；全仓搜索只有声明和定义）。按本 HEAD 的行为，订阅调用会发生，但会话不会启动，周期任务也不会创建。现场固件是否在别处调用，标【未知】，需要 P4-02a 的镜像或日志证据。

## 5. 解析器边界

`iems_bmser_ipc_load` 是文件进入系统的唯一入口。它复用 `iems_config_strict_parse` 与 `iems_config_check_keys`，在解析前拒绝 BOM、裸 NUL、非法 UTF-8、重复键、尾部数据；在解析时拒绝未知键、缺字段、类型不符、超长字符串和越界数值。错误格式是 `KIND path: detail`。

| 规则 | 取值 | 相对 v3 的差异 |
| --- | --- | --- |
| 根 | 对象；`dev` 必填数组；`schema_version` 可选 `"1.0.0"` | v3 不读版本号 |
| 设备数 | 最多 256 | 与 `CHANNEL_DEVS_MAX` 一致【静态】（`src/tag.h:58`） |
| 每设备块数 | 最多 256 | 与 `MAX_MQTT_NORTH_BLOCK_NUM` 一致【静态】（`mqtt_north.h:22`） |
| 每块子项数 | 最多 512 | 与 `MAX_MQTT_NORTH_SUB_NUM` 一致【静态】（`mqtt_north.h:23`） |
| `dev_id`、`module`、`type` | 非空，短于 128 字节 | v3 结构是 `char[128]`，超长会溢出 |
| `name` | 可选，短于 64 字节 | v3 不读该字段 |
| `tag` | 非空，短于 64 字节；`<dev_id>.<tag>` 短于 64 字节 | v3 用 `sprintf` 加 `strcpy` 写 64 字节缓冲 |
| `dev_id` 唯一 | 是 | v3 查找返回第一个匹配，重复会造成二义 |
| 每设备内 `block_id` 唯一 | 是 | 同上 |
| `block_id` 写法 | `0x` 加 1 到 8 个十六进制数字，或 1 到 10 个十进制数字 | v3 的 `sscanf` 接受尾部垃圾 |
| `cycle` | 0 或 1000 的正整数倍 | v3 对 1 到 999 除零，对非整数倍按整秒截断 |
| `type` | 六个枚举之一 | 一致 |
| `scale` | 必填，有限，非零 | v3 缺失时得到 0 |

同一文件重复 tag 和 `place<N>` 占位符是合法的，解析器保留每个槽位并统计 `placeholder_count`。

## 6. 验证

真实文件副本在 `tests/fixtures/bmser_ipc_bin.json`，sha256 与来源一致。测试加载该副本并断言第 2.1 节的全部计数，以及字段样本、重复 tag 和占位符数量。负例覆盖根、设备、块、子项四层的未知键、缺字段、坏类型、坏块号、坏周期、坏 scale、重复 id、超长身份，以及缺文件和非法参数。

运行：

```bash
cmake --build build --target test_iems_bmser_ipc
./build/tests/test_iems_bmser_ipc
```

## 7. 缺口与 UNCHECKED

UNCHECKED 表示本机无法从 repo 与 v3 源码导出，必须由现场证据或产品裁决补齐。G9 到 G15 是已导出的 v3 缺陷，新实现不得继承。

| 编号 | 状态 | 缺口 |
| --- | --- | --- |
| G1 | UNCHECKED【未知】 | 外部 `bmser_trans`、`bmser_alarm`、`bmser_upgrade` 的本体、语言、部署方式。三者是否都参与 IPC，还是只有 `bmser_trans` 参与，未知。 |
| G2 | UNCHECKED【未知】 | 主站与设备侧谁发起 `Get` 和 `Set`，周期推送是否需要主站先订阅或先握手。本文只冻结了 v3 侧观察到的方向。 |
| G3 | UNCHECKED【未知】 | broker 的 retain、订阅 QoS、clean session、LWT、TLS、认证。`libmqtt_session` 头文件不在本机。 |
| G4 | UNCHECKED【未知】 | `mqtt_north_start()` 在本 HEAD 无调用者。现场固件是否调用、何时调用，需要镜像、日志或抓包证据。 |
| G5 | UNCHECKED【未知】 | `BI16` 的线宽和位序。v3 两段编解码都被注释，当前实际输出 0 字节；外部进程是否按 2 字节位域处理，未知。解析器接受该类型只是为了不丢字段，不承诺线宽。 |
| G6 | UNCHECKED【推导+未知】 | `place<N>` 的意图是占位，但 v3 没有专门分支。外部进程是否忽略这些槽位，未知。 |
| G7 | UNCHECKED【未知】 | 现场部署的 `bmser_ipc_bin.json` 与仓库样例可能不同。样例只覆盖 `U16/S16/U32/S32`，没有 `FLOAT` 和 `BI16`。 |
| G8 | UNCHECKED【未知】 | 主站对 v3 的两个畸形响应的容忍度：`GetResp` 未知块导致的块数不一致，`SetResp` 未知块的错位帧。 |
| G9 | 【静态】 | `block_num` 线上是 uint8，但 v3 的 `analsys_dev_info_t.block_info[32]` 只有 32 项，超过会栈越界（`mqtt_north.h:124-128`、`mqtt_north.c:279-282`）。新实现应限制为 32。 |
| G10 | 【静态】 | `DevLen`、`CmdLen` 各 1 字节，v3 直接 `memcpy` 到 128 字节栈缓冲，255 会溢出（`mqtt_north.c:243-249`）。 |
| G11 | 【静态】 | 响应缓冲是 2048 字节栈数组，块数可达 255，每块点数可达 512，极端帧会溢出（`:572`、`:936`）。 |
| G12 | 【静态+推断】 | `StartSubId=0` 会得到下标 -1；`Set` 短包先越界读再报错。新实现必须拒绝这两种输入。 |
| G13 | 【静态+推断】 | 静态 `TransId` 无锁，周期线程和 MQTT 回调线程可能并发自增（`:708`、`:847-855`）。 |
| G14 | 【静态】 | 设备查找大小写敏感，注释和样例大小写不一致。 |
| G15 | 【静态】 | 无校验和。位翻转只能靠 MQTT 或 TCP 发现。 |

## 8. 产品裁决（open）

是否在 EdgeC 产品中采用这套 BMSER-BIN IPC，仍未决。P3-01c 依赖该裁决。可选的路线有三条，本文不替产品选择。

1. 保留外部 v3 进程，EdgeC 只按本文实现北向对端。
2. 由 EdgeC 替换外部进程，本文同时作为对外契约。
3. 不采用，BMS 侧走 CAN 或其他协议。

在裁决前，本文与 `cmd/iems/iems_bmser_ipc.{h,c}` 只作为契约和严格解析器存在，不接入产品启动路径。
