"""故障注入:错帧与慢应答。

判据写在验收条款里(IEMS-ROBUST-1/2),这里只做取证:
  - 错帧:台架塞的是 99.9V(载荷 999),iems 必须整帧丢弃 —— 对外任何地方
    都不许出现这个值,之后要回到 231.7。
  - 慢应答:每条应答延迟 3 秒(超过一轮轮询周期),iems 不许退出、不许写 0,
    恢复后读数必须是 231.7。

产品自己的日志不算证据:这里是拿它对外的 REST 读数当观察面。
"""
import json
import time
from pathlib import Path

import pytest

from bench import (RUN_DIR, TAG_CURRENT, TAG_VOLTAGE, VOLTAGE, bus_seen, latest,
                   port_of)
from testplatform.cases import case

BAD_VALUE = 99.9
TOLERANCE_V = 0.5


@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    return evidence_session


def _faults_served() -> list:
    path = Path(RUN_DIR) / "faults.json"
    return json.loads(path.read_text(encoding="utf-8")) if path.is_file() else []


def _wait_faults(count: int, deadline_s: float = 20.0) -> list:
    """等台架真的把故障注入出去。

    这一步是防空转的:没有它,用例可能连的是别的进程(端口被上一轮占用),
    什么都没注入也会"通过"。
    """
    deadline = time.time() + deadline_s
    while time.time() < deadline:
        served = _faults_served()
        if len(served) >= count:
            return served
        time.sleep(0.2)
    return _faults_served()


def _sample(seconds: float) -> list:
    """在窗口内连续采集 REST 报的 tag 1001 值。

    错帧那条必须在整个窗口里盯住:只看"最终值对不对"会漏掉中间那一瞬把错帧
    载荷当读数的情形。
    """
    port = port_of()
    values, deadline = [], time.time() + seconds
    while time.time() < deadline:
        body = latest(port, 1001)
        if body.get("tag_id") == 1001 and body.get("value") is not None:
            values.append(float(body["value"]))
        time.sleep(0.2)
    return values


def _wait_latest(deadline_s: float):
    """等 REST 报出一个 tag 1001 的读数。

    返回 (值, 见过的 1001 值列表)。`{"tag_id":0,"value":0}` 是"还没有数据"的形状,
    不是"读到 0",所以按 tag_id 过滤,别把它当成产品写了 0。
    """
    port = port_of()
    seen = []
    seen = []
    deadline = time.time() + deadline_s
    while time.time() < deadline:
        body = latest(port, 1001)
        if body.get("tag_id") == 1001 and body.get("value") is not None:
            volts = float(body["value"])  # 已经是工程值(scale 在映射里算过)
            seen.append(volts)
            return volts, seen
        time.sleep(0.5)
    return None, seen


def _wait_latest_tag(port: int, tag_id: int, deadline_s: float) -> dict | None:
    """等某个 tag 第一次真的被发布(tag_id 对得上,不是空槽的 tag 0)。"""
    deadline = time.time() + deadline_s
    while time.time() < deadline:
        body = latest(port, tag_id)
        if body.get("tag_id") == tag_id and body.get("value") is not None:
            return body
        time.sleep(0.2)
    return None


def _slow_netbuf_warnings() -> list[str]:
    """产品日志里的 Long-lived NetBuf 告警(迟到帧长期占着 NetBuf 的证据)。"""
    path = Path(RUN_DIR) / "iems.log"
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except FileNotFoundError:
        return []
    return [line for line in text.splitlines() if "Long-lived NetBuf" in line]


@case(id="I_00_00_01_000020", clause="IEMS-ROBUST-1", tags=["故障"])
def test_bad_crc_frame_never_becomes_a_reading(meter):
    assert _wait_faults(2), "台架一帧错帧都没注入 —— 这条用例什么都没验到(假绿)"
    # 两次坏 CRC 各自按事务超时(2s)收尾,超时后还有一条恢复窗口(2s):
    # 采样窗要盖住「超时 + 窗口」两轮,否则测的是恢复速度而不是 CRC 语义。
    seen = _sample(8.0)
    bogus = [v for v in seen if abs(v - BAD_VALUE) <= TOLERANCE_V]
    assert not bogus, (
        f"错帧里的 {BAD_VALUE}V 被当成了读数: {bogus} —— "
        "主站必须在 CRC 校验失败时整帧丢弃,不能取载荷"
    )
    assert seen, "错帧之后 REST 一直没有读数,不能算通过"
    assert seen[-1] == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"恢复后的读数 {seen[-1]}V 不是台架真值 {VOLTAGE}V(见过的值 {seen})"
    )
    # 从平台这一侧再读一次:结论要落在设备上(带读数证据),不能只信产品自己的 REST。
    meter.connect()
    assert meter.read_all().readings["电压"] == pytest.approx(VOLTAGE, abs=TOLERANCE_V)


