"""产品重启验收:新会话必须重发 BIRTH,最后一次采集值必须从持久化路径恢复。

## 为什么另起一个任务

一个任务只拉一次产品,"重启后行为"表达不了。产品壳因此多了一条控制面:
`RUN_DIR/restart.request` → 按 SIGTERM(3 秒后强杀)停产品 → 同一 env 再起,
`RUN_DIR/crash.request` → 直接 SIGKILL → 同一 env 再起。进程号、退出码与
通道(graceful/crash)落 `RUN_DIR/restarts.json`。重启前后 MQTT 会话各一个,
BIRTH 的"每会话一次"才验得到。

## 首个 BIRTH 为什么要启动闸门

观察器在 pytest 启动时才订阅,而产品是 run_task 在 pytest 之前拉起的 ——
BIRTH 每会话只发一次,早就过去了。任务壳因此先占住 ${PORT} 让就绪探针通过,
等本用例(观察器已订阅)写 `RUN_DIR/start.request` 再起产品。

## 判据

1. 重启前:MqttJson 会话里有上线帧(`{"online":true,...}`)
2. 重启前后进程号不同(证明真的重启了),优雅退出(3 秒内、退出码 0)
3. 重启后:新会话里出现第二个上线帧(驱动契约:新会话重发)
4. 重启后:REST 与北向镜像恢复最后一次采集值,WAL 回放条数 > 0
   —— 只断言"值对了"不够:新一轮轮询也会给出同样的值
5. SIGKILL 之后没有优雅收尾,恢复只能靠 WAL:回放条数 > 0、REST 值回来、
   南向轮询继续、新会话重发 BIRTH,且台架自身没有 `_fail` 记录
"""
import json
import os
import re
import time

import pytest

from bench import (RUN_DIR, TAG_VOLTAGE, VOLTAGE, bus_seen, latest, port_of,
                   rest)
from testplatform.cases import case
from testplatform.protocol import mqtt_json

TOLERANCE_V = 0.5
BOOT_WAIT_S = 30.0
RESTART_WAIT_S = 30.0
REPLAY_RE = re.compile(r"WAL replayed successfully, loaded (\d+) events")


@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    """保住证据会话:没有它 sessionfinish 不写 run-scope.json。"""
    return evidence_session


def _read_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None


def _diag(limit: int = 20) -> str:
    """失败现场:产品日志末尾与重启记录。"""
    try:
        lines = (RUN_DIR / "iems.log").read_text(encoding="utf-8", errors="replace").splitlines()
        tail = "\n".join(lines[-limit:])
    except OSError:
        tail = "(没有 iems.log)"
    return f"\n--- iems.log 末尾 ---\n{tail}\n--- restarts.json ---\n{_read_json(RUN_DIR / 'restarts.json')}"


def _wait_port() -> int:
    deadline = time.time() + 10
    while time.time() < deadline:
        try:
            return port_of()
        except (FileNotFoundError, ValueError):
            time.sleep(0.2)
    pytest.fail("产品壳没有写出监听端口,服务没起来")


def _wait_health() -> None:
    """就绪探针语义:端口能连不等于服务可用,健康检查必须真的应。"""
    deadline = time.time() + BOOT_WAIT_S
    body = {}
    while time.time() < deadline:
        body = rest(_wait_port(), "/api/v1/system/health")
        if "error" not in body:
            return
        time.sleep(0.2)
    pytest.fail(f"产品 REST 一直不可用: {body}{_diag()}")


def _wait_voltage(timeout: float = BOOT_WAIT_S) -> float | None:
    port = _wait_port()
    deadline = time.time() + timeout
    got = None
    while time.time() < deadline:
        value = latest(port, TAG_VOLTAGE).get("value")
        if value is not None:
            got = float(value)
            if abs(got - VOLTAGE) <= TOLERANCE_V:
                return got
        time.sleep(0.3)
    return got


def _node_id() -> str:
    return os.environ.get("MQTT_NODE_ID", "IEMS_NB_E2E")


def _is_our_birth(msg) -> bool:
    """本节点的上线帧:主题里带节点身份,载荷带 online 键。"""
    return _node_id() in msg.topic and mqtt_json.is_birth(msg.payload)


