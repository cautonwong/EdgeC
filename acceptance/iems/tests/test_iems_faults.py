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

from bench import RUN_DIR, VOLTAGE, latest, port_of
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


@case(id="I_00_00_01_000020", clause="IEMS-ROBUST-1", tags=["故障"])
def test_bad_crc_frame_never_becomes_a_reading():
    assert _wait_faults(2), "台架一帧错帧都没注入 —— 这条用例什么都没验到(假绿)"
    seen = _sample(4.0)
    bogus = [v for v in seen if abs(v - BAD_VALUE) <= TOLERANCE_V]
    assert not bogus, (
        f"错帧里的 {BAD_VALUE}V 被当成了读数: {bogus} —— "
        "主站必须在 CRC 校验失败时整帧丢弃,不能取载荷"
    )
    assert seen, "错帧之后 REST 一直没有读数,不能算通过"
    assert seen[-1] == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"恢复后的读数 {seen[-1]}V 不是台架真值 {VOLTAGE}V(见过的值 {seen})"
    )


@case(id="I_00_00_01_000021", clause="IEMS-ROBUST-2", tags=["故障"])
def test_slow_response_does_not_stall_the_product():
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
