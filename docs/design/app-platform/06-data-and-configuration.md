# 06 应用参数、数据与跨应用取数

状态：设计基线，待评审。未实现。

## 1. 先分清三种东西

"应用的数据放在哪"这个问题里有三样不同的东西，混在一起就答不清楚。

| 种类 | 例子 | 归属 | 可见性 |
| --- | --- | --- | --- |
| 参数配置 | 越限阈值、工作模式、上报周期 | 应用私有，管理员可改 | 应用可读写，管理员可配，密钥只写不读 |
| 运行时数据 | 中间状态、缓存、游标 | 应用私有 | 只有应用自己能读写 |
| 对外发布 | 派生点位、告警、页面 | 应用声明，平台承载 | 授权后可被其他应用、北向、WebUI 消费 |

参数与运行时数据是私有的。对外发布是唯一的公共接口。跨应用取数只能走第三条，不能去翻别人的私有数据。这一条决定了"装一个新应用要不要改别的应用"的答案：不需要。

## 2. 存储位置与命名空间

应用包目录只放代码和清单，安装时校验后只读，升级整包替换。**任何运行期数据都不写在包目录里**，否则升级会覆盖。

数据全部放在设备数据区，按应用命名空间隔离：

```
/var/edgeos/data/apps/<app-id>/          # 大文件，受配额限制
SQLite（ADR-0007 的库）：
  app_registry       应用清单与状态（已有表，扩展字段）
  app_settings       参数值，按 app_id + key
  app_secret         密钥密文，按 app_id + key
  app_kv             运行时键值，按 app_id + key
  app_data_meta      每个应用的 dataVersion、settingsVersion
```

表结构：

```sql
app_settings(
  app_id TEXT, key TEXT, value_json TEXT,
  schema_version INTEGER, updated_at INTEGER, updated_by TEXT,
  PRIMARY KEY(app_id, key)
);

app_secret(
  app_id TEXT, key TEXT, ciphertext BLOB, nonce BLOB,
  updated_at INTEGER, updated_by TEXT,
  PRIMARY KEY(app_id, key)
);

app_kv(
  app_id TEXT, key TEXT, value BLOB, persist INTEGER DEFAULT 1,
  updated_at INTEGER,
  PRIMARY KEY(app_id, key)
);

app_data_meta(
  app_id TEXT PRIMARY KEY,
  data_version INTEGER, settings_version INTEGER,
  updated_at INTEGER
);
```

`app_registry` 是已有表，需要补 `package_digest`、`abi`、`settings_schema_version`、`data_version` 四列。它现在存的是旧清单字段（`wasm_path`、`trigger_tag`），新平台不沿用旧语义，迁移时一次性转换。

**不复用两张旧表。** `system_params` 是编译期元数据同步到数据库的全局参数，`edge_settings` 是按产品加点位存的旧配置。两者都是全局或产品级，不能承载按应用隔离的数据，混用会让命名空间失效。

## 3. 参数配置

参数在清单里声明，不是代码里硬编码，也不是自由格式的键值对。

```yaml
settings:
  schemaVersion: 1
  fields:
    threshold:
      type: number
      title: 越限阈值
      unit: V
      default: 240
      minimum: 180
      maximum: 300
    mode:
      type: string
      enum: [warn, trip]
      default: warn
      title: 处理方式
    sampleIntervalMs:
      type: integer
      default: 1000
      minimum: 100
      maximum: 60000
      requiresRestart: false
    apiKey:
      type: string
      secret: true
      title: 平台接入密钥
```

规则：

- 类型只有 string、number、integer、boolean、enum、array<string>。够用即可，不引入任意 JSON。
- 每个字段必须有 `default`。安装时按默认值初始化，应用首次启动就能读到完整配置，不需要"先配置再启动"。
- `minimum`、`maximum`、`maxLength`、`enum` 是硬校验，写入不通过返回 422 与字段错误，不做静默截断。
- `secret: true` 的字段进 `app_secret`，接口永不返回明文，只返回"已设置/未设置"。应用通过 Host ABI 读取明文，只在内存中使用。
- `requiresRestart: true` 的字段变更后重启应用；其余字段通过变更通知让应用热更新。
- 未声明的键写入一律拒绝。保留未知键会让配置在升级后悄悄失效。
- 参数属于应用，不属于某个版本。升级保留参数，卸载在宽限期后删除，恢复安装时可选择恢复。

