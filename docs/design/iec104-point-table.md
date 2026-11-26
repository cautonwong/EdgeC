# IEC104 CA/点和控制 IOA 表

> 日期：2026-11-26　状态：只读抽取，未改 v3 源码和产品 C 代码
> 工单：P4-01a　基线：`feat/iems` `f304bf44`
> 工具：`tools/migration/iec104_points.py`　快照：`tools/migration/golden/iec104_points/`

## 1. 结论

从 `/home/edge/iems_v3/config/iec104_config.json` 抽出 2 个 CA、178 个点。全部点落在遥信/遥测 IOA 区间，kind 为 telemetry；配置里没有遥控/遥调点。站点控制表不在仓库内，列入第 5 节 UNCHECKED。

| 项 | 值 |
| --- | --- |
| CA | 2（CA 1、CA 2） |
| 点数 | 178（CA 1 176，CA 2 2） |
| 类型 | bool 29，int 76，float 73 |
| kind | telemetry 178，control 0，unknown 0 |
| IOA 范围 | 1..27、16385..16534 |

## 2. 数据形状与来源

工具在写逻辑前先定数据形状：

```
Table(points, ranges, config_rel, warnings)
Point    = (ca, name, ioa, data_type, scale, kind, evidence)
IoaRange = (const, start, kind, source_line)
```

`kind` 不按点名字猜测，也不按条目所在数组判断，而是按 IOA 落在哪个协议起始地址区间判断。原因见本节末尾。

来源文件：

- 点表：`config/iec104_config.json`。`ver` 为 2，`pts[]` 每项是 `commonAddr` 加 `pointInfo[]`；`pointInfo[]` 每项是五元组 `[name, ioa, type, changeReport, coefficient]`，字段语义见 `src/iec104_server/iec104_server.c:1068-1071`。工具取第 5 个字段作 `scale`。
- 区间常量：`src/automatic_north/iec104_create.c:64-67`（`start_RemotePoint/Meas/Ctrl/Regu`）。中文语义在 `src/automatic_north/iec104_create.h:40-43`。
- 运行时地址公式：`src/automatic_north/north_automatic.c:803`（RP）、`:813`（RM）、`:823`（RC）、`:837`（RR），形式为 `start + section_indx * RESERVE + nb_*`。分段常量在 `src/automatic_north/north_automatic.h:38-41`：RP 500、RM 300、RC 20、RR 500。
- CA 分配：`src/automatic_north/north_automatic.c:567-605`。PCS/BMS/CELL 每个设备单独分配 CA，`section_indx` 恒为 0。其他类（EMS、表计、DIDO 等）共用类的 `str_addr`，`section_indx` 逐设备递增。
- 运行时 map：服务加载 `/app/config/north/iec104_north_map.json`（`src/iec104_server/iec104_server.h:16`）。该 map 的 `pointInfo` 是 RemotePoint/RemoteMeas/RemoteCtrl/RemoteRegu 四个对象数组（`src/automatic_north/iec104_create.c:349-371`）。

关键来源差异：flat 配置没有 type 分组，运行时 map 有。从 flat 配置抽表时，kind 只能按 IOA 区间判定。另外，v3 文档把 `/app/config/iec104_config.json` 列为废弃配置（`docs/默认配置整理.md:233-234`），当前代码里没有读取这份 flat 文件的路径。抽取对象是仓库里的现场快照；快照的生成来源和版本未确认，列入 UNCHECKED。

## 3. kind 判定依据

| 常量 | 起始 | 覆盖 IOA | kind | 语义 | 证据 |
| --- | ---: | --- | --- | --- | --- |
| start_RemotePoint | 1 | 1..16384 | telemetry | 遥信 | `iec104_create.c:64`、`.h:40` |
| start_RemoteMeas | 16385 | 16385..24576 | telemetry | 遥测 | `iec104_create.c:65`、`.h:41` |
| start_RemoteCtrl | 24577 | 24577..25088 | control | 遥控 | `iec104_create.c:66`、`.h:42` |
| start_RemoteRegu | 25089 | 25089.. | control | 遥调 | `iec104_create.c:67`、`.h:43` |

