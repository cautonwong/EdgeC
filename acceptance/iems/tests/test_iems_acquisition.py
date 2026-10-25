"""iems 采集链路验收:台架在总线上给了多少,iems 的 REST 必须报多少。

判据三步,顺序有意义 —— 前面不成立时,后面不成立不能算 iems 的错:
  1. 服务起来、REST 可用(否则整轮的失败原因不是采集)
  2. iems 必须向台架发出 Modbus 读请求(否则 REST 上的数不是从总线来的)
  3. REST 报的电压必须等于台架给的值(±0.5V)

为什么判据 2 不能省:实测过一次,总线一帧都没有时,心跳照样每秒打印
`[HEARTBEAT] Shadow Tag 1001 = 238.50` —— 那是上一轮跑留下的 WAL 被回放
(`[DataHub] WAL replayed successfully, loaded 26753 events`)。只查"REST 有值"
会被陈旧值骗过。
"""
import time

import pytest

from bench import (BUS_LOG, SOCK, TAG_VOLTAGE, VOLTAGE, bus_seen, latest,
                   port_of, rest)
from testplatform.cases import case

TOLERANCE_V = 0.5


@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    """保住证据会话:没有它 sessionfinish 不写 run-scope.json,
    归因会把整轮判成「范围缺失」而不是「用例失败」。"""
    return evidence_session


def _port() -> int:
    deadline = time.time() + 10
    while time.time() < deadline:
        try:
            return port_of()
        except (FileNotFoundError, ValueError):
            time.sleep(0.2)
    pytest.fail("产品壳没有写出监听端口,服务没起来")


def _wait_value(timeout: float = 8.0) -> float | None:
    port = _port()
    deadline = time.time() + timeout
    while time.time() < deadline:
        got = latest(port, TAG_VOLTAGE).get("value")
        if got:
            return float(got)
        time.sleep(0.5)
    return 0.0


@case(id="I_00_00_01_000001", clause="IEMS-SVC-1", tags=["验收"])
@pytest.mark.contract
def test_rest_health_is_up():
    body = rest(_port(), "/api/v1/system/health")
    assert "error" not in body, f"REST 不可用: {body}"


@case(id="I_00_00_01_000002", clause="IEMS-ACQ-2", tags=["验收"])
@pytest.mark.contract
def test_iems_polls_the_bench_bus():
    seen = bus_seen()
    assert seen["requests"] > 0, (
        f"iems 启动后没有向 {SOCK} 发过任何 Modbus 请求 —— "
        "南向轮询没跑起来(检查 sqlite 里的 poll_tasks/inventory 与 matrix_meta)"
    )
    # 1 秒一轮;给到 2 帧才能说"周期在跑",1 帧可能是启动时那一下。
    time.sleep(2.5)
    again = bus_seen()
    assert again["requests"] > seen["requests"], (
        f"只发了 {seen['requests']} 帧就停了,不是周期轮询: {BUS_LOG}"
    )


@case(id="I_00_00_01_000003", clause="IEMS-ACQ-1", tags=["验收"])
@pytest.mark.contract
def test_rest_reports_the_bench_voltage():
    got = _wait_value()
    assert got is not None, "REST 一直没返回相位 A 电压"
    assert got == pytest.approx(VOLTAGE, abs=TOLERANCE_V), (
        f"台架给了 {VOLTAGE}V(iems 的实时接口读到 {got}V)"
    )
