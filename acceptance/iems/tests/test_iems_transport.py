"""南向真实传输验收:UART(PTY)、TCP 上的 Modbus RTU,与 Modbus TCP(MBAP)。

UDS 用例只能证明"同一台机器上的 socket 会应答":连接必然成功、帧边界由内核
保证、没有波特率、没有启动选路。生产现场是 RS485 UART 或 tcp:// 网关 ——
产品要先按路径/类型选传输(`iems_southbound_transport_for`),再打开真实设备
(termios/波特率)或在 TCP 流上按 RTU 拆帧。这几段只有真实传输形态才走得到。

MBAP 档换的是**帧格式**:inventory.protocol=ModbusTCP 让产品选 ModbusTCP 驱动,
台架按 7 字节 MBAP 头收发。它与 RTU over TCP 的区别正是现场最常配错的一处。

判据沿用 UDS 用例的同一套证据,每条传输各一份:
  1. 产品在**这条传输上**发出周期读请求(bus.json 的 requests 在增加)
  2. REST 报的电压等于台架给的值(±0.5V)
  3. 北向写 220.0V 必须变成南向 FC06、地址 0、值 2200 —— 写应答只是回显,
     判据落在台架收到的那一帧上;写完还原 231.7,不留脏读数

每条传输由自己的任务启动(`tasks/iems_transport_uart.yaml` /
`iems_transport_tcp.yaml` / `iems_transport_mbap.yaml`);收集期按
`RUN_DIR/transport.txt` 只留匹配的那条,UDS 的验收任务不会把它们收进来
(见 conftest 的 transport 标记处理)。TCP 还有一份断链任务
(`tasks/iems_transport_tcp_reconnect.yaml`):台架按 `--drop-after` 关连接,
验的是产品的退避重连与轮询恢复,不是再验一遍传输本身。
"""
import json
import struct
import time

import pytest

from bench import (DROPPED_LOG, RUN_DIR, TAG_VOLTAGE, VOLTAGE, bus_seen,
                   dropped, latest, port_of)
from testplatform.cases import case
from testplatform.protocol.modbus_rtu import WRITE_SINGLE_REGISTER

TOLERANCE_V = 0.5
WAIT_S = 15.0


@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    """保住证据会话:没有它 sessionfinish 不写 run-scope.json。"""
    return evidence_session


def _port() -> int:
    deadline = time.time() + 10
    while time.time() < deadline:
        try:
            return port_of()
        except (FileNotFoundError, ValueError):
            time.sleep(0.2)
    pytest.fail("产品壳没有写出监听端口,服务没起来")


def _wait_voltage() -> float | None:
    deadline = time.time() + WAIT_S
    got = None
    while time.time() < deadline:
        value = latest(_port(), TAG_VOLTAGE).get("value")
        if value is not None:
            got = float(value)
            if abs(got - VOLTAGE) <= TOLERANCE_V:
                return got
        time.sleep(0.3)
    return got


def _assert_polls_over_the_transport() -> None:
    """台架只在这条传输上服务,所以 bus.json 的帧就是产品真的走了它。

    帧数必须还在增加:文件里有帧可能只是启动那一帧,周期轮询才是采集。
    """
    seen = bus_seen()
    assert seen["requests"] > 0, (
        "台架在这条传输上一帧请求都没收到 —— 产品没走这个传输,或者启动选路失败"
        "(看 product.out 的 PHYSICS-SUCCESS / NOT connected 行)"
    )
    deadline = time.time() + WAIT_S
    while time.time() < deadline:
        if bus_seen()["requests"] > seen["requests"]:
            break
        time.sleep(0.5)
    else:
        pytest.fail(
            f"{WAIT_S:.0f} 秒内帧数没有增加(停在 {seen['requests']} 帧),不是周期轮询"
        )
    reads = bus_seen().get("reads") or []
    assert any(r["unit"] == 1 and r["address"] == 0 for r in reads), (
        f"轮询里没有 unit 1 地址 0(电压)的读;收到的是 {reads[:5]}"
    )


def _writes() -> list:
    return bus_seen().get("writes") or []