判定规则：取不大于 IOA 的最大起始地址，kind 取该地址所属 kind；上界是下一个起始地址减一。IOA 小于第一个起始地址时为 unknown。区间起始地址重复视为用法错误，工具退出码 3。

控制点判定依据：

- 本配置没有 control 点。遥控/遥调 IOA 由运行时生成（`north_automatic.c:823`、`:837`），不写在 flat 配置里。
- 遥控点同时在遥信区间占一个上报位（`north_automatic.c:832`、`:884`）；遥调点同时在遥测区间占位（`north_automatic.c:846`、`:898`）。同一个控制对象因此会同时有一个 control IOA 和一个 telemetry 上报 IOA。
- 下列名字像控制或定值，但 IOA 在遥测区间，本表按地址判为 telemetry：`EMS.OnOff`、`EMS.PcsAutoTurn`、`EMS.PcsAutoTurnOffTime`、`EMS.SetConnect`、`EMS.SetRePower`、`EMS.SetPower`、`EMS.StratagyType`、`EMS.ControlMode`、`EMS.Adjpreriod`、`EMS.K1`、`EMS.K2`、`EMS.Pset`、`EMS.LocalCtrlPower`、`EMS.DemandStartMin/Hour`、`EMS.DemandEndMin/Hour`。现场是否另有遥控/遥调镜像，UNCHECKED。

## 4. 抽取结果

真实快照由第 6 节的命令生成，工具输出与这里一致。

| CA | points | telemetry | control | unknown |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 176 | 176 | 0 | 0 |
| 2 | 2 | 2 | 0 | 0 |
| total | 178 | 178 | 0 | 0 |

### 完整点表

