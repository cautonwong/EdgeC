# 02 授权模型：声明、批准、执行、撤销

状态：设计基线，待评审。未实现。

## 1. 结论先行

清单里的 `capabilities` 是**申请**，不是授权。授权来自三条同时成立：管理员批准、宿主策略允许、运行时逐次校验通过。任何一条不成立，调用被拒绝并留审计。

上一版文档写"能力即权限"，把申请当成了授权，这是安全体系里最危险的一类简化，本文件撤销该表述。

## 2. 四层分离

| 层 | 由谁产生 | 存在哪里 | 能改变什么 |
| --- | --- | --- | --- |
| 申请 | 应用作者，写进 `app.yaml` | `app_version.manifest` | 只决定"能申请什么" |
| 批准 | 管理员，显式操作 | `grant` 表 | 决定"实际给了什么" |
| 策略 | 设备所有者，设备级配置 | 策略文件，随固件或运维下发 | 决定"这类申请现在允不允许" |
| 执行 | 宿主，每次 Host 调用 | 运行时内存中的授权快照 | 决定"这一次放不放行" |

四层的顺序不能颠倒。批准不能绕过策略，执行不能绕过批准。

## 3. 能力模型

能力按资源类型分组，每个能力带范围。范围是必须的，不接受"申请全部"。

```yaml
capabilities:
  tags:
    read:  [U_A, U_B]        # 明确点位，不支持通配
    write: []
  alarms:
    raise: [power.voltage]   # 明确规则类型
  kv:
    namespace: app.voltage_guard
  schedule:
    jobs: []
  net:
    egress: []               # 默认空，即禁止
```

v1 只开放 `tags.read`（单点位）、`kv`、`log`。`tags.write`、`alarms.raise`、`net.egress` 在 P0 之后单独评审。

通配符只允许出现在宿主策略里，不允许出现在应用清单里。应用写 `U_*` 直接判为非法清单。

## 4. 批准流程

```
申请(manifest)
   │
   ├─ 静态检查：范围是否合法、是否含通配、是否属 v1 开放集
   │      └ 不通过 → 拒绝安装，不产生 grant
   │
   ├─ 只读能力 + 策略 auto_grant_read = true
   │      └ 自动写 grant，granted_by = "policy:auto_grant_read"，仍记审计
   │
   └─ 其余 → grant 状态 pending，应用可安装但不可启动
          └ 管理员批准 → granted；拒绝 → denied，可卸载
```

写能力不接受自动批准。批准写能力必须满足：

1. 操作者具备 `app.approve_write` 权限。
2. 触发双人复核，复用现有双人授权服务，不新造一套。
3. 记录工单号或原因，为空拒绝。
4. 设备不处于"调试窗口期"以外的宽松模式。

## 5. 执行点

每个 Host 函数在执行前做四项检查，顺序固定，任一失败即拒绝。

1. 应用身份：调用来自哪个实例，实例是否属于该应用。
2. grant：该应用是否存在未撤销、未过期的 grant，且范围覆盖本次资源。
3. 策略：设备级策略当前是否允许该类操作。
4. 配额：本次调用是否超过速率、并发、存储上限。

拒绝返回类型化错误，不返回通用失败：

```
denied_not_granted
denied_expired
denied_policy
denied_scope
denied_quota
denied_revoked
```

每次拒绝写审计，按应用和错误码限流，避免日志被刷爆。放行也写审计，但只记录摘要，不记录每个值。

## 6. 撤销的时效

撤销必须在下一次调用生效，不能等应用重启。

- `grant` 表带 `revoked_at`。
- app-manager 维护一个授权代次 `grant_generation`，撤销时 +1。
- 宿主在每次 Host 调用前比对本地缓存的代次，不一致则重新拉取授权快照。
- 目标：撤销到生效的 p99 ≤ 200ms。这个数字是设计目标，未实测。

## 7. 权限点与角色

应用安装时生成权限点，命名沿用现有规则：

```
app.<app-id>:read        # 查看该应用页面与状态
app.<app-id>:control     # 启停该应用
app.<app-id>:configure   # 修改该应用参数
```

这三种是 Core 权限，写进 `permissions/catalog.yaml` 的运行期扩展，不走构建期展开。角色授予仍由管理员在角色管理里显式操作，不随应用安装自动授予。

内置角色的默认建议：

| 角色 | 默认 |
| --- | --- |
| viewer | 无 |
| operator | `app.*:read` |
| engineer | `app.*:read`、`app.*:control`，可批准只读能力 |
| admin | 全部，可批准写能力 |

"可批准"也走权限点 `app.approve_read`、`app.approve_write`，不复用 `app.*:control`，避免启停权被扩成授权权。

## 8. 审计事件

必须记录的事件，字段至少包含 `app_id`、`app_version`、`capability`、`scope`、`actor`、`result`、`ts`。

```
app.install.requested
app.install.rejected          (原因码)
app.grant.pending
app.grant.approved
app.grant.denied
app.grant.revoked
app.start / app.stop / app.fault
app.cap.allowed               (摘要)
app.cap.denied                (含错误码，限流)
app.uninstall
```

审计落现有审计存储，不新建通道。

## 9. 拒绝路径的界面

- 应用详情页显示"已申请但未批准"的能力，以及申请时间。
- 启动按钮在存在 pending 能力时禁用，提示"等待授权"。
- 运行时拒绝在应用日志里可见，管理员可在审计里看到原因码。
- 撤销后在应用页面显示"权限已撤销"，不静默失败。

## 10. 验收

1. 清单含通配符，安装被拒。
2. 只读能力在 `auto_grant_read=false` 时进入 pending，应用不可启动。
3. 管理员批准后启动成功，审计有批准人。
4. 撤销后 200ms 内下一次读取返回 `denied_revoked`。
5. 读未授权点位返回 `denied_not_granted`，不返回空值。
6. 写能力未经双人复核，批准被拒。
7. 同一应用重复批准不产生重复 grant。
8. `operator` 无法批准任何能力，`engineer` 无法批准写能力。
9. 拒绝日志按应用限流，1 秒内同错误码不超过 N 条。
10. 权限点在应用卸载后失效，旧 token 不能继续调用。

## 11. 未决

- `auto_grant_read` 是否应该默认开启。现场便利与安全默认值需要决策。
- 授权快照的存储形式与大小，是否复用注册表快照。
- 双人复核在离线设备上如何落，现有服务是否可离线。
- 应用权限点是否允许被自定义角色批量授予，还是必须逐个应用批准。
