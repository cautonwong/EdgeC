"""南向断链:iems 必须停下,而不是假装还在读,更不能死。

判据四步,顺序有意义:
  1. 断链前必须已经采到正确值(先走一次北向读数 —— 顺便把读数证据落到
     设备上,否则"失败但没读数"会被归因判成「未完成」而不是「设备不合格」)
  2. 断链后 REST 的时间戳必须停止前进(旧值可以留,但不能再盖新时间戳)
  3. 断链不许把 ts 清零(清成 0 也是一种"新读数",下游会当成本轮读到的值)
  4. 断链不许把服务带走:REST 仍可用、北向镜像仍是最后一个好值(不是 0)

为什么判据 1 的读数必须先做:平台只在设备有读数时才敢把失败记成"设备不合格";
没有读数的失败只能落"未完成"。这条不是为了凑覆盖,是为了让结论可归属。
"""
import time

import pytest

from bench import (DROPPED_LOG, TAG_VOLTAGE, VOLTAGE, bus_seen, dropped,
                   latest, port_of, rest)
from testplatform.cases import case

TOLERANCE_V = 0.5
SETTLE_S = 3.0    # 断链后允许的收尾时间(在途帧可能刚落库)
WATCH_S = 6.0     # 观察窗:这段时间内 ts 一次都不许前进


@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    return evidence_session


def _port() -> int:
    deadline = time.time() + 10
    while time.time() < deadline:
        try:
            return port_of()
        except (FileNotFoundError, ValueError):
            time.sleep(0.2)
    pytest.fail("产品壳没有写出监听端口,服务没起来")


def _ts() -> int:
    got = latest(_port(), TAG_VOLTAGE)
    assert "error" not in got, f"REST 不可用: {got}"
    return int(got.get("ts") or 0)


@case(id="I_00_00_01_000030", clause="IEMS-LINK-1", tags=["链路"])
@pytest.mark.contract
@pytest.mark.requires_capability("电压")
def test_link_loss_stops_publishing_without_killing_the_service(meter):
    # 判据 1:断链前先证明确实在正常采集。这次读数同时是证据。
    meter.connect()
    before = meter.read_all().readings["电压"]
    assert abs(before - VOLTAGE) <= TOLERANCE_V, (
        f"断链前读数就不对:镜像 电压 = {before},台架给的是 {VOLTAGE}±{TOLERANCE_V}"
    )
    ts_before = _ts()
    assert ts_before > 0, (
        f"断链前 REST 时间戳是 0(总线 {bus_seen()['requests']} 帧)—— "
        "没采集过就没有'断链后停止'可言"
    )

    # 等到台架真的断链(否则下面断言的是"没断之前"的状态,恒成立)
    deadline = time.time() + 25.0
    while time.time() < deadline:
        if dropped():
            break
        time.sleep(0.2)
    else:
        pytest.fail(
            f"台架没有断链记录({DROPPED_LOG}):总线只看到 {bus_seen()['requests']} 帧,"
            "产品没跑到断链那一帧,这一轮不能判 iems"
        )

    # 判据 2:断链后在途帧收尾 SETTLE_S,然后盯 WATCH_S,ts 一次都不许前进
    time.sleep(SETTLE_S)
    ts_settled = _ts()
    # 判据 3:不许清零/回退
    assert ts_settled >= ts_before, (
        f"断链后 REST 时间戳被清零或回退({ts_before} → {ts_settled})—— "
        "清成 0 也是一种新数据,下游会当成本轮读到的值"
    )
    time.sleep(WATCH_S)
    ts_after = _ts()
    assert ts_after == ts_settled, (
        f"断链 {WATCH_S:.0f} 秒后 REST 时间戳还在前进({ts_settled} → {ts_after}),"
        f"等于对外宣告'又读到了'。总线停在 {bus_seen()['requests']} 帧"
    )

    # 判据 4:服务与北向镜像都得活着
    health = rest(_port(), "/api/v1/system/health")
    assert "error" not in health, f"断链把服务带走了: {health}"
    after = meter.read_all().readings["电压"]
    assert abs(after - VOLTAGE) <= TOLERANCE_V, (
        f"断链后北向镜像 电压 = {after},最后一个好值是 {VOLTAGE}±{TOLERANCE_V};"
        "断链不许把镜像清成 0"
    )