def _wait_write(before: int) -> dict:
    deadline = time.time() + WAIT_S
    while time.time() < deadline:
        new = _writes()[before:]
        if new:
            return new[0]
        time.sleep(0.2)
    pytest.fail(
        f"北向写之后 {WAIT_S:.0f} 秒内南向台架没收到任何写 —— "
        "北向从站回显成功不能证明写下发了,这一帧才是"
    )


def _assert_voltage_write_lands(meter) -> float:
    """北向 220.0V 必须变成南向 FC06 地址 0 值 2200,并还原 2317。

    返回还原写的台架时间戳:调用方用它等**下一轮轮询**,别把控制写的影子值
    当成轮询读数。
    """
    meter.connect()
    before = len(_writes())
    try:
        meter.write("电压", 220.0)
        got = _wait_write(before)
        assert got["unit"] == 1 and got["address"] == 0, (
            f"南向写落在 {got},期望 unit 1 地址 0(电压)"
        )
        assert got["function"] == WRITE_SINGLE_REGISTER, (
            f"南向写功能码是 0x{got['function']:02X},期望 FC06(单寄存器)"
            " —— 点位是 1 个寄存器,退化/升格成都说明宽度映射不对"
        )
        assert got["values"] == [2200], (
            f"南向寄存器是 {got['values']},期望 [2200](220.0V × 0.1)"
        )
    finally:
        # 台架寄存器就是别的用例的读数:不还原,后面的用例会看到 220V
        restore_before = len(_writes())
        meter.write("电压", VOLTAGE)

    restored = _wait_write(restore_before)
    assert restored["values"] == [2317], (
        f"还原写是 {restored['values']},期望 [2317](231.7V × 0.1)"
    )
    return restored["at"]


def _assert_voltage_restored() -> None:
    deadline = time.time() + WAIT_S
    value = None
    while time.time() < deadline:
        value = latest(_port(), TAG_VOLTAGE).get("value")
        if value is not None and abs(float(value) - VOLTAGE) <= TOLERANCE_V:
            return
        time.sleep(0.3)
    pytest.fail(f"还原 {VOLTAGE}V 后 REST 没回到台架值,最后读数 {value}")


def _assert_voltage_write_roundtrip(meter) -> None:
    _assert_voltage_write_lands(meter)
    _assert_voltage_restored()


def _wait_polled_voltage(after: float) -> float | None:
    """等 `after` 之后的第一轮南向读,再取 REST 值。

    北向控制写会直接改影子(工程值),不等下一轮轮询就读,读到的不是采集链路
    的结果 —— 那样测出来的绿是假的。这里只认台架真的收到过 FC03 地址 0 之后的读数。
    """
    deadline = time.time() + WAIT_S
    while time.time() < deadline:
        reads = [
            r for r in (bus_seen().get("reads") or [])
            if r["address"] == 0 and r["at"] > after
        ]
        if reads:
            time.sleep(0.3)  # 应答到 datahub 落地还有几毫秒
            value = latest(_port(), TAG_VOLTAGE).get("value")
            return None if value is None else float(value)
        time.sleep(0.2)
    pytest.fail(f"还原写之后 {WAIT_S:.0f}s 内没等到新的电压轮询帧")


@case(id="I_00_00_06_000001", clause="IEMS-TRANSPORT-UART", tags=["验收", "传输"])
@pytest.mark.transport("uart")
@pytest.mark.contract
@pytest.mark.requires_capability("电压")
def test_uart_transport_polls_and_writes(meter):
    """产品必须经 PTY(/dev/pts/N)按真实 UART 打开总线并完成采集与下行写。

    UDS 用例看不到这一段:socket 永远连得上、帧边界由内核保证、没有 termios
    和波特率。UART 选路(type=UART + /dev/ 路径)、打开设备、RTU 收发的任何
    一环退化,这里都会红 —— 而 UDS 验收仍然全绿。
    """
    _assert_polls_over_the_transport()
    got = _wait_voltage()
    assert got is not None, "REST 一直没返回相位 A 电压"
    assert got == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"PTY 上给了 {VOLTAGE}V,REST 读到 {got}V —— 值没有沿 UART 采集链路落下"
    )
    _assert_voltage_write_roundtrip(meter)