写入路径：

1. WebUI 或 `appctl settings set` 提交。
2. 校验权限 `app.<id>:configure`。
3. 按清单 schema 校验类型与范围，失败返回字段级错误。
4. 单事务写入，写审计（谁、哪个键、旧值摘要、新值摘要；密钥只记"已变更"）。
5. 通知运行中的实例；需要重启的按故障策略重启。

WebUI 的设置页由 schema 生成，不改前端代码。这是"装一个应用不用改别的应用"的另一半，页面和表单也不用手写。应用如果需要自定义界面，走第 07 号文档的三层模型，参数仍然只存这里，不随界面方案变化。

## 4. 运行时数据

- `kv` 按应用命名空间隔离，键是应用自己的字符串，不跨应用可见。
- 单值上限与总量配额由清单 `limits.storageKb` 约束，超限写失败并告警，不淘汰旧数据。
- `persist` 标记决定备份与恢复：`persist=1` 进备份，`persist=0` 视为缓存，重启或恢复后可以丢。
- 大于单值上限的二进制放 `/var/edgeos/data/apps/<app-id>/files/`，路径由宿主分配，应用拿到的是句柄，不是任意路径。目录配额独立计算。
- 时间序列不进 KV。需要历史的数据必须以点位发布，由历史存储负责降采样与保留。

## 5. 升级、降级与迁移

- 参数与数据都带版本：`settings_version`、`data_version`。
- 升级时先跑迁移演练：用现有参数与数据在内存里跑 `migrateSettings` 与 `migrateData`，任何一条失败就不允许升级。迁移函数是纯函数，可单测。
- 迁移幂等，允许重跑。迁移不删除旧字段，先写新字段。
- 降级有硬规则：如果设备上的 `data_version` 高于目标版本支持的版本，拒绝降级并给出可读原因。降级不能让旧代码读新格式的数据。
- 参数在所有版本间共享，不随版本复制。回滚不恢复旧参数，除非管理员显式选择"回滚含配置"。

## 6. 备份与恢复

- 备份包含：`app_settings`、`persist=1` 的 `app_kv`、`app_secret` 密文、`app_data_meta`、包清单与摘要。
- 备份不包含：包文件本体（可从商店重装）、`persist=0` 的缓存、运行中状态。
- 恢复到另一台设备时，密钥用设备密钥加密，可能无法解密。此时标记"密钥需重新录入"，不阻断应用启动，页面提示管理员补录。
- 恢复顺序：安装对应版本的应用，恢复参数，恢复数据，再启动。版本不匹配时先拒绝恢复该应用，记录待办，不让它带着错版本的数据跑起来。

## 7. 跨应用取数：点位是唯一公共接口

一个应用要拿另一个应用的数据，只有一条路：对方把数据以点位发布，自己申请读取授权。没有直接调用，没有共享内存，没有读对方 KV。

点位的契约在清单里声明：

```yaml
contributes:
  telemetry.points:
    - name: sag_count            # 完整名 app.voltage_guard.sag_count
      type: number
      unit: 次
      retention: latest          # latest | latest+history
      stability: stable          # stable | experimental
```

消费方在清单里声明自己要什么，以及期望类型：

```yaml
requires:
  points:
    - name: app.voltage_guard.sag_count
      type: number
    - name: app.demand_forecast.next_peak_kw
      type: number
```

规则：

- 名称与类型必须完全匹配，v1 不做隐式类型转换。类型不符拒绝订阅，返回 `denied_type_mismatch`，而不是给一个看起来像数字的字符串。
- `stability: experimental` 的点位可以被消费，但安装时提示"上游可能变化"。
- 授权仍然走第 02 号文档：消费方申请 `tags.read`，管理员批准 `app.<publisher>.<point>` 范围。
- 点位的 `retention` 决定消费方能否读历史。只声明 `latest` 的点位只能拿当前值。