| CA | name | IOA | type | scale | kind | evidence |
| ---: | --- | ---: | --- | ---: | --- | --- |
| 1 | EMS.BmsLimited | 1 | bool | 1.0 | telemetry | config/iec104_config.json:10; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.SysStoped | 2 | bool | 1.0 | telemetry | config/iec104_config.json:11; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.PowerReflux | 3 | bool | 1.0 | telemetry | config/iec104_config.json:12; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.PowerLimited | 4 | bool | 1.0 | telemetry | config/iec104_config.json:13; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.IoProtect | 5 | bool | 1.0 | telemetry | config/iec104_config.json:14; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.MeterError | 6 | bool | 1.0 | telemetry | config/iec104_config.json:15; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EMS.DcTotalVolUnusual | 7 | bool | 1.0 | telemetry | config/iec104_config.json:16; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.ProhibiteCha | 8 | bool | 1.0 | telemetry | config/iec104_config.json:17; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.ProhibiteDisc | 9 | bool | 1.0 | telemetry | config/iec104_config.json:18; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserStop1 | 10 | bool | 1.0 | telemetry | config/iec104_config.json:19; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserStop2 | 11 | bool | 1.0 | telemetry | config/iec104_config.json:20; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserStop3 | 12 | bool | 1.0 | telemetry | config/iec104_config.json:21; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserAlarm1 | 13 | bool | 1.0 | telemetry | config/iec104_config.json:22; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserAlarm2 | 14 | bool | 1.0 | telemetry | config/iec104_config.json:23; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.UserAlarm3 | 15 | bool | 1.0 | telemetry | config/iec104_config.json:24; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnBmsAlarm | 16 | bool | 1.0 | telemetry | config/iec104_config.json:25; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnProtectDOD | 17 | bool | 1.0 | telemetry | config/iec104_config.json:26; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnIoProtect | 18 | bool | 1.0 | telemetry | config/iec104_config.json:27; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnProtectTransf | 19 | bool | 1.0 | telemetry | config/iec104_config.json:28; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnTraceLoad | 20 | bool | 1.0 | telemetry | config/iec104_config.json:29; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.LoadTrackingLimit | 21 | bool | 1.0 | telemetry | config/iec104_config.json:30; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnProtectReverse | 22 | bool | 1.0 | telemetry | config/iec104_config.json:31; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnDraughtFanPower | 23 | bool | 1.0 | telemetry | config/iec104_config.json:32; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.EnDraughtFanTem | 24 | bool | 1.0 | telemetry | config/iec104_config.json:33; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.MeterInvert | 25 | bool | 1.0 | telemetry | config/iec104_config.json:34; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.ClrAlarm | 26 | bool | 1.0 | telemetry | config/iec104_config.json:35; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.OnOff | 27 | bool | 1.0 | telemetry | config/iec104_config.json:36; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 1 | EMS.DemandThreshold1 | 16385 | int | 1.0 | telemetry | config/iec104_config.json:37; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold2 | 16386 | int | 1.0 | telemetry | config/iec104_config.json:38; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold3 | 16387 | int | 1.0 | telemetry | config/iec104_config.json:39; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold4 | 16388 | int | 1.0 | telemetry | config/iec104_config.json:40; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold5 | 16389 | int | 1.0 | telemetry | config/iec104_config.json:41; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold6 | 16390 | int | 1.0 | telemetry | config/iec104_config.json:42; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold7 | 16391 | int | 1.0 | telemetry | config/iec104_config.json:43; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold8 | 16392 | int | 1.0 | telemetry | config/iec104_config.json:44; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold9 | 16393 | int | 1.0 | telemetry | config/iec104_config.json:45; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold10 | 16394 | int | 1.0 | telemetry | config/iec104_config.json:46; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold11 | 16395 | int | 1.0 | telemetry | config/iec104_config.json:47; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandThreshold12 | 16396 | int | 1.0 | telemetry | config/iec104_config.json:48; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.Pmlmax | 16396 | int | 1.0 | telemetry | config/iec104_config.json:49; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SOCmax | 16397 | int | 1.0 | telemetry | config/iec104_config.json:50; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SOCmin | 16398 | int | 1.0 | telemetry | config/iec104_config.json:51; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandErr | 16399 | int | 1.0 | telemetry | config/iec104_config.json:52; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.TempThreshold | 16400 | int | 1.0 | telemetry | config/iec104_config.json:53; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S1Power | 16401 | int | 1.0 | telemetry | config/iec104_config.json:54; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S1StMin | 16402 | int | 1.0 | telemetry | config/iec104_config.json:55; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S1StHour | 16403 | int | 1.0 | telemetry | config/iec104_config.json:56; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S1EnMin | 16404 | int | 1.0 | telemetry | config/iec104_config.json:57; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S1EnHour | 16405 | int | 1.0 | telemetry | config/iec104_config.json:58; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S2Power | 16406 | int | 1.0 | telemetry | config/iec104_config.json:59; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S2StMin | 16407 | int | 1.0 | telemetry | config/iec104_config.json:60; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S2StHour | 16408 | int | 1.0 | telemetry | config/iec104_config.json:61; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S2EnMin | 16409 | int | 1.0 | telemetry | config/iec104_config.json:62; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S2EnHour | 16410 | int | 1.0 | telemetry | config/iec104_config.json:63; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S3Power | 16411 | int | 1.0 | telemetry | config/iec104_config.json:64; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S3StMin | 16412 | int | 1.0 | telemetry | config/iec104_config.json:65; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S3StHour | 16413 | int | 1.0 | telemetry | config/iec104_config.json:66; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S3EnMin | 16415 | int | 1.0 | telemetry | config/iec104_config.json:67; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S3EnHour | 16416 | int | 1.0 | telemetry | config/iec104_config.json:68; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S4Power | 16417 | int | 1.0 | telemetry | config/iec104_config.json:69; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S4StMin | 16418 | int | 1.0 | telemetry | config/iec104_config.json:70; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S4StHour | 16419 | int | 1.0 | telemetry | config/iec104_config.json:71; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S4EnMin | 16420 | int | 1.0 | telemetry | config/iec104_config.json:72; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S4EnHour | 16421 | int | 1.0 | telemetry | config/iec104_config.json:73; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S5Power | 16422 | int | 1.0 | telemetry | config/iec104_config.json:74; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S5StMin | 16423 | int | 1.0 | telemetry | config/iec104_config.json:75; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S5StHour | 16424 | int | 1.0 | telemetry | config/iec104_config.json:76; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S5EnMin | 16425 | int | 1.0 | telemetry | config/iec104_config.json:77; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S5EnHour | 16426 | int | 1.0 | telemetry | config/iec104_config.json:78; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S6Power | 16427 | int | 1.0 | telemetry | config/iec104_config.json:79; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S6StMin | 16428 | int | 1.0 | telemetry | config/iec104_config.json:80; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S6StHour | 16429 | int | 1.0 | telemetry | config/iec104_config.json:81; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S6EnMin | 16430 | int | 1.0 | telemetry | config/iec104_config.json:82; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.S6EnHour | 16431 | int | 1.0 | telemetry | config/iec104_config.json:83; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.PcsAutoTurn | 16432 | int | 1.0 | telemetry | config/iec104_config.json:84; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.PcsAutoTurnOffTime | 16433 | int | 1.0 | telemetry | config/iec104_config.json:85; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SetConnect | 16434 | int | 1.0 | telemetry | config/iec104_config.json:86; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SetRePower | 16435 | int | 1.0 | telemetry | config/iec104_config.json:87; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SetPower | 16436 | int | 1.0 | telemetry | config/iec104_config.json:88; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.StratagyType | 16437 | int | 1.0 | telemetry | config/iec104_config.json:89; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.ControlMode | 16438 | int | 1.0 | telemetry | config/iec104_config.json:90; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.Adjpreriod | 16439 | int | 1.0 | telemetry | config/iec104_config.json:91; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.K2 | 16440 | int | 1.0 | telemetry | config/iec104_config.json:92; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.Pset | 16442 | int | 1.0 | telemetry | config/iec104_config.json:93; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandStartMin | 16443 | int | 1.0 | telemetry | config/iec104_config.json:94; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandStartHour | 16444 | int | 1.0 | telemetry | config/iec104_config.json:95; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandEndMin | 16445 | int | 1.0 | telemetry | config/iec104_config.json:96; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DemandEndHour | 16446 | int | 1.0 | telemetry | config/iec104_config.json:97; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.K1 | 16447 | int | 1.0 | telemetry | config/iec104_config.json:98; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DFanPowerThreShold | 16448 | int | 1.0 | telemetry | config/iec104_config.json:99; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.DFanTemThreShold | 16449 | int | 1.0 | telemetry | config/iec104_config.json:100; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.LocalCtrlPower | 16450 | int | 1.0 | telemetry | config/iec104_config.json:101; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.Income | 16451 | int | 1.0 | telemetry | config/iec104_config.json:102; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.Efficiency | 16452 | int | 1.0 | telemetry | config/iec104_config.json:103; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.ExpectPower | 16453 | int | 1.0 | telemetry | config/iec104_config.json:104; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.SysStatus | 16454 | int | 1.0 | telemetry | config/iec104_config.json:105; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.CabPower | 16455 | int | 1.0 | telemetry | config/iec104_config.json:106; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.CabCapacity | 16456 | int | 1.0 | telemetry | config/iec104_config.json:107; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.GridPower | 16457 | float | 1.0 | telemetry | config/iec104_config.json:108; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | EMS.LoadPower | 16458 | float | 1.0 | telemetry | config/iec104_config.json:109; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.ActiveEnergy | 16459 | float | 1.0 | telemetry | config/iec104_config.json:110; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.ReactiveEnergy | 16460 | float | 1.0 | telemetry | config/iec104_config.json:111; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphaseVoltage | 16461 | float | 1.0 | telemetry | config/iec104_config.json:112; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphaseVoltage | 16462 | float | 1.0 | telemetry | config/iec104_config.json:113; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphaseVoltage | 16463 | float | 1.0 | telemetry | config/iec104_config.json:114; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphaseCurrent | 16464 | float | 1.0 | telemetry | config/iec104_config.json:115; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphaseCurrent | 16465 | float | 1.0 | telemetry | config/iec104_config.json:116; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphaseCurrent | 16466 | float | 1.0 | telemetry | config/iec104_config.json:117; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.ActivePower | 16467 | float | 1.0 | telemetry | config/iec104_config.json:118; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphaseActivePower | 16468 | float | 1.0 | telemetry | config/iec104_config.json:119; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphaseActivePower | 16469 | float | 1.0 | telemetry | config/iec104_config.json:120; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphaseActivePower | 16470 | float | 1.0 | telemetry | config/iec104_config.json:121; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.ReactivePower | 16471 | float | 1.0 | telemetry | config/iec104_config.json:122; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphaseReactivePower | 16472 | float | 1.0 | telemetry | config/iec104_config.json:123; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphaseReactivePower | 16473 | float | 1.0 | telemetry | config/iec104_config.json:124; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphaseReactivePower | 16474 | float | 1.0 | telemetry | config/iec104_config.json:125; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.ApparentPower | 16475 | float | 1.0 | telemetry | config/iec104_config.json:126; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphaseApparentPower | 16476 | float | 1.0 | telemetry | config/iec104_config.json:127; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphaseApparentPower | 16477 | float | 1.0 | telemetry | config/iec104_config.json:128; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphaseApparentPower | 16478 | float | 1.0 | telemetry | config/iec104_config.json:129; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.PowerFactor | 16479 | float | 1.0 | telemetry | config/iec104_config.json:130; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.AphasePowerFactor | 16480 | float | 1.0 | telemetry | config/iec104_config.json:131; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.BphasePowerFactor | 16481 | float | 1.0 | telemetry | config/iec104_config.json:132; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | GRID_METER.CphasePowerFactor | 16482 | float | 1.0 | telemetry | config/iec104_config.json:133; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ActiveEnergy | 16483 | float | 1.0 | telemetry | config/iec104_config.json:134; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.PositiveActiveRate1 | 16484 | float | 1.0 | telemetry | config/iec104_config.json:135; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.PositiveActiveRate2 | 16485 | float | 1.0 | telemetry | config/iec104_config.json:136; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.PositiveActiveRate3 | 16486 | float | 1.0 | telemetry | config/iec104_config.json:137; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.PositiveActiveRate4 | 16487 | float | 1.0 | telemetry | config/iec104_config.json:138; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReactiveEnergy | 16488 | float | 1.0 | telemetry | config/iec104_config.json:139; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReverseActiveRate1 | 16489 | float | 1.0 | telemetry | config/iec104_config.json:140; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReverseActiveRate2 | 16490 | float | 1.0 | telemetry | config/iec104_config.json:141; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReverseActiveRate3 | 16491 | float | 1.0 | telemetry | config/iec104_config.json:142; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReverseActiveRate4 | 16492 | float | 1.0 | telemetry | config/iec104_config.json:143; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphaseVoltage | 16493 | float | 1.0 | telemetry | config/iec104_config.json:144; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphaseVoltage | 16494 | float | 1.0 | telemetry | config/iec104_config.json:145; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphaseVoltage | 16495 | float | 1.0 | telemetry | config/iec104_config.json:146; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphaseCurrent | 16496 | float | 1.0 | telemetry | config/iec104_config.json:147; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphaseCurrent | 16497 | float | 1.0 | telemetry | config/iec104_config.json:148; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphaseCurrent | 16498 | float | 1.0 | telemetry | config/iec104_config.json:149; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ActivePower | 16499 | float | 1.0 | telemetry | config/iec104_config.json:150; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphaseActivePower | 16500 | float | 1.0 | telemetry | config/iec104_config.json:151; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphaseActivePower | 16501 | float | 1.0 | telemetry | config/iec104_config.json:152; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphaseActivePower | 16502 | float | 1.0 | telemetry | config/iec104_config.json:153; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ReactivePower | 16503 | float | 1.0 | telemetry | config/iec104_config.json:154; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphaseReactivePower | 16504 | float | 1.0 | telemetry | config/iec104_config.json:155; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphaseReactivePower | 16505 | float | 1.0 | telemetry | config/iec104_config.json:156; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphaseReactivePower | 16506 | float | 1.0 | telemetry | config/iec104_config.json:157; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.ApparentPower | 16507 | float | 1.0 | telemetry | config/iec104_config.json:158; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphaseApparentPower | 16508 | float | 1.0 | telemetry | config/iec104_config.json:159; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphaseApparentPower | 16509 | float | 1.0 | telemetry | config/iec104_config.json:160; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphaseApparentPower | 16510 | float | 1.0 | telemetry | config/iec104_config.json:161; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.PowerFactor | 16511 | float | 1.0 | telemetry | config/iec104_config.json:162; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.AphasePowerFactor | 16512 | float | 1.0 | telemetry | config/iec104_config.json:163; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.BphasePowerFactor | 16513 | float | 1.0 | telemetry | config/iec104_config.json:164; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS_METER.CphasePowerFactor | 16514 | float | 1.0 | telemetry | config/iec104_config.json:165; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.AphaseVoltage | 16515 | float | 1.0 | telemetry | config/iec104_config.json:166; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.BphaseVoltage | 16516 | float | 1.0 | telemetry | config/iec104_config.json:167; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.CphaseVoltage | 16517 | float | 1.0 | telemetry | config/iec104_config.json:168; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.AphaseCurrent | 16518 | float | 1.0 | telemetry | config/iec104_config.json:169; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.BphaseCurrent | 16519 | float | 1.0 | telemetry | config/iec104_config.json:170; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.CphaseCurrent | 16520 | float | 1.0 | telemetry | config/iec104_config.json:171; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.AphaseActPower | 16521 | float | 1.0 | telemetry | config/iec104_config.json:172; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.BphaseActPower | 16522 | float | 1.0 | telemetry | config/iec104_config.json:173; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.CphaseActPower | 16523 | float | 1.0 | telemetry | config/iec104_config.json:174; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.ActivePower | 16524 | float | 1.0 | telemetry | config/iec104_config.json:175; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.ReactivePower | 16525 | float | 1.0 | telemetry | config/iec104_config.json:176; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.ApparentPower | 16526 | float | 1.0 | telemetry | config/iec104_config.json:177; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | PCS.Frequency | 16527 | float | 1.0 | telemetry | config/iec104_config.json:178; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.RackVoltage | 16528 | float | 1.0 | telemetry | config/iec104_config.json:179; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.RackCurrent | 16529 | float | 1.0 | telemetry | config/iec104_config.json:180; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.RackRunState | 16530 | int | 1.0 | telemetry | config/iec104_config.json:181; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.RackConnectionState | 16531 | int | 1.0 | telemetry | config/iec104_config.json:182; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.RackCagDsgState | 16532 | int | 1.0 | telemetry | config/iec104_config.json:183; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.SOC | 16533 | int | 1.0 | telemetry | config/iec104_config.json:184; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 1 | BMS.SOH | 16534 | int | 1.0 | telemetry | config/iec104_config.json:185; src/automatic_north/iec104_create.c:65 start_RemoteMeas=16385 |
| 2 | EMS.BmsLimited | 1 | bool | 1.0 | telemetry | config/iec104_config.json:192; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |
| 2 | EMS.SysStoped | 2 | bool | 1.0 | telemetry | config/iec104_config.json:193; src/automatic_north/iec104_create.c:64 start_RemotePoint=1 |