@case(id="I_00_00_06_000002", clause="IEMS-TRANSPORT-TCP", tags=["验收", "传输"])
@pytest.mark.transport("tcp")
@pytest.mark.contract
@pytest.mark.requires_capability("电压")
def test_tcp_transport_polls_and_writes(meter):
    """产品必须按 tcp://127.0.0.1:<port> 建立南向连接,在 TCP 流上收发 RTU 帧。

    这一档验的不是 Modbus TCP 的 MBAP:产品总线 0 的协议仍是 ModbusRTU,
    tcp:// 只换传输。驱动若退化成 MBAP,或按一次 read 的长度当帧边界,
    UDS 用例都看不出来,这里会红。
    """
    _assert_polls_over_the_transport()
    got = _wait_voltage()
    assert got is not None, "REST 一直没返回相位 A 电压"
    assert got == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"TCP 上给了 {VOLTAGE}V,REST 读到 {got}V —— 值没有沿 TCP 采集链路落下"
    )
    _assert_voltage_write_roundtrip(meter)


def _wait_log_line(needle: str, count: int = 1, timeout: float = WAIT_S) -> None:
    """日志里必须有产品自己打的这一行(文案由产品给,不由用例编)。

    启动与重连共用:count > 1 用来等同一行第 N 次出现(第二次 CONNECTED)。
    """
    log = RUN_DIR / "iems.log"
    deadline = time.time() + timeout
    text = ""
    while time.time() < deadline:
        try:
            text = log.read_text(encoding="utf-8", errors="replace")
        except FileNotFoundError:
            text = ""
        if text.count(needle) >= count:
            return
        time.sleep(0.2)
    tail = "\n".join(text.splitlines()[-5:]) if text else "(iems.log 不存在)"
    pytest.fail(f"{timeout:.0f}s 内日志里没有第 {count} 次 {needle!r};最后几行:\n{tail}")


def _assert_mbap_framing() -> None:
    """台架收到并回过的必须是真的 MBAP 帧,不是 CRC 结尾的 RTU 帧。

    MBAP 头:事务号 2 | 协议号 2(=0)| 长度 2(= 单元号 + PDU)| 单元号 1。
    长度字段与整帧对不上,或协议号不是 0,就说明线上不是 Modbus TCP。
    """
    raw = bus_seen().get("first_request_hex")
    assert raw, f"台架一帧请求都没记到: {bus_seen()}"
    frame = bytes.fromhex(raw)
    assert len(frame) >= 9, f"首帧 {len(frame)} 字节,短于 MBAP 头 + PDU: {raw}"
    _tid, proto, length, unit = struct.unpack(">HHHB", frame[:7])
    assert proto == 0, f"MBAP 协议号是 {proto}(必须 0),线上不是 Modbus TCP: {raw}"
    assert length == len(frame) - 6, (
        f"MBAP 长度字段 {length} 与整帧 {len(frame)} 字节对不上"
        f"(应为 {len(frame) - 6}): {raw}"
    )
    assert unit == 1, f"MBAP 单元号是 {unit},期望 1: {raw}"


@case(id="I_00_00_06_000003", clause="IEMS-TRANSPORT-MBAP", tags=["验收", "传输"])
@pytest.mark.transport("mbap")
@pytest.mark.contract
@pytest.mark.requires_capability("电压")
def test_mbap_transport_polls_and_writes(meter):
    """产品必须按 inventory.protocol=ModbusTCP 走 MBAP 帧完成采集与下行写。

    RTU over TCP 与 Modbus TCP 的差别就在帧格式:前者是 CRC 结尾的 RTU 帧,
    后者是 7 字节 MBAP 头 + 无 CRC 的 PDU。台架的 MBAP 从站只认后者 ——
    协议选错时产品会在第一帧就超时,而不是"看起来通、值却不对"。
    """
    _wait_log_line("Bus 0 CONNECTED via TCP")
    _assert_polls_over_the_transport()
    _assert_mbap_framing()
    restored_at = _assert_voltage_write_lands(meter)
    got = _wait_polled_voltage(restored_at)
    assert got is not None, "REST 一直没返回相位 A 电压"
    assert got == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"MBAP 上给了 {VOLTAGE}V,REST 读到 {got}V —— 值没有沿 Modbus TCP 采集链路落下"
    )