@case(id="I_00_00_01_000021", clause="IEMS-ROBUST-2", tags=["故障"])
def test_slow_response_does_not_stall_the_product(meter):
    assert len(_wait_faults(2)) >= 2, "台架没有真的注入慢应答 —— 假绿"
    got, seen = _wait_latest(25.0)
    zeros = [v for v in seen if abs(v) <= 0.01]
    assert got is not None, f"慢应答之后一直没读数(见过的值 {seen})"
    assert not zeros, (
        f"慢应答期间 REST 报了 0: {seen} —— 超时不许写成 0,"
        "那会让下游把'没读到'当成'读到了 0'"
    )
    assert got == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"恢复后的读数 {got}V 不是台架真值 {VOLTAGE}V(见过的值 {seen})"
    )
    # 慢应答过去之后,平台这一侧必须重新读得到 —— 只验产品自报等于信它自己的话。
    meter.connect()
    assert meter.read_all().readings["电压"] == pytest.approx(VOLTAGE, abs=TOLERANCE_V)


@case(id="I_00_00_01_000022", clause="IEMS-ROBUST-4", tags=["故障"])
def test_slow_response_below_timeout_never_crosses_tags(meter):
    """1.2s 应答(<2s 超时)慢于 1s 轮询:迟到帧不许解码成下一个 tag 的读数。

    判据只看对外 REST:1001 在整个观察窗内必须一直是台架电压(串 tag 时会拿到
    台架给 1002 的 0);1002 必须有自己的 tag_id 且 ts 前进;bus.json 的读地址
    仍是 0/1002,证明产品没有发错请求。
    """
    assert len(_wait_faults(2)) >= 2, "台架没有真的注入慢应答 —— 假绿"
    port = port_of()

    first = _wait_latest_tag(port, TAG_VOLTAGE, 25.0)
    assert first is not None, "1.2s 慢应答后一直没读到 tag 1001"
    assert float(first["value"]) == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"tag 1001 首个读数 {first['value']}V 不是台架真值 {VOLTAGE}V"
    )

    values, tss, deadline = [], [], time.time() + 8.0
    while time.time() < deadline:
        body = latest(port, TAG_VOLTAGE)
        if body.get("tag_id") == TAG_VOLTAGE and body.get("value") is not None:
            values.append(float(body["value"]))
            tss.append(int(body["ts"]))
        time.sleep(0.2)

    assert len(values) >= 3, f"8 秒窗口内 tag 1001 只报了 {len(values)} 次,不能算验过"
    crossed = [v for v in values if abs(v - VOLTAGE) > TOLERANCE_V]
    assert not crossed, (
        f"tag 1001 被写入了别的 tag 的值: {crossed}(见过的值 {values}) —— "
        "迟到应答必须排空,不能解码到下一个 tag"
    )
    assert len(set(tss)) >= 2, f"tag 1001 的时间戳没有前进: {tss}"

    current = _wait_latest_tag(port, TAG_CURRENT, 25.0)
    assert current is not None, "latest/1002 一直 not_found —— 慢应答下 tag 1002 一次都没落 shadow"
    ts_before = int(current["ts"])
    deadline = time.time() + 12.0
    ts_after = ts_before
    while time.time() < deadline and ts_after <= ts_before:
        body = latest(port, TAG_CURRENT)
        if body.get("tag_id") == TAG_CURRENT and body.get("ts") is not None:
            ts_after = int(body["ts"])
        time.sleep(0.2)
    assert int(current["tag_id"]) == TAG_CURRENT
    assert ts_after > ts_before, (
        f"tag 1002 的 shadow 时间戳停住({ts_before} -> {ts_after}),轮询没有恢复"
    )

    addresses = set(bus_seen().get("addresses") or [])
    assert 0 in addresses and 1002 in addresses, (
        f"台架只见到读地址 {sorted(addresses)} —— 产品没有按 0/1002 两个点位轮询"
    )
    assert addresses <= {0, 1002}, f"出现了点位之外的读地址: {sorted(addresses)}"

    log_path = Path(RUN_DIR) / "iems.log"
    assert log_path.is_file(), "iems.log 缺失(任务要带 --record-startup),NetBuf 断言没有观察面"
    warnings = _slow_netbuf_warnings()
    assert not warnings, "迟到帧仍长期占着 NetBuf(应先排空再结算):\n" + "\n".join(warnings)


