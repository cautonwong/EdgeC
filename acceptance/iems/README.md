# iems 验收

台架:产品壳 `iems_product.py` 在 `IEMS_RUN_DIR/vbus0.sock` 当 Modbus RTU 从站
(或按 `--transport tcp/mbap` 在 TCP 上当 RTU/MBAP 从站),
把请求/写记进同目录的 `bus.json`;北向把 iems 的 unix 从站桥成
`127.0.0.1:19211`,平台按 `modbus_rtu/tcp` 读写。判据落在产品 REST、南向
`bus.json` 和平台设备模型三处,不只看协议应答。

重启任务的控制面也在 `IEMS_RUN_DIR`:`start.request` 放产品启动、
`restart.request` 触发优雅冷重启(SIGTERM)、`crash.request` 触发 SIGKILL 重启,
`product.pid` / `restarts.json` 记录进程号、退出码与通道(mode=graceful/crash)。
产品输出在 `iems.log`,跨重启追加。

## 跑法

```bash
pip install -r acceptance/iems/requirements.txt   # 另需 apt install mosquitto
EDGEC_BUILD_DIR=<构建目录> TESTPLATFORM=<test-platform> PYTHON=<venv>/bin/python \
  ./acceptance/run.sh iems
```

只跑某个任务时把 `iems` 换成任务文件所在的产品名仍是 `iems`;平台按
`tasks/*.yaml` 逐个跑,`select` 在任务文件里。

## 覆盖

| 任务 | 用例 | 验什么 |
| --- | --- | --- |
| `iems_acceptance` | `I_00_00_01_000001..4` | REST 健康检查、周期轮询、电压落到实时接口、北向镜像等于台架值 |
| `iems_acceptance` | `I_00_00_03_000001..6` | MQTT 北向:节点身份、点位名、读数、下行执行、失败回执 |
| `iems_acceptance` | `I_00_00_03_000007..8` | 北向 UDS 生命周期:5 次连接/断开后产品 fd 不涨(±1);第一路保持时第二路被拒且第一路仍能读镜像电压 |
| `iems_acceptance`, `iems_control_route` | `I_00_00_04_000001` | 控制写按 poll_tasks/inventory 路由到 unit 3、不写 unit 1、无路由的写回 Modbus 异常 |
| `iems_acceptance`(经 `tag:验收`), `iems_iec104` | `I_00_00_05_000001` | IEC104 北向端到端:STARTDT/STOPDT/TESTFR 逐字节 ACT→CON、第二路客户端被拒且第一路存活、断开后新主站接管、总召 ACT_CON→数据→ACT_TERM(IOA 16386 = 台架电压)、负 ACT_CON(QOI/CA)、命令 COT 47 且无南向写、畸形 APCI 不崩、自发上报 COT=3 |
| `iems_transport_uart` | `I_00_00_06_000001` | 真实 UART 形态:产品按 `/dev/pts/N` + `OMNIS_BUS_0_BAUD=9600` 打开总线,PTY 上周期轮询、REST 电压 = 231.7V、北向写 220.0V 变成南向 FC06 地址 0 值 2200,写完还原;应答按两段写,逼出主站的帧重组 |
| `iems_transport_tcp` | `I_00_00_06_000002` | `tcp://127.0.0.1:<port>` 传输:同一组判据压在 TCP 流上,线上必须是 RTU 帧(不是 MBAP),应答同样拆两段写 |
| `iems_transport_mbap` | `I_00_00_06_000003` | Modbus TCP 帧:`inventory.protocol=ModbusTCP`,台架只按 7 字节 MBAP 头收发(协议号 0、长度字段自洽),同一组采集/写判据;启动日志必须出现 `Bus 0 CONNECTED via TCP`。读值量纲与 RTU 共用同一张点表(`raw×scaler`),台架 2317 还原为 231.7V |
| `iems_transport_tcp_reconnect` | `I_00_00_06_000004` | TCP 断链重连已覆盖:台架 `--drop-after 8` 断链(首轮轮询已完成),产品日志必须出现 `connection lost, retry #` 并按退避重连;PID 不变、`restarts.json` 为空(是 link 重连不是产品重启);重连后 `bus.json` 帧数继续增长、REST 时间戳前进、电压仍 231.7V |
| `iems_restart` | `I_00_00_07_000001` | 重启/恢复:启动闸门让观察器先订上 → 首个 BIRTH;`restart.request` 后优雅退出(3 秒内、退出码 0)、进程号变化、新 MQTT 会话重发 BIRTH;REST 与北向镜像恢复 231.7V,WAL 回放条数 > 0 |
| `iems_crash_restart` | `I_00_00_07_000002` | SIGKILL 崩溃重启:`crash.request` 直接杀进程(mode=crash、退出码 -9),同一 env 重启;第二次启动 WAL 回放条数 > 0、REST 与北向镜像恢复 231.7V、南向读帧继续增长、新 MQTT 会话重发 BIRTH;台架无 `_fail` 记录 |
| `iems_boot_guard` | `I_00_00_01_000010` | 南向全连不上时启动失败,不进重连循环 |
| `iems_fault_badcrc` | `I_00_00_01_000020` | CRC 错的应答整帧丢弃 |
| `iems_fault_slow` | `I_00_00_01_000021` | 应答超时不把 0 写成读数,下一轮恢复 |
| `iems_fault_slow_pollback` | `I_00_00_01_000022` | 应答 1.2s(低于 2s 超时、慢于 1s 轮询)时会积压:迟到帧不许串到下一个 tag,1001 始终 231.7V、1002 的 ts 继续前进、读地址仍是 0/1002 |
| `iems_fault_slow_timeout` | `I_00_00_01_000023` | 应答 3s(真实超过 2s 超时):超时后的恢复窗口先等迟到帧落地再由 `DISCARD_RX` 排空,1001 不被 1002 的 0 覆盖、1002 保持台架 0 且 ts 继续前进、读地址仍是 0/1002 |
| `iems_link_loss` | `I_00_00_01_000030` | 断链后停止发布新数据,服务不退出 |
| `iems_partial_bus` | `I_00_00_01_000040` | 部分总线连不上时启动汇总行完整、点名缺哪一路 |