def _wait_birth(probe, start: int, what: str) -> None:
    deadline = time.time() + BOOT_WAIT_S
    while time.time() < deadline:
        for msg in probe.messages[start:]:
            if _is_our_birth(msg):
                return
        time.sleep(0.05)
    seen = [f"{m.topic} ({len(m.payload)}B)" for m in probe.messages[start:]][-5:]
    pytest.fail(f"{what}:{BOOT_WAIT_S:.0f}s 内没收到上线帧;期间收到 {seen}{_diag()}")


def _read_pid() -> int:
    path = RUN_DIR / "product.pid"
    deadline = time.time() + 10
    while time.time() < deadline:
        try:
            return int(path.read_text(encoding="utf-8").strip())
        except (OSError, ValueError):
            time.sleep(0.05)
    pytest.fail(f"产品壳没有写出进程号({path})")


def _wait_restart(count: int) -> dict:
    path = RUN_DIR / "restarts.json"
    deadline = time.time() + RESTART_WAIT_S
    while time.time() < deadline:
        restarts = _read_json(path)
        if isinstance(restarts, list) and len(restarts) > count:
            return restarts[count]
        time.sleep(0.05)
    pytest.fail(f"{RESTART_WAIT_S:.0f}s 内产品没按 restart.request 重启(看 {path}){_diag()}")


def _wait_log_replay(after_offset: int) -> int:
    """崩溃后追加进 iems.log 的 WAL 回放条数。"""
    path = RUN_DIR / "iems.log"
    deadline = time.time() + BOOT_WAIT_S
    while time.time() < deadline:
        try:
            text = path.read_text(encoding="utf-8", errors="replace")[after_offset:]
        except OSError:
            text = ""
        replays = [int(n) for n in REPLAY_RE.findall(text)]
        if replays:
            return replays[-1]
        time.sleep(0.2)
    pytest.fail(f"崩溃重启后 {BOOT_WAIT_S:.0f}s 内没有 WAL 回放行{_diag()}")


def _wait_bus_reads(after: int) -> None:
    deadline = time.time() + BOOT_WAIT_S
    while time.time() < deadline:
        if len(bus_seen().get("reads") or []) > after:
            return
        time.sleep(0.2)
    pytest.fail(f"崩溃重启后 {BOOT_WAIT_S:.0f}s 内南向轮询没恢复:读帧数停在 {after}{_diag()}")


@case(id="I_00_00_07_000001", clause="IEMS-RESTART-1", tags=["验收", "重启"])
@pytest.mark.transport("restart")
@pytest.mark.contract
def test_restart_rebirths_and_restores_last_value(northbound_probe, meter):
    """产品重启后:新会话重发 BIRTH,REST/北向/WAL 恢复最后一次采集值。"""
    assert not any(_is_our_birth(m) for m in northbound_probe.messages), (
        "观察器订阅之前就收到了本节点的 BIRTH —— 启动闸门没起作用,首个上线帧会被漏掉"
    )
    # 观察器已订上(pytest_configure 里 subscribe 等过 SUBACK),放产品启动。
    (RUN_DIR / "start.request").write_text("1", encoding="utf-8")

    first_seen = len(northbound_probe.messages)
    _wait_birth(northbound_probe, first_seen, "重启前的首个 BIRTH")
    _wait_health()
    got = _wait_voltage()
    assert got is not None and got == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"重启前采集没落到 REST(读到 {got}V,台架 {VOLTAGE}V):恢复断言没有基线"
    )
    pid_before = _read_pid()

    post_restart_seen = len(northbound_probe.messages)
    (RUN_DIR / "restart.request").write_text('{"reason": "acceptance"}', encoding="utf-8")
    restart = _wait_restart(0)

    assert restart["pid"] == pid_before, (
        f"重启记录里的旧进程号 {restart['pid']} 与重启前 {pid_before} 不一致"
    )
    assert restart["new_pid"] != pid_before, (
        f"进程号没有变化({pid_before})—— 产品没有真的重启"
    )
    assert restart["forced"] is False, (
        f"产品 3 秒内没走完 SIGTERM 优雅退出,被强杀了: {restart}"
    )
    assert restart["mode"] == "graceful", (
        f"这条通道不是优雅退出: {restart}"
    )
    assert restart["exit_code"] == 0, (
        f"优雅退出返回了 {restart['exit_code']},不是 0: {restart}"
    )

    _wait_birth(northbound_probe, post_restart_seen, "重启后的第二个 BIRTH")
    _wait_health()

    got = _wait_voltage()
    assert got is not None and got == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"重启后 REST 没恢复最后一次采集值(读到 {got}V,台架 {VOLTAGE}V)"
    )

    meter.connect()
    value = meter.read_all().readings["电压"]
    assert abs(value - VOLTAGE) <= TOLERANCE_V, (
        f"重启后北向镜像 电压 = {value},台架给的是 {VOLTAGE}±{TOLERANCE_V}"
    )

    text = (RUN_DIR / "iems.log").read_text(encoding="utf-8", errors="replace")
    replays = [int(n) for n in REPLAY_RE.findall(text)]
    assert replays, f"重启后的启动日志里没有 WAL 回放行: {RUN_DIR / 'iems.log'}"
    assert replays[-1] > 0, (
        f"重启后 WAL 回放 {replays[-1]} 条事件 —— 持久化路径没被用上,"
        "值只可能来自新一轮轮询,证明不了'恢复'"
    )