## 5. 异常、风险和 UNCHECKED

异常：

1. CA 1 的 IOA 16396 重复：`EMS.DemandThreshold12` 和 `EMS.Pmlmax`。运行时按 IOA 建立 `point_map_hash`（`src/iec104_server/iec104_server.c:1075`），重复键的实际生效者未验证。
2. IOA 16414、16441 空缺。16414 在 `S3StHour`(16413) 与 `S3EnMin`(16415) 之间，16441 在 `K2`(16440) 与 `Pset`(16442) 之间。不是抽取错误。
3. CA 2 的 2 个点与 CA 1 的前 2 个点同名同 IOA（`EMS.BmsLimited` 1、`EMS.SysStoped` 2），只按 CA 区分。主站侧是否允许同名点，属于现场约定。
4. `iec104_create.c:46-58` 的注释块给出的设备地址映射与 `_iec_home_page`（`:26-42`）和 `iec_class_info`（`:68-84`）不一致。注释块写 PCS 2..17、主控 18..33、电芯 49..64；代码表写 PCS 2..33、BMS 34..97、CELL 98..161。注释块不能作为地址依据。
5. flat 配置在 v3 文档中标注废弃（`docs/默认配置整理.md:233-234`），当前代码只加载运行时 map（`iec104_server.h:16`）。