### 后装应用怎么拿到已有数据

这是本节的核心问题。答案分四种情况，都不需要改发布方。

1. **当前值**。DataHub 保留每个点位的影子值。新应用安装并授权后订阅，立刻拿到当前值，不需要等下一次发布。
2. **历史值**。发布方声明了 `retention: latest+history`，消费方又获批历史权限，才能回填。否则只能从现在开始积累。
3. **发布方还没装**。允许安装，应用以 `degraded` 状态启动，读取返回 `unavailable`，页面上明确显示"等待上游应用"。发布方装上并开始发布后，消费方自动转为正常。安装顺序不构成依赖。
4. **发布方没有发布这个点位**。消费方拿不到，这是封装，不是缺陷。解决办法是发布方发一个新版本把点位暴露出来，这是正常的应用更新，不是"装 B 要改 A"。

### 反向同理

先装的应用要拿后装应用的数据，走同一套：后装应用发布点位，先装应用在清单里声明 `requires.points` 并获批，运行时自动订阅。发布方不知道谁在消费，消费方也不需要发布方为它做任何改动。

### 明确不做的

- 应用之间直接 RPC 调用：会把两个应用的版本和生命周期绑死。
- 读对方的 KV 或配置文件：绕过授权与封装。
- 共享内存直连：绕过类型与权限检查。
- 应用间依赖声明：v1 禁止，避免启动顺序与循环依赖。需要协作时通过点位间接完成。

## 8. 安装一个应用到底改了哪些行

为了回答"是不是得改别的应用"，把一次安装的写入面列清楚。

```
新增或修改：
  app_registry        一行，本应用
  app_version         一行，本应用
  app_settings        若干行，本应用默认值
  app_secret          零行
  app_kv              零行
  contribution        本应用声明的若干行
  grant               pending 或已批准的若干行，本应用

不修改：
  其他应用的任何行
  其他应用的包文件
  其他应用的参数与数据
  全局 system_params
  产品 edge_settings
```

安装只写自己的行。发布方的包摘要、参数、数据在安装前后逐字节不变，这可以作为一条验收。

## 9. 反模式

- 把参数写进包目录。升级整包替换会丢配置。
- 把应用参数塞进 `system_params`。那是编译期元数据的镜像，全局可见。
- 把应用数据塞进 `edge_settings`。那是按产品加点位存的旧模型，没有应用命名空间。
- 让应用 A 直接读应用 B 的 KV。授权、审计、版本隔离全部失效。
- 用安装顺序表达依赖。谁先装谁后装都会出现，必须让缺席成为可处理的状态。
- 用 KV 存时间序列。历史、降采样、保留策略都不归应用管。

## 10. 验收

1. 安装后应用首次启动能读到全部默认参数，不需要先配置。
2. 写入类型错误、越界、未声明的键，返回 422 与字段级错误。
3. `secret` 字段通过接口读取只返回"已设置"，不返回明文。
4. 升级后参数保留，迁移演练失败时升级被阻断。
5. 设备数据版本高于目标版本时降级被拒，提示可读。
6. 备份恢复后参数与 `persist=1` 的 KV 一致，缓存不恢复。
7. 后装应用订阅已发布点位，安装后立即读到当前值。
8. 发布方未安装时，消费方以 `degraded` 启动，安装不失败；发布方上线后自动恢复。
9. 安装应用 B 前后，应用 A 的包摘要、参数行、KV 行完全不变。
10. 应用尝试读另一个应用的 KV 或参数，返回 `denied_not_granted`。
11. 点位类型不匹配时订阅被拒，返回 `denied_type_mismatch`。
12. 超过存储配额写入失败并告警，不淘汰旧数据。

## 11. 未决

- 大文件的句柄接口形态，是路径加配额，还是读写回调。
- 备份到另一台设备时，密钥是否需要支持导出与重新封装。
- 结构化数据（如分时电价表）跨应用共享是否需要独立的 `datasets` 贡献类型。v1 只支持标量与字符串点位，结构化共享留到有真实需求再开。
- 参数变更是否需要复用现有配置事务的"应用后确认"流程。v1 采用立即生效加审计，高风险参数另议。