`iems_acceptance` 与 `iems_control_route` 都以 `--slaves 1,3` 启动:设备 1
采集 tag 1001/1002,设备 3 挂 tag 1003 的轮询任务,控制写才有唯一路由。
只有一个从站时,把从站地址写死也能绿,所以路由用例必须两个从站。

## 南向传输任务的收集期隔离

传输用例也带 `tag:验收`(它们就是验收的一部分),但 `iems_acceptance` 固定跑
UDS 壳,直接收集会在错误的传输上断言。产品壳把本次传输档位写进
`RUN_DIR/transport.txt`,用例用 `@pytest.mark.transport("uart"/"tcp"/"mbap"/"restart")`
声明所需档位,`conftest.py` 的收集钩子只保留匹配的那一条。这是"这一轮不选它"
(与 `--select` 同类):`iems_acceptance` 的 run-scope 里没有这条用例,
各条用例分别在各自任务里真跑。不用运行时 skip —— skip 会计入未测,把任务
结论拖灰。`restart` 档由产品壳的 `--start-gate` 写入,它同时是重启任务的
启动闸门(先让观察器订上 MQTT 再起产品)。

## IEC104 北向的验收面

产品在契约表 bus 11 上自己起 IEC104 TCP 从站(`cmd/iems/iems_app.c` 调
`iems_north_iec104_init`),端口由 `IEMS_IEC104_PORT` 给。产品壳
`iems_product.py` 启动前挑一个空闲端口写进 `RUN_DIR/iec104_port.txt`,用例与
产品读的是同一个值;写死 2404 在并行任务或残留进程下会撞端口。

test-platform 没有 IEC104 驱动,且 `/home/edge/test-platform` 只读;验收因此
在项目里带一份最小主站 `iec104_client.py`(APCI + 类型 13/100 的 ASDU)。
它只覆盖断言用到的帧,不是通用库,也不参与平台设备模型。

**平台门禁拿不到这条的设备级结论**:自定义客户端不产生平台设备证据,所以
`iems_iec104` 是 `kind: smoke`(失败会单列,但不算某台设备的准入)。设备级
结论仍由 `iems_acceptance` 的 Modbus 北向镜像那几条给出。

## 剩余缺口

- IEC104 的**正 C_SC 路径**还没有可验的控制点:产品控制点表
  (`cmd/iems/iems_driver_adapter.c` 的 `s_iec104_controls`)只有哨兵行,
  IOA 24577 必然回 COT 47。现场 xlsx 入库后,本用例要把命令断言从"拒绝 +
  无南向写"改成"回 COT 7 + 南向写落到唯一从站"。
- IEC104 只验了电压越死区(0.5V)的一次自发上报;周期上报(COT=1)、S 帧、
  t3 主动 TESTFR、STOPDT、多 CA、类型 11/1 的过程点都未覆盖。
- 真实 `/dev/ttyS*` 硬件与 RS485 电气层未覆盖:UART 用例走 PTY,验的是启动
  选路、termios/波特率配置与 RTU 帧收发;线缆、终端电阻、总线冲突和真实
  波特率下的时序仍只能上硬件。
- **DLT645 南向的产品级验收被地址登记挡住**:产品从未调用
  `edge_api_set_protocol_meta("DLT645", ...)`(全仓只有 `cmd/dcu` 的适配层与
  单测调用),驱动拿不到 6 字节 BCD 表地址时回落到广播地址 0x99×6
  (`adapters/io_drivers/src/driver_dlt645.c` 的 `memset(p->addr_bcd, 0x99, ...)`)。
  此时台架就算应答,也只能证明"广播读能通",证明不了产品会按现场表地址寻址 ——
  那是假绿,因此不补 DLT645 产品验收。修法是把表地址从配置登记进
  `edge_api_set_protocol_meta`(与 DCU 的 `s_meter_addr_bcd` 同一形状)后再来。
- 重启用例只验了单次冷启动后的 BIRTH 与 WAL 恢复;连续多次重启、快照文件
  (5 分钟 compaction 才写)与重启期间主站下行缓冲仍未覆盖。
- 多从站台架目前只认 `bench.TAGS_BY_SLAVE` 里的 1/3 两台;加从站要同时补
  点位和 poll_tasks。