风险：

- kind 边界假设遥测地址不进入遥控区间。RM RESERVE 为 300，非 PCS/BMS/CELL 类按 `section_indx` 递增（`north_automatic.c:586-605`）。`section_indx ≥ 28` 时 `start_RemoteMeas + section_indx * 300` 必然达到 24577 以上；`section_indx = 27` 时若 `nb_rm ≥ 92` 也会进入。本表会把这类遥测点判成 control。本配置最大 IOA 为 16534，不受影响；现场大站要按实际 `section_indx` 复核。证据：`north_automatic.h:38-41`、`north_automatic.c:813`、`iec104_create.c:65-66`。
- 若现场用运行时 map 而不是 flat 配置，应直接按 map 的四个 type 数组判 kind。本表的区间推断只适用于 flat 配置。

UNCHECKED（站点专属，需现场表或 `iec104_north_map.json` 确认）：

- 站点控制表：仓库内没有 IOA ≥ 24577 的遥控/遥调点。站点开关、定值、功率设定点的 IOA 和 type 必须从现场 map 或主站点表补。
- 多 CA 现场差异：本配置只有 CA 1、CA 2。现场 CA 数量、每个 CA 的设备类型、PCS/BMS/CELL 的逐设备 CA 分配、其他类的 `section_indx` 递增，都要按现场配置确认。
- CA 2 的设备身份：CA 2 承载 `EMS.*` 点。按当前代码（`north_automatic.c:567-605`），非 PCS/BMS/CELL 类（含 EMS）的 CA 固定为该类的 `str_addr`，第二个 EMS 设备仍是 CA 1，且 `section_indx` 递增会把 IOA 推到 500 起，不是 1 起；PCS/BMS/CELL 类才会逐设备递增 CA，但 `section_indx` 恒为 0。CA 2 是 PCS/主控类的设备（tag 前缀为 EMS）、旧配置残留，还是别的来源，未确认。
- 名字像控制的遥测点是否在现场另有控制镜像：`EMS.OnOff`、`EMS.SetPower`、`EMS.Pset`、`EMS.ControlMode` 等（见第 3 节列表）。
- IOA 16396 重复点的生效者：`EMS.DemandThreshold12` 还是 `EMS.Pmlmax`。
- flat 配置与运行时 map 的版本对应关系：flat 文件已被 v3 标注废弃，现场实际下发的 map 是否由它生成，未确认。
- 注释块与代码表的地址映射冲突：现场以哪张表为准。

## 6. 复现

```
python3 tools/migration/iec104_points.py \
  --config /home/edge/iems_v3/config/iec104_config.json \
  --source-root /home/edge/iems_v3 \
  --out tools/migration/golden/iec104_points
python3 tools/migration/iec104_points.py \
  --config /home/edge/iems_v3/config/iec104_config.json \
  --source-root /home/edge/iems_v3 \
  --check tools/migration/golden/iec104_points
python3 tools/migration/iec104_points.py --selftest
```

`--check` 逐字节比较 `iec104_config.tsv` 和 `iec104_config.md`。`--selftest` 使用 `tools/migration/golden/iec104_points/synthetic/` 的合成配置和合成 C 源（起始地址 1/100/200/300），覆盖 telemetry、control、unknown、重复 IOA、证据行号、漂移检测；`/home/edge/iems_v3` 存在时同时核对真实快照的 2 CA/178 点。