RECONNECT_WAIT_S = 20.0


def _read_pid() -> int:
    path = RUN_DIR / "product.pid"
    deadline = time.time() + 10
    while time.time() < deadline:
        try:
            return int(path.read_text(encoding="utf-8").strip())
        except (OSError, ValueError):
            time.sleep(0.05)
    pytest.fail(f"产品壳没有写出进程号({path})")


def _wait_drop(timeout: float = 30.0) -> dict:
    """等台架按 --drop-after 主动关掉南向连接。文件缺失 = 还没断。"""
    deadline = time.time() + timeout
    while time.time() < deadline:
        got = dropped()
        if got:
            return got
        time.sleep(0.2)
    pytest.fail(
        f"{timeout:.0f}s 内台架没有断链记录({DROPPED_LOG}):总线只看到 "
        f"{bus_seen()['requests']} 帧,产品没跑到 --drop-after 那一帧"
    )


@case(id="I_00_00_06_000004", clause="IEMS-TRANSPORT-TCP-RECONNECT",
      tags=["验收", "传输", "链路"])
@pytest.mark.transport("tcp")
@pytest.mark.contract
@pytest.mark.requires_capability("电压")
def test_tcp_transport_reconnects_after_link_loss(meter):
    """台架在第 8 帧后断开 TCP:产品必须按退避重连,不是重启,也不能停在断链态。

    判据拆成三件可观察的事:产品日志出现 connection lost/retry 与第二次
    CONNECTED、进程号没变(restarts.json 为空)、重连后帧数继续增长且 REST
    时间戳前进。任何一环不成立,链路就只是"看起来还活着"。
    """
    _wait_log_line("Bus 0 CONNECTED via TCP")
    pid_before = _read_pid()
    got = _wait_voltage()
    assert got is not None, "REST 一直没返回相位 A 电压"
    assert got == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"TCP 上给了 {VOLTAGE}V,REST 读到 {got}V —— 断链前采集就没成立,"
        "后面的重连断言没有基线"
    )

    _wait_drop()
    _wait_log_line("TCP link bus 0: connection lost, retry #", timeout=RECONNECT_WAIT_S)
    _wait_log_line("Bus 0 CONNECTED via TCP", count=2, timeout=RECONNECT_WAIT_S)

    pid_after = _read_pid()
    assert pid_after == pid_before, (
        f"重连前后进程号变了({pid_before} → {pid_after})—— "
        "这不是 link 重连,是产品重启(看 restarts.json 与 iems.log)"
    )
    restarts = json.loads((RUN_DIR / "restarts.json").read_text(encoding="utf-8"))
    assert restarts == [], f"产品在断链期间被重启过: {restarts}"

    # 轮询恢复:新连接上的帧必须继续落库,且 REST 时间戳前进(不是旧值撑着)。
    ts_before = int(latest(_port(), TAG_VOLTAGE).get("ts") or 0)
    frames_before = bus_seen()["requests"]
    deadline = time.time() + WAIT_S
    value = ts = None
    while time.time() < deadline:
        got = latest(_port(), TAG_VOLTAGE)
        value = got.get("value")
        ts = int(got.get("ts") or 0)
        if bus_seen()["requests"] > frames_before and ts > ts_before:
            break
        time.sleep(0.3)
    else:
        pytest.fail(
            f"重连后 {WAIT_S:.0f}s 内轮询没恢复:帧数停在 {bus_seen()['requests']}"
            f"(重连时 {frames_before}),REST ts {ts_before} → {ts}"
        )
    assert value is not None and abs(float(value) - VOLTAGE) <= TOLERANCE_V, (
        f"重连后 REST 读到 {value}V,台架给的是 {VOLTAGE}V —— 链路回来了但采集没回来"
    )

    meter.connect()
    mirror = meter.read_all().readings["电压"]
    assert abs(mirror - VOLTAGE) <= TOLERANCE_V, (
        f"重连后北向镜像 电压 = {mirror},台架给的是 {VOLTAGE}±{TOLERANCE_V}"
    )

    errors = bus_seen().get("errors") or []
    assert not errors, f"台架在处理帧时抛了异常,这一轮结论无效: {errors}"