@case(id="I_00_00_07_000002", clause="IEMS-RESTART-2", tags=["验收", "重启", "崩溃"])
@pytest.mark.transport("restart")
@pytest.mark.contract
def test_crash_restart_replays_wal_and_resumes(northbound_probe, meter):
    """SIGKILL 后重启:WAL 回放恢复最后值,新会话重发 BIRTH,轮询继续。"""
    assert not any(_is_our_birth(m) for m in northbound_probe.messages), (
        "观察器订阅之前就收到了本节点的 BIRTH —— 启动闸门没起作用,首个上线帧会被漏掉"
    )
    (RUN_DIR / "start.request").write_text("1", encoding="utf-8")

    first_seen = len(northbound_probe.messages)
    _wait_birth(northbound_probe, first_seen, "崩溃前的首个 BIRTH")
    _wait_health()
    got = _wait_voltage()
    assert got is not None and got == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"崩溃前采集没落到 REST(读到 {got}V,台架 {VOLTAGE}V):恢复断言没有基线{_diag()}"
    )
    pid_before = _read_pid()
    reads_before = len(bus_seen().get("reads") or [])
    log_offset = (RUN_DIR / "iems.log").stat().st_size
    post_crash_seen = len(northbound_probe.messages)

    (RUN_DIR / "crash.request").write_text('{"reason": "acceptance"}', encoding="utf-8")
    crash = _wait_restart(0)

    assert crash["mode"] == "crash" and crash["forced"] is True, (
        f"这不是 SIGKILL 路径: {crash}{_diag()}"
    )
    assert crash["exit_code"] == -9, (
        f"被 SIGKILL 的进程退出码应为 -9,记录是 {crash['exit_code']}: {crash}{_diag()}"
    )
    assert crash["pid"] == pid_before, (
        f"崩溃记录里的旧进程号 {crash['pid']} 与崩溃前 {pid_before} 不一致{_diag()}"
    )
    assert crash["new_pid"] != pid_before, (
        f"进程号没有变化({pid_before})—— 产品没有真的重启{_diag()}"
    )

    _wait_birth(northbound_probe, post_crash_seen, "崩溃重启后的第二个 BIRTH")
    _wait_health()

    replayed = _wait_log_replay(log_offset)
    assert replayed > 0, (
        f"崩溃重启后 WAL 回放 {replayed} 条事件 —— 持久化路径没被用上,"
        f"值只可能来自新一轮轮询{_diag()}"
    )

    got = _wait_voltage()
    assert got is not None and got == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"崩溃重启后 REST 没恢复最后一次采集值(读到 {got}V,台架 {VOLTAGE}V){_diag()}"
    )

    _wait_bus_reads(reads_before)

    meter.connect()
    value = meter.read_all().readings["电压"]
    assert abs(value - VOLTAGE) <= TOLERANCE_V, (
        f"崩溃重启后北向镜像 电压 = {value},台架给的是 {VOLTAGE}±{TOLERANCE_V}{_diag()}"
    )

    errors = bus_seen().get("errors") or []
    assert not errors, f"台架在处理帧时抛了异常,这一轮结论无效: {errors}{_diag()}"