@case(id="I_00_00_01_000023", clause="IEMS-ROBUST-4", tags=["故障"])
def test_true_timeout_late_response_does_not_cross_tags(meter):
    """3s 应答 > 2s 事务超时:超时结算后总线必须先等迟到应答落地再派发。

    迟到帧(3s)比超时晚 1s;若下一事务立刻上线,它会先收到上一帧 —— RTU 读
    应答没有寄存器地址,只能被当成自己的读数(1001 被 1002 的 0 覆盖,或反过来)。
    判据只看对外 REST 和台架记录,不看产品自报日志。
    """
    assert len(_wait_faults(2)) >= 2, "台架没有真的注入慢应答 —— 假绿"
    port = port_of()

    first = _wait_latest_tag(port, TAG_VOLTAGE, 30.0)
    assert first is not None, "3s 慢应答(超过 2s 超时)之后一直没读到 tag 1001"
    assert float(first["value"]) == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"tag 1001 首个读数 {first['value']}V 不是台架真值 {VOLTAGE}V"
    )

    values, currents, tss, deadline = [], [], [], time.time() + 8.0
    while time.time() < deadline:
        body = latest(port, TAG_VOLTAGE)
        if body.get("tag_id") == TAG_VOLTAGE and body.get("value") is not None:
            values.append(float(body["value"]))
        cbody = latest(port, TAG_CURRENT)
        if cbody.get("tag_id") == TAG_CURRENT and cbody.get("value") is not None:
            currents.append(float(cbody["value"]))
            tss.append(int(cbody["ts"]))
        time.sleep(0.2)

    assert len(values) >= 3, f"8 秒窗口内 tag 1001 只报了 {len(values)} 次,不能算验过"
    crossed = [v for v in values if abs(v - VOLTAGE) > TOLERANCE_V]
    assert not crossed, (
        f"tag 1001 被写入了别的 tag 的值: {crossed}(见过的值 {values}) —— "
        "超时后的迟到应答必须被恢复窗口隔离,不能解码到下一事务"
    )
    assert currents, "tag 1002 一次都没有落 shadow,轮询没有恢复"
    assert all(abs(v) <= TOLERANCE_V for v in currents), (
        f"tag 1002 被写入了别的 tag 的值: {currents}(台架真值是 0A) —— "
        "回复 1001 的迟到帧串到了 1002"
    )

    current = _wait_latest_tag(port, TAG_CURRENT, 25.0)
    assert current is not None, "latest/1002 一直 not_found —— 恢复窗口后轮询没有继续"
    ts_before = int(current["ts"])
    deadline = time.time() + 12.0
    ts_after = ts_before
    while time.time() < deadline and ts_after <= ts_before:
        body = latest(port, TAG_CURRENT)
        if body.get("tag_id") == TAG_CURRENT and body.get("ts") is not None:
            ts_after = int(body["ts"])
        time.sleep(0.2)
    assert ts_after > ts_before, (
        f"tag 1002 的 shadow 时间戳停住({ts_before} -> {ts_after}),轮询没有恢复"
    )

    addresses = set(bus_seen().get("addresses") or [])
    assert 0 in addresses and 1002 in addresses, (
        f"台架只见到读地址 {sorted(addresses)} —— 产品没有按 0/1002 两个点位轮询"
    )
    assert addresses <= {0, 1002}, f"出现了点位之外的读地址: {sorted(addresses)}"
